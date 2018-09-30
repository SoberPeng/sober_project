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
#include "filecache/base/basedefine.h"
#include "filecache/kvdb/kvdbclient.h"

namespace cache {

class KvdbClientImpl : public KvdbClient,
  public boost::noncopyable,
  public boost::enable_shared_from_this<KvdbClientImpl>,
  public sigslot::has_slots<> {
 public:
  KvdbClientImpl(const std::string& name) {
    cache_service_ = CacheServer::Instance();
    ASSERT(cache_service_ != NULL);
    signal_event_ = vzes::SignalEvent::CreateSignalEvent();
    name_ = name;
  }

  virtual ~KvdbClientImpl() {
  }


  bool SetKey(const char *key, uint8 key_size,
              const char *value, uint32 value_size) {
    std::string skey(key, key_size);
    return SetKey(skey, value, value_size);
  }

  bool SetKey(const std::string key,
              const char *value, uint32 value_size) {
    vzes::CritScope cr(&crit_);

    KvdbData::Ptr kvdb_data(new KvdbData);
    kvdb_data->name         = name_;
    kvdb_data->key          = key;
    kvdb_data->value.append(value, value_size);
    kvdb_data->signal_event = signal_event_;
    kvdb_data->res          = CACHE_ERROR_TIMEOUT;

    cache_service_->KVDBSetKey(kvdb_data);
    int signal_res = signal_event_->WaitSignal(DEFAULT_TIMEOUT);
    if (signal_res != SIGNAL_EVENT_DONE) {
      LOG(L_ERROR) << "return false";
      return KVDB_FAILURE;
    }
    if (kvdb_data->res != CACHE_DONE) {
      return KVDB_FAILURE;
    }
    return KVDB_SUCCEED;
  }

  bool GetKey(const char *key, uint8 key_size,
              std::string *result)  {
    std::string skey(key, key_size);
    return GetKey(skey, result);
  }

  bool GetKey(const std::string key,
              std::string *result)  {
    vzes::CritScope cr(&crit_);
    KvdbData::Ptr kvdb_data(new KvdbData);
    kvdb_data->name         = name_;
    kvdb_data->key          = key;
    kvdb_data->signal_event = signal_event_;
    kvdb_data->res          = CACHE_ERROR_TIMEOUT;

    cache_service_->KVDBGetKey(kvdb_data);
    int signal_res = signal_event_->WaitSignal(DEFAULT_TIMEOUT);
    if (signal_res != SIGNAL_EVENT_DONE) {
      LOG(L_ERROR) << "return false";
      return KVDB_FAILURE;
    }
    if (kvdb_data->res != CACHE_DONE) {
      return KVDB_FAILURE;
    }
    *result = kvdb_data->value;
    return KVDB_SUCCEED;
  }

  bool GetKey(const std::string key,
              void *buffer,
              std::size_t buffer_size)  {
    std::string result;
    bool res = GetKey(key, &result);
    if (res != KVDB_SUCCEED) {
      return res;
    }
    if (result.size() > buffer_size) {
      LOG(L_ERROR) << "dst buffer no enough! "
                   << "value len = " << result.size()
                   << "dst buffer len = " << buffer_size;
      return KVDB_FAILURE;
    }
    memcpy(buffer, result.c_str(), result.size());
    return KVDB_SUCCEED;
  }

  bool GetKey(const char *key, uint8 key_size,
              void *buffer, std::size_t buffer_size)  {
    std::string skey(key, key_size);
    return GetKey(skey, buffer, buffer_size);
  }

  bool DeleteKey(const char *key, uint8 key_size)  {
    vzes::CritScope cr(&crit_);
    KvdbData::Ptr kvdb_data(new KvdbData);
    kvdb_data->name         = name_;
    kvdb_data->key.append(key, key_size);
    kvdb_data->signal_event = signal_event_;
    kvdb_data->res          = CACHE_ERROR_TIMEOUT;
    cache_service_->KVDBDeleteKey(kvdb_data);
    int signal_res = signal_event_->WaitSignal(DEFAULT_TIMEOUT);
    if (signal_res != SIGNAL_EVENT_DONE) {
      LOG(L_ERROR) << "return false";
      return KVDB_FAILURE;
    }
    if (kvdb_data->res != CACHE_DONE) {
      return KVDB_FAILURE;
    }
    return KVDB_SUCCEED;
  }

  bool BackupDatabase()  {
    vzes::CritScope cr(&crit_);

    KvdbData::Ptr kvdb_data(new KvdbData);
    kvdb_data->name         = name_;
    kvdb_data->signal_event = signal_event_;
    kvdb_data->res          = CACHE_ERROR_TIMEOUT;
    cache_service_->KVDBBackUp(kvdb_data);
    int signal_res = signal_event_->WaitSignal(DEFAULT_TIMEOUT);
    if (signal_res != SIGNAL_EVENT_DONE) {
      LOG(L_ERROR) << "return false";
      return KVDB_FAILURE;
    }
    if (kvdb_data->res != CACHE_DONE) {
      return KVDB_FAILURE;
    }
    return KVDB_SUCCEED;
  }

  bool RestoreDatabase()  {
    vzes::CritScope cr(&crit_);
    KvdbData::Ptr kvdb_data(new KvdbData);
    kvdb_data->name         = name_;
    kvdb_data->signal_event = signal_event_;
    kvdb_data->res          = CACHE_ERROR_TIMEOUT;
    cache_service_->KVDBRestore(kvdb_data);
    int signal_res = signal_event_->WaitSignal(DEFAULT_TIMEOUT);
    if (signal_res != SIGNAL_EVENT_DONE) {
      LOG(L_ERROR) << "return false";
      return KVDB_FAILURE;
    }
    if (kvdb_data->res != CACHE_DONE) {
      return KVDB_FAILURE;
    }
    return KVDB_SUCCEED;
  }
 private:
  vzes::SignalEvent::Ptr  signal_event_;
  vzes::EventService::Ptr cs_service_;
  CacheServer            *cache_service_;
  vzes::CriticalSection   crit_;
  static const uint32     DEFAULT_TIMEOUT   = 5000;
  std::string             name_;
};

KvdbClient::Ptr KvdbClient::CreateKvdbClient(const std::string& name) {
  return KvdbClient::Ptr(new KvdbClientImpl(name));
}

}

