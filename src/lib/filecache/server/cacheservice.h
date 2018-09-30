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

#ifndef FILE_CACHE_CONN_CACHED_SERVICES_H_
#define FILE_CACHE_CONN_CACHED_SERVICES_H_

#include <set>
#include <deque>
#include <vector>
#include "eventservice/base/noncopyable.h"
#include "eventservice/net/eventservice.h"
#include "filecache/server/cachestanzapool.h"
#include "filecache/base/basedefine.h"


namespace cache {

#define MAX_ROOT_PREFIX_SIZE 64

struct FlashCachedSettings {
  uint32_t max_image_size;
  char     root_prefix_[MAX_ROOT_PREFIX_SIZE];
};

struct FlcStanza {
  uint32 max_size;
  std::string path;
  FlcStanza &operator=(const FlcStanza &stanza) {
    max_size = stanza.max_size;
    path     = stanza.path;
    return *this;
  }
};

enum CachedMessage {
  CACHED_ADD,
  CACHED_CHECK
};

class CachedService : public vzes::noncopyable,
  public vzes::MessageHandler {
 public:
  typedef boost::shared_ptr<CachedService> Ptr;

 public:
  CachedService(vzes::EventService::Ptr es, std::size_t cache_size);
  virtual ~CachedService();

  bool Start();
  bool SaveFile(const std::string path, MemBuffer::Ptr data);
  MemBuffer::Ptr GetFile(const std::string path);
  bool RemoveFile(const std::string path);
  void RemoveOutOfDataStanza();

  virtual void OnMessage(vzes::Message *msg);

 private:
  void CheckFileLimit();
  bool InitFileLimitCheck();
  void StartFileLimitCheckTimer();

 private:
  int  OnAsyncSaveFile(CachedStanza::Ptr stanza);
  void OnAsyncRemoveFile(std::string path);
  void ReplaceCachedFile(CachedStanza::Ptr stanza);
  bool AddFile(CachedStanza::Ptr stanza, bool is_cached = true);
  bool ReadFile(const std::string path, MemBuffer::Ptr data_buffer);

#ifndef WIN32
  void iMakeDirRecursive(const char *pPath);
#endif

 private:
  struct StanzaMessageData : public vzes::MessageData {
    CachedStanza::Ptr stanza;
  };

 private:
  static const int     READ_FILE_BUFFER_SIZE  = (4096);
  static const int     ASYNC_TIMEOUT_TIMES    = (10);

  std::size_t          cache_size_;
  std::size_t          current_cache_size_;
  vzes::EventService::Ptr event_service_;
  std::deque<CachedStanza::Ptr> cached_stanzas_;
  CachedStanzaPool    *cachedstanza_pool_;
  FlashCachedSettings *flash_cached_settings_;

  std::vector<FlcStanza> flc_stanzas_;
  uint32                 current_stanzas_index_;
};
}

#endif // FILE_CACHE_CONN_CACHED_SERVICES_H_
