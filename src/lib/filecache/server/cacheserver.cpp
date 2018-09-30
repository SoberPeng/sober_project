/*
 * vzes
 * Copyright 2013 - 2018, Vzenith Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "filecache/server/cacheserver.h"
#include "eventservice/base/common.h"
#include <stdio.h>

namespace cache {

#define TYPE_ASYNC_DELETE     1
#define TYPE_ASYNC_WRITE      2
#define TYPE_ASYNC_READ       3

#define TYPE_CACHE_GATE       4

#define TYPE_KVDB_SET_KEY     5
#define TYPE_KVDB_GET_KEY     6
#define TYPE_KVDB_DELETE_KEY  7
#define TYPE_KVDB_BACKUP      8
#define TYPE_KVDB_RESTORE     9

#define TYPE_KVDB_GATE        10

/**< file cache缓存大小 */
#define CACHE_POOL_SIZE       4

#define CACHE_PATH_BUFF_LEN   512


#ifdef WIN32
#define KVDB_PARENT_FOLD  "D:\\kvdb"
#else
#ifdef LITEOS
#define KVDB_PARENT_FOLD  "/usr/kvdb"
#else
#define KVDB_PARENT_FOLD  "/mnt/usr/kvdb"
#endif
#endif

CacheServer *CacheServer::instance_ = NULL;

CacheServer *CacheServer::Instance() {
  if (!instance_) {
    instance_ = new CacheServer();
    instance_->InitCacheService();
  }
  return instance_;
}

CacheServer::CacheServer() {
}

CacheServer::~CacheServer() {
  event_service_->UninitEventService();
  KVDBs::iterator iter;
  for (iter = kvdbs_.begin(); iter != kvdbs_.end(); ++iter) {
    delete iter->second;
  }
  kvdbs_.clear();
}

void CacheServer::InitCacheService() {
  event_service_ = vzes::EventService::CreateEventService(NULL, "CacheService");
  cache_service_ =
    CachedService::Ptr(new CachedService(event_service_, CACHE_POOL_SIZE));
}

void CacheServer::AsyncWrite(CacheData::Ptr cache_data) {
  event_service_->Post(this, TYPE_ASYNC_WRITE, cache_data);
}

void CacheServer::AsyncRead(CacheData::Ptr cache_data) {
  event_service_->Post(this, TYPE_ASYNC_READ, cache_data);
}

void CacheServer::AsyncDelete(CacheData::Ptr cache_data) {
  event_service_->Post(this, TYPE_ASYNC_DELETE, cache_data);
}

void CacheServer::OnMessage(vzes::Message *msg) {

  ASSERT(event_service_->IsThisThread(vzes::Thread::Current()));

  if (msg->message_id < TYPE_CACHE_GATE) {

    CacheData::Ptr cache_data =
      boost::dynamic_pointer_cast<CacheData>(msg->pdata);
    if (msg->message_id == TYPE_ASYNC_DELETE) {
      OnDeleteEvent(cache_data);
    } else if (msg->message_id == TYPE_ASYNC_WRITE) {
      OnWriteEvent(cache_data);
    } else if (msg->message_id == TYPE_ASYNC_READ) {
      OnReadEvent(cache_data);
    } else {
      LOG(L_ERROR) << "Unkown CACHE message";
    }

  } else if (msg->message_id > TYPE_CACHE_GATE
             && msg->message_id < TYPE_KVDB_GATE) {
    KvdbData::Ptr kvdb_data =
      boost::dynamic_pointer_cast<KvdbData>(msg->pdata);
    if (msg->message_id == TYPE_KVDB_SET_KEY) {
      OnKVDBSet(kvdb_data);
    } else if (msg->message_id == TYPE_KVDB_GET_KEY) {
      OnKVDBGet(kvdb_data);
    } else if (msg->message_id == TYPE_KVDB_DELETE_KEY) {
      OnKVDBDelete(kvdb_data);
    } else if (msg->message_id == TYPE_KVDB_BACKUP) {
      OnKVDBBackup(kvdb_data);
    } else if (msg->message_id == TYPE_KVDB_RESTORE) {
      OnKVDBRestore(kvdb_data);
    } else {
      LOG(L_ERROR) << "Unkown KVDB message";
    }
  } else {
    LOG(L_ERROR) << "Unkown thread message";
  }
}

void CacheServer::OnWriteEvent(CacheData::Ptr cache_data) {
  if (cache_service_->SaveFile(cache_data->path, cache_data->buffer)) {
    cache_data->res = CACHE_DONE;
  } else {
    cache_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  }

  if (cache_data->signal_event) {
    cache_data->signal_event->TriggerSignal();
  }
}

void CacheServer::OnReadEvent(CacheData::Ptr cache_data) {
  cache_data->buffer = cache_service_->GetFile(cache_data->path);
  if (cache_data->buffer) {
    cache_data->res = CACHE_DONE;
  } else {
    cache_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  }

  if (cache_data->signal_event) {
    cache_data->signal_event->TriggerSignal();
  }
}

void CacheServer::OnDeleteEvent(CacheData::Ptr cache_data) {
  if (cache_service_->RemoveFile(cache_data->path)) {
    cache_data->res = CACHE_DONE;
  } else {
    cache_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  }

  if (cache_data->signal_event) {
    cache_data->signal_event->TriggerSignal();
  }
}

