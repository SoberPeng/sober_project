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

#include "filecache/cache/cacheclient.h"
#include "filecache/server/cacheserver.h"

namespace cache {

class CacheClientImpl : public CacheClient,
  public boost::noncopyable,
  public boost::enable_shared_from_this<CacheClientImpl>,
  public sigslot::has_slots<> {
 public:
  CacheClientImpl() {
    cache_service_ = CacheServer::Instance();
    ASSERT(cache_service_ != NULL);
    signal_event_ = vzes::SignalEvent::CreateSignalEvent();
  }

  virtual ~CacheClientImpl() {
  }

  virtual bool Write(const char *path, const char *data, int data_size) {

    vzes::CritScope cr(&crit_);

    CacheData::Ptr cache_data(new CacheData());
    cache_data->buffer->WriteBytes(data, data_size);
    cache_data->path.append(path, strlen(path));
    cache_data->signal_event  = vzes::SignalEvent::Ptr();
    cache_data->res           = CACHE_ERROR_TIMEOUT;

    cache_service_->AsyncWrite(cache_data);

	
    return CACHED_SUCCEED;
  }

  virtual MemBuffer::Ptr Read(const char *path) {
    vzes::CritScope cr(&crit_);

    CacheData::Ptr cache_data(new CacheData());
    cache_data->buffer = MemBuffer::Ptr();
    cache_data->path.append(path, strlen(path));
    cache_data->signal_event  = signal_event_;
    cache_data->res           = CACHE_ERROR_TIMEOUT;

    cache_service_->AsyncRead(cache_data);

    int res = signal_event_->WaitSignal(DEFAULT_REQ_TIMEOUT);
    if (res != SIGNAL_EVENT_DONE) {
      return MemBuffer::Ptr();
    }
    if (cache_data->res != CACHE_DONE) {
      return MemBuffer::Ptr();
    }

    return cache_data->buffer;
  }

  virtual bool Delete(const char *path) {
    vzes::CritScope cr(&crit_);

    CacheData::Ptr cache_data(new CacheData());
    cache_data->path.append(path, strlen(path));
    cache_data->signal_event  = vzes::SignalEvent::Ptr();
    cache_data->res           = CACHE_ERROR_TIMEOUT;

    cache_service_->AsyncDelete(cache_data);
    return CACHED_SUCCEED;
  }

 private:
  vzes::SignalEvent::Ptr   signal_event_;  /**< client和server线程同步信号 */
  CacheServer             *cache_service_;  /**< Server对象 */
  vzes::CriticalSection    crit_;  /**< 互斥锁，保证线程安全 */
};

CacheClient::Ptr CacheClient::CreateCacheClient() {
  CacheClient::Ptr cc(new CacheClientImpl());
  return cc;
}

}

