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

#include "filecache/server/cacheservice.h"
#include "filecache/base/diskhelper.h"
#include "eventservice/base/logging.h"
#include "eventservice/base/timeutils.h"
#include "json/json.h"

#ifndef WIN32
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#endif

namespace cache {

#ifdef WIN32
#define vzsleep(x)             Sleep(x)
#define FILE_LIMIT_CONFIG      "E:/file_limit_config.json"
#else
#define vzsleep(x)             usleep(x * 1000);
#define FILE_LIMIT_CONFIG      "/tmp/app/exec/file_limit_config.json"
#endif

#define MAX_PATH_SIZE          256
#define JSON_FLC_LIMIT_CHECKS  "limit_checks"
#define JSON_FLC_PATH          "path"
#define JSON_FLC_MAX_SIZE      "max_size"
#define SD_CAP_PATH            "/media/mmcblk0p1/VzIPCCap"


CachedService::CachedService(vzes::EventService::Ptr es,
                             std::size_t cache_size)
  : cache_size_(cache_size)
  , current_cache_size_(0)
  , event_service_(es)
  , current_stanzas_index_(0) {
  cachedstanza_pool_ = CachedStanzaPool::Instance();
  cachedstanza_pool_->SetDefaultCachedSize(cache_size);
  BOOST_ASSERT(cachedstanza_pool_ != NULL);
}

CachedService::~CachedService() {
}

bool CachedService::Start() {
  if (InitFileLimitCheck()) {
    StartFileLimitCheckTimer();
  }
  return true;
}

void CachedService::OnMessage(vzes::Message *msg) {
  switch (msg->message_id) {
  case CACHED_ADD: {
    StanzaMessageData *stanza_msg =
      static_cast<StanzaMessageData *>(msg->pdata.get());
    if (stanza_msg) {
      uint64 nBng = vzes::Time();
      int res = OnAsyncSaveFile(stanza_msg->stanza);
      if (res < 0) {
        // 存储失败删除文件
        LOG(L_ERROR) << "Filecache save file "
                     << stanza_msg->stanza->path()
                     << " failed.";
        remove(stanza_msg->stanza->path().c_str());
      }

      static int nWriteSlowTimes = 0;
      // 计算每32K写入速度
      uint64 nUseTime = vzes::Time() - nBng;
      const size_t PAG_SIZE = 32 * 1024;
      size_t nFileSize = stanza_msg->stanza->size();
      int nPag = nFileSize / PAG_SIZE + (((nFileSize % PAG_SIZE) > 0) ? 1 : 0);
      int nPerPagUseTime = nUseTime / nPag;
      LOG(L_INFO) << "use time " << nUseTime
                  << ", pag number " << nPag
                  << ", per pag use time " << nPerPagUseTime
                  << ", write slow times " << nWriteSlowTimes;
      //LOG_INFO("use time %lld, pag number %d, per pag use time %d; write slow times %d.",
      //         nUseTime, nPag, nPerPagUseTime, nWriteSlowTimes);

      if ((-1 == res) && (nPerPagUseTime > 200)) {
        // 打开文件失败(只读)
        // 32K数据需要200ms,大于此时间为慢速卡
        nWriteSlowTimes++;
        if (4 == nWriteSlowTimes) {
          // 两次 大图+小图存储失败或速度慢就umount存储分区
          //const char scmd[] = "{\"type\" : \"set_disk_umount\"}";
          //DpClient_SendDpMessage("SYS_DISK_DPJSON_REQ", 0, scmd, strlen(scmd));
        }
      } else {
        nWriteSlowTimes = 0;
      }
    }
    break;
  }

  case CACHED_CHECK: {
    CheckFileLimit();
    StartFileLimitCheckTimer();
    break;
  }

  default:
    break;
  }
}

void CachedService::CheckFileLimit() {
  // Check to reset stanza index
  if (current_stanzas_index_ >= flc_stanzas_.size()) {
    current_stanzas_index_ = 0;
  }

  FlcStanza &stanza = flc_stanzas_[current_stanzas_index_];
  current_stanzas_index_++;
  uint32 file_size = 0;

#ifndef WIN32
  struct stat statbuf;
  if (!stat(stanza.path.c_str(), &statbuf)) {
    file_size = statbuf.st_size;
  }
#endif
  LOG(L_INFO) << "The file info "
              << stanza.max_size << "\t"
              << file_size << "\t"
              << stanza.path;
  if (stanza.max_size < file_size) {
    remove(stanza.path.c_str());
  }
}

bool CachedService::InitFileLimitCheck() {
  // Open this file
  FILE * fp = fopen(FILE_LIMIT_CONFIG, "rb");
  if (fp == NULL) {
    LOG(L_ERROR) << "Failed to open " << FILE_LIMIT_CONFIG;
    return false;
  }

  do {
    // Read the file data
    char temp[128];
    std::string data;
    while(true) {
      std::size_t read_size = fread(temp, sizeof(char), 128, fp);
      if (read_size) {
        data.append(temp, read_size);
      } else {
        break;
      }
    }

    // Check the file data is empty
    if (data.empty()) {
      LOG(L_ERROR) << "The file is empty " << FILE_LIMIT_CONFIG;
      break;
    }

    // Parse the file data to flc_stanzas_
    Json::Reader reader;
    Json::Value  value;
    if (!reader.parse(data, value)) {
      LOG(L_ERROR) << "Parse the file data error "
                   << reader.getFormattedErrorMessages();
      break;
    }

    Json::Value limit_checks = value[JSON_FLC_LIMIT_CHECKS];
    vzes::FLASH_SIZE flash_type = vzes::FlashTotalSize();
    for (std::size_t i = 0; i < limit_checks.size(); i++) {
      if (limit_checks[i][JSON_FLC_PATH].isNull()
          || limit_checks[i][JSON_FLC_MAX_SIZE].isNull()) {
        LOG(L_ERROR) << "Failure Item";
        continue;
      }
      FlcStanza stanza;
      stanza.max_size   = limit_checks[i][JSON_FLC_MAX_SIZE].asUInt();
      stanza.path       = limit_checks[i][JSON_FLC_PATH].asString();
      if (flash_type == vzes::FLASH_128M && stanza.max_size > 0) {
        stanza.max_size /= 2;
      }
      LOG(L_INFO) << stanza.max_size << "\t" << stanza.path;
      flc_stanzas_.push_back(stanza);
    }

  } while(0);

  fclose(fp);
  if (flc_stanzas_.size()) {
    return true;
  }

  return false;
}

void CachedService::StartFileLimitCheckTimer() {
  event_service_->PostDelayed(ASYNC_TIMEOUT_TIMES * 1000, this, CACHED_CHECK);
}

bool CachedService::AddFile(CachedStanza::Ptr stanza, bool is_cached) {
  //LOG(L_INFO) << "Cached file = " << stanza->path()
  //            << " is cached " << is_cached
  //            << " stanza reference " <<stanza.use_count();
  ReplaceCachedFile(stanza);

  if(is_cached) {
    /* the number of caches reached the threshold, the current stanza is
     * not save to file.
     */
    if(cached_stanzas_.size() > (cache_size_ - 1)) {
      stanza->SaveConfimation();
      LOG(L_WARNING) << "cached stanzas reached the threshold " << (cache_size_ - 1)
                     << ", not save file " << stanza->path() << " to flash";
      return true;
    }

    StanzaMessageData *stanza_msg = new StanzaMessageData();
    stanza_msg->stanza = stanza;
    vzes::MessageData::Ptr msg_data(stanza_msg);
    event_service_->Post(this, CACHED_ADD, msg_data);
  } else {
    /* Set the "saved" flag, make sure this stanza can be recycled
     * in all scenarios.
     */
    stanza->SaveConfimation();
  }

  return true;
}

bool CachedService::SaveFile(const std::string path, MemBuffer::Ptr data) {
  RemoveOutOfDataStanza();
  CachedStanza::Ptr stanza = CachedStanzaPool::Instance()->TakeStanza();
  if (stanza) {
    stanza->SetPath(path.c_str());
    stanza->SetData(data);
    return AddFile(stanza, true);
  } else {
    return false;
  }
}

void CachedService::ReplaceCachedFile(CachedStanza::Ptr stanza) {
  // 去掉重复的元素
  for(std::size_t i = 0; i < cached_stanzas_.size(); i++) {
    if(cached_stanzas_[i]->path() == stanza->path()) {
      cached_stanzas_.erase(cached_stanzas_.begin() + i);
    }
  }
  cached_stanzas_.push_back(stanza);
}

bool CachedService::RemoveFile(const std::string path) {
  //LOG(L_INFO) << "Remove file " << path;
  BOOST_ASSERT(cachedstanza_pool_ != NULL);
  OnAsyncRemoveFile(path);

  for(std::deque<CachedStanza::Ptr>::iterator iter = cached_stanzas_.begin();
      iter != cached_stanzas_.end(); ++iter) {
    if((*iter)->path() == path) {
      if (((*iter)->IsSaved()) && ((*iter).use_count() == 1)) {
        cached_stanzas_.erase(iter);
      }
      break;
    }
  }
  return true;
}

void CachedService::OnAsyncRemoveFile(std::string path) {
  remove(path.c_str());
}

MemBuffer::Ptr CachedService::GetFile(const std::string path) {
  for(std::deque<CachedStanza::Ptr>::iterator iter = cached_stanzas_.begin();
      iter != cached_stanzas_.end(); ++iter) {
    if((*iter)->path() == path) {
      //LOG(L_INFO) << "Found cached file " << (*iter)->path();
      return (*iter)->data();
    }
  }

  BOOST_ASSERT(cachedstanza_pool_ != NULL);
  LOG(L_INFO) << "Can't find the file " << path
              << " in cache, read it from flash";
  RemoveOutOfDataStanza();
  CachedStanza::Ptr stanza = cachedstanza_pool_->TakeStanza();
  if(!stanza) {
    LOG(L_INFO) << "Get file failed: " << path;
    return MemBuffer::Ptr();
  }

  MemBuffer::Ptr data_buff = stanza->data();
  stanza->SetPath(path.c_str());
  if(ReadFile(stanza->path(), data_buff)) {
    /* 存入缓存。注意: stanza的生命周期必须和MemBuffer一致，
     * 即Stanza和MemBuffer一一对应，否则会导致实际分配的MemBuffer
     * 数量大于Stanza的数量(即缓存的数量).
     */
    AddFile(stanza, false);
    LOG(L_INFO) << "read file from flash successed: " << path;
    return data_buff;
  } else {
  }

  LOG(L_INFO) << "Get file failed: " << path;
  return MemBuffer::Ptr();
}

#ifndef WIN32
void CachedService::iMakeDirRecursive(const char *pPath) {

  if(pPath == NULL) {
    return;
  }

  int i,j;
  const char *p = pPath;
  char buf[512] = {0};
  int len = strlen(pPath);

  //去掉连续的 '/'
  for(i = 0,j = 0; i < len && j < 512; i++,j++) {
    buf[j] = p[i];
    if(p[i] == '/') {
      while(p[i] == '/') {
        i++;
      }
      j++;
      buf[j] = p[i];
    }
  }

  char *q = buf;
  char *pQ;
  //递归创建文件夹
  while((pQ = strchr(q, '/')) != NULL) {
    int rt;
    struct stat st;

    *pQ = 0; //每次创建一级目录，结束符
    rt = stat(buf, &st);
    if(!(rt >= 0 && S_ISDIR(st.st_mode))) { //如果不是目录则创建，是则往下搜索
      mkdir(buf, 0777);
    }
    q = pQ + 1;
    *pQ = '/'; //恢复
  }
}
#endif

int CachedService::OnAsyncSaveFile(CachedStanza::Ptr stanza) {
  BOOST_ASSERT(stanza && (!stanza->IsSaved()));
  static const uint32 RECURSION_TIME = 128;
  static const uint32 MAX_TRY_TIMES  = 4;
  int res = -1;

#ifndef WIN32
  iMakeDirRecursive(stanza->path().c_str());
#endif
  FILE *fp = fopen(stanza->path().c_str(), "wb");
  if(fp == NULL) {
    LOG(L_ERROR) << "Failure to open file " << stanza->path();
    stanza->SaveConfimation();
    return res;
  }

  uint32 recursion_time = RECURSION_TIME;
  if (stanza->size() <= 0) {
    stanza->SaveConfimation();
    fclose(fp);
    return res;
  }

  std::size_t write_size = 0;
  std::size_t try_write_times = MAX_TRY_TIMES;
  vzes::BlocksPtr &block_list = stanza->data()->blocks();

  for (vzes::BlocksPtr::iterator iter = block_list.begin();
       iter != block_list.end(); iter++) {
    vzes::Block::Ptr block = *iter;
    const char *pdata = (const char*)block->buffer;
    std::size_t res = 0;

    res = fwrite(pdata, 1, block->buffer_size, fp);
    //LOG(INFO) << "res = " << res
    //		  << "\t write size = "
    //		  << write_size
    //		  << "\t" << stanza->size();
    if (ferror(fp) && try_write_times) {
      LOG(L_ERROR) << "Write file error: size != stanza->data.size() "
                   << ferror(fp);
      perror("Failure to write file");
      LOG(L_ERROR) << "Try times " << MAX_TRY_TIMES - try_write_times;
      clearerr(fp);
      break;
    }

    if(res == 0) {
      break;
    } else {
#ifndef WIN32
      // Mandatory file synchronization
      // fsync(fileno(fp));
#endif
    }

    write_size += res;
    if(recursion_time) {
      vzsleep(recursion_time);
      recursion_time = recursion_time >> 1;
    }
  }

  if(write_size != stanza->size()) {
    LOG(L_ERROR) << "Write file error: size != stanza->data.size() "
                 << ferror(fp);
    perror("Failure to write file");
    res = -2;
  } else {
    LOG(L_INFO) << "save cached file " << stanza->path()
                << ", stanze reference " << stanza.use_count();
    res = write_size;
  }

  stanza->SaveConfimation();
  LOG(L_WARNING) << "save file size " << write_size;
  fclose(fp);
  return res;
}

bool CachedService::ReadFile(const std::string path,
                             MemBuffer::Ptr data_buffer) {
  static char read_buffer[READ_FILE_BUFFER_SIZE];
  FILE *fp = fopen(path.c_str(), "rb");
  if(fp == NULL) {
    LOG(L_ERROR) << "Failure to open the file " << path;
    return false;
  }

  while(!feof(fp)) {
    int read_size = fread((void *)read_buffer, 1, READ_FILE_BUFFER_SIZE, fp);
    if(read_size > 0) {
      data_buffer->WriteBytes(read_buffer, read_size);
    } else {
      break;
    }
  }

  LOG(L_WARNING) << "file data size " << data_buffer->size();
  fclose(fp);
  return true;
}

void CachedService::RemoveOutOfDataStanza() {
  BOOST_ASSERT(cachedstanza_pool_ != NULL);
  /*while(cached_stanzas_.size() > cache_size_) {
    CachedStanza::Ptr stanza = cached_stanzas_.front();
    if(stanza && stanza->IsSaved()) {
      cached_stanzas_.pop_front();
    } else {
      // break;
      vsleep(1);
    }
  }*/

  if (cached_stanzas_.size() < cache_size_) {
    return;
  }

  LOG(L_INFO) << "Cached stanzas reached the cache capacity, recycle stanzas"
              << ", cached stanza = " << cached_stanzas_.size()
              << ", cache capacity " << cache_size_;

  std::deque<CachedStanza::Ptr>::iterator iter;
  for (iter = cached_stanzas_.begin();
       iter != cached_stanzas_.end(); ++iter) {
    /* stanza可以被释放的条件：
     * 1.文件已经被写入flash
     * 2.当前没有用户使用该文件，即MemBuffer::Ptr的引用计数等于1(iterator引用不会+1)
     */
    if (((*iter)->IsSaved()) && ((*iter).use_count() == 1)) {
      LOG(L_INFO) << "Recycle not in used stanza";
      cached_stanzas_.erase(iter);
      break;
    }
  }
}

}
