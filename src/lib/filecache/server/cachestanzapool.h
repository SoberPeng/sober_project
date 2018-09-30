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


#ifndef FILECACHE_SERVER_CACHEDSTANZAPOOL_H_
#define FILECACHE_SERVER_CACHEDSTANZAPOOL_H_

#include <list>
#include <vector>
#include <string>

#include "boost/boost_settings.hpp"
#include "eventservice/base/basictypes.h"
#include "eventservice/base/criticalsection.h"
#include "eventservice/base/noncopyable.h"
#include "filecache/base/basedefine.h"

namespace cache {

class CachedStanza : public vzes::noncopyable,
  public boost::enable_shared_from_this<CachedStanza> {
 public:
  typedef boost::shared_ptr<CachedStanza> Ptr;

  CachedStanza();
  virtual ~CachedStanza();

  std::string &path() {
    return path_;
  }

  void SetPath(const char *path) {
    path_ = path;
  }

  MemBuffer::Ptr data() {
    return cache_data_;
  }

  void SetData(MemBuffer::Ptr data);

  bool IsSaved();
  void SaveConfimation();
  void ResetDefualtState();

  std::size_t size() {
    if (cache_data_) {
      return cache_data_->size();
    }

    return 0;
  }

  static uint32 stanza_count;
 private:
  std::string path_;
  MemBuffer::Ptr cache_data_;
  bool is_saved_;

  vzes::CriticalSection stanza_mutex_;
};

class CachedStanzaPool : public vzes::noncopyable,
  public boost::enable_shared_from_this<CachedStanzaPool> {
 public:
  typedef boost::shared_ptr<CachedStanzaPool> Ptr;
  CachedStanzaPool();
  virtual ~CachedStanzaPool();
  static CachedStanzaPool *Instance();
  // Thread safed
  CachedStanza::Ptr TakeStanza();
  // Thread safed
  void SetDefaultCachedSize(std::size_t stanza_size);
  std::size_t CachedStanzaSize() {
    return stanzas_.size();
  }
 private:
  static void RecyleBuffer(void *stanza);
  void RecyleStanza(CachedStanza *stanza);
 private:
  static const int MAX_CACHED_STANZA_SIZE = 128;
  typedef std::list<CachedStanza *> Stanzas;
  Stanzas stanzas_;
  vzes::CriticalSection pool_mutex_;
 private:
  static CachedStanzaPool *pool_instance_;
};
}

#endif // FILECACHE_SERVER_CACHEDSTANZAPOOL_H_