////////////////////////////////////////////////////////////////////////////////

void CacheServer::KVDBSetKey(KvdbData::Ptr kvdb_data) {
  event_service_->Post(this, TYPE_KVDB_SET_KEY, kvdb_data);
}

void CacheServer::KVDBGetKey(KvdbData::Ptr kvdb_data) {
  event_service_->Post(this, TYPE_KVDB_GET_KEY, kvdb_data);
}

void CacheServer::KVDBDeleteKey(KvdbData::Ptr kvdb_data) {
  event_service_->Post(this, TYPE_KVDB_DELETE_KEY, kvdb_data);
}

void CacheServer::KVDBBackUp(KvdbData::Ptr kvdb_data) {
  event_service_->Post(this, TYPE_KVDB_BACKUP, kvdb_data);
}

void CacheServer::KVDBRestore(KvdbData::Ptr kvdb_data) {
  event_service_->Post(this, TYPE_KVDB_RESTORE, kvdb_data);
}

KvdbService *CacheServer::GetKvdb(std::string &name) {
  char buf[CACHE_PATH_BUFF_LEN];
  KvdbService *p = NULL;
  KVDBs::iterator iter = kvdbs_.find(name);
  if (iter != kvdbs_.end()) {
    p = iter->second;
  } else if (name.length() > 0) {
    struct KvdbCache cache[2];
    cache[0].size = 128;
    cache[0].counter = 50;  // 6KB
    cache[1].size = 2048;
    cache[1].counter = 10;   // 20KB

    MAKE_FILE_NAME(buf, CACHE_PATH_BUFF_LEN, KVDB_PARENT_FOLD, name.c_str());
    p = new KvdbService(buf, cache, 2);
    kvdbs_[name] = p;
  }
  return p;
}

void CacheServer::OnKVDBSet(KvdbData::Ptr kvdb_data) {
  KvdbError rslt;
  KvdbService *p;

  kvdb_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  p = GetKvdb(kvdb_data->name);
  if(p) {
    rslt = p->Replace(kvdb_data->key, kvdb_data->value);
    if (rslt == KVDB_ERR_OK) {
      kvdb_data->res = CACHE_DONE;
    }
  }
  kvdb_data->signal_event->TriggerSignal();
}

void CacheServer::OnKVDBGet(KvdbData::Ptr kvdb_data) {
  KvdbError rslt;
  KvdbService *p;

  kvdb_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  p = GetKvdb(kvdb_data->name);
  if(p) {
    rslt = p->Seek(kvdb_data->key, kvdb_data->value);
    if (rslt == KVDB_ERR_OK) {
      kvdb_data->res = CACHE_DONE;
    }
  }
  kvdb_data->signal_event->TriggerSignal();
}

void CacheServer::OnKVDBDelete(KvdbData::Ptr kvdb_data) {
  KvdbError rslt;
  KvdbService *p;

  kvdb_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  p = GetKvdb(kvdb_data->name);
  if(p) {
    rslt = p->Remove(kvdb_data->key);
    if (rslt == KVDB_ERR_OK) {
      kvdb_data->res = CACHE_DONE;
    }
  }
  kvdb_data->signal_event->TriggerSignal();
}

void CacheServer::OnKVDBBackup(KvdbData::Ptr kvdb_data) {
  KvdbError rslt;
  KvdbService *p;
  char bak_foldpath[CACHE_PATH_BUFF_LEN];

  kvdb_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  p = GetKvdb(kvdb_data->name);
  if(p) {
#ifdef WIN32
    snprintf(bak_foldpath, CACHE_PATH_BUFF_LEN, "%s\\%s_bak",
             KVDB_PARENT_FOLD, kvdb_data->name.c_str());
#else
    snprintf(bak_foldpath, CACHE_PATH_BUFF_LEN, "%s/%s_bak",
             KVDB_PARENT_FOLD, kvdb_data->name.c_str());
#endif
    rslt = p->Backup(bak_foldpath);
    if (rslt == KVDB_ERR_OK) {
      kvdb_data->res = CACHE_DONE;
    }
  }
  kvdb_data->signal_event->TriggerSignal();
}

void CacheServer::OnKVDBRestore(KvdbData::Ptr kvdb_data) {
  KvdbError rslt;
  KvdbService *p;
  char bak_foldpath[CACHE_PATH_BUFF_LEN];

  kvdb_data->res = CACHE_ERROR_FILE_NOT_FOUND;
  p = GetKvdb(kvdb_data->name);
  if(p) {
#ifdef WIN32
    _snprintf(bak_foldpath, CACHE_PATH_BUFF_LEN, "%s\\%s_bak",
              KVDB_PARENT_FOLD, kvdb_data->name.c_str());
#else
    snprintf(bak_foldpath, CACHE_PATH_BUFF_LEN, "%s/%s_bak",
             KVDB_PARENT_FOLD, kvdb_data->name.c_str());
#endif
    rslt = p->Restore(bak_foldpath);
    if (rslt == KVDB_ERR_OK) {
      kvdb_data->res = CACHE_DONE;
    }
  }
  kvdb_data->signal_event->TriggerSignal();
}

}
