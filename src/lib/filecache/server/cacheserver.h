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

#ifndef FILECHACHE_SERVER_CACHESERVER_H_
#define FILECHACHE_SERVER_CACHESERVER_H_

#include <string>
#include <map>

#include "filecache/base/basedefine.h"
#include "filecache/server/cacheservice.h"
#include "eventservice/event/signalevent.h"
#include "eventservice/net/eventservice.h"
#include "filecache/server/kvdbservice.h"

namespace cache {

/**< filecache消息结构体，用于Client和Server之间通信 */
struct CacheData : public vzes::MessageData {
  typedef boost::shared_ptr<CacheData> Ptr;
  CacheData() {
    buffer = MemBuffer::CreateMemBuffer();
  }
  std::string             path;         /**< file directory */
  MemBuffer::Ptr          buffer;       /**< file content buffer for read\write operations */
  vzes::SignalEvent::Ptr  signal_event; /**< client、server线程同步信号 */
  int                     res;          /**< 响应结果 */
};

/**< kvdb消息结构体，用于Client和Server之间通信 */
struct KvdbData : public vzes::MessageData {
  typedef boost::shared_ptr<KvdbData> Ptr;
  std::string             name;          /**< KvdbClient name */
  std::string             key;           /**< key */
  std::string             value;         /**< value */
  vzes::SignalEvent::Ptr  signal_event;  /**< client、server线程同步信号 */
  int                     res;           /**< 响应结果 */
};


class CacheServer : public boost::noncopyable,
  public boost::enable_shared_from_this<CacheServer>,
  public sigslot::has_slots<>,
  public vzes::MessageHandler {
 public:
  typedef boost::shared_ptr<CacheServer> Ptr;
  static CacheServer *Instance();
 public:
  virtual ~CacheServer();
  void InitCacheService();
  /**< 这个函数是线程安全的，你可以在任何线程中调用这个函数 */
  void AsyncWrite(CacheData::Ptr cache_data);
  /**< 这个函数是线程安全的，你可以在任何线程中调用这个函数 */
  void AsyncRead(CacheData::Ptr cache_data);
  /**< 这个函数是线程安全的，你可以在任何线程中调用这个函数 */
  void AsyncDelete(CacheData::Ptr cache_data);
  //
  void KVDBSetKey(KvdbData::Ptr kvdb_data);
  void KVDBGetKey(KvdbData::Ptr kvdb_data);
  void KVDBDeleteKey(KvdbData::Ptr kvdb_data);
  void KVDBBackUp(KvdbData::Ptr kvdb_data);
  void KVDBRestore(KvdbData::Ptr kvdb_data);
 private:
  virtual void OnMessage(vzes::Message *msg);
  //
  void OnWriteEvent(CacheData::Ptr cache_data);
  void OnReadEvent(CacheData::Ptr cache_data);
  void OnDeleteEvent(CacheData::Ptr cache_data);
  //
  void OnKVDBSet(KvdbData::Ptr kvdb_data);
  void OnKVDBGet(KvdbData::Ptr kvdb_data);
  void OnKVDBDelete(KvdbData::Ptr kvdb_data);
  void OnKVDBBackup(KvdbData::Ptr kvdb_data);
  void OnKVDBRestore(KvdbData::Ptr kvdb_data);
  KvdbService *GetKvdb(std::string &name);
 private:
  CacheServer();
 private:
  static CacheServer       *instance_;  /** server单实例对象 */
  CachedService::Ptr        cache_service_;  /** cache service对象 */
  vzes::EventService::Ptr   event_service_;  /** Server线程对象 */
  typedef std::map<std::string, KvdbService *> KVDBs;  /** KVDB 底层操作类map */
  KVDBs                     kvdbs_;
};

}

#endif  // FILECHACHE_SERVER_CACHESERVER_H_