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

#ifndef FILECHACHE_KVDB_KVDBCLIENT_H_
#define FILECHACHE_KVDB_KVDBCLIENT_H_

#include "filecache/base/basedefine.h"

namespace cache {

#define KVDB_NAME_KVDB     "kvdb"
#define KVDB_NAME_SKVDB    "skvdb"
#define KVDB_NAME_USRKVDB  "usr_data_kvdb"

#define KVDB_SUCCEED       true
#define KVDB_FAILURE       false

class KvdbClient {
 public:
  typedef boost::shared_ptr<KvdbClient> Ptr;

  static KvdbClient::Ptr CreateKvdbClient(const std::string& name);

  virtual bool SetKey(const char *key, uint8 key_size,
                      const char *value, uint32 value_size) = 0;
  virtual bool SetKey(const std::string key,
                      const char *value, uint32 value_size) = 0;

  virtual bool GetKey(const char *key, uint8 key_size,
                      std::string *result) = 0;
  virtual bool GetKey(const std::string key,
                      std::string *result) = 0;
  virtual bool GetKey(const std::string key,
                      void *buffer,
                      std::size_t buffer_size) = 0;
  virtual bool GetKey(const char *key, uint8 key_size,
                      void *buffer, std::size_t buffer_size) = 0;

  virtual bool DeleteKey(const char *key, uint8 key_size) = 0;

  virtual bool BackupDatabase() = 0;

  virtual bool RestoreDatabase() = 0;
};

}  // FILECHACHE_KVDB_KVDBCLIENT_H_

#endif  // FILECHACHE_KVDB_KVDBCLIENT_H_
