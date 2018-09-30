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


#ifndef FILECHACHE_CACHE_CACHECLIENT_H_
#define FILECHACHE_CACHE_CACHECLIENT_H_

#include "filecache/base/basedefine.h"

#define CACHED_SUCCEED true
#define CACHED_FAILURE false

namespace cache {

class CacheClient {
 public:
  typedef boost::shared_ptr<CacheClient> Ptr;

  /**
   * 创建一个新的CacheClient，每一个实例只需要一个
   *
   * @return 成功，CacheClient::Ptr；失败，NULL
   */
  static CacheClient::Ptr CreateCacheClient();

  /**
   * 通过Filecache存储数据到指定的路径、文件，该接口为异步接口，线程安全。
   *
   * @param path
   *   文件绝对路径，如, /tmp/app/exec/test.jpg
   * @param data
   *   文件内容
   * @param data_size
   *   文件内容长度，单位Byte
   * @return 成功，CACHED_SUCCEED；失败，CACHED_FAILURE
   */
  virtual bool Write(const char *path, const char *data, int data_size) = 0;

  /**
   * 读取文件，该接口为同步接口，线程安全。
   *
   * @param path
   *   文件绝对路径，如, /tmp/app/exec/test.jpg
   * @return 成功，MemBuffer::Ptr；失败，NULL
   */
  virtual MemBuffer::Ptr Read(const char *path) = 0;

  /**
  * 读取文件，该接口为异步接口，线程安全。
  *
  * @param path
  *   文件绝对路径，如, /tmp/app/exec/test.jpg
  * @return 成功，CACHED_SUCCEED；失败，CACHED_FAILURE
  */
  virtual bool Delete(const char *path) = 0;
};

}

#endif  // FILECHACHE_CACHE_CACHECLIENT_H_