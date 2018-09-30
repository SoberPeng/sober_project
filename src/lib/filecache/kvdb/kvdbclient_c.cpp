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
#include <stdio.h>
#include <string.h>

#include "filecache/kvdb/kvdbclient_c.h"
#include "filecache/kvdb/kvdbclient.h"


/**
 * kvdb client C实现实例结构体
 */
typedef struct {
  cache::KvdbClient::Ptr kvdb_client_; /**< kvdb client C++对象指针 */
} kvdb_client_entity;


KVDB_HANDLE Kvdb_Create(const char *name) {
  kvdb_client_entity *
  kvdb_entity = new kvdb_client_entity;
  if (NULL == kvdb_entity) {
    return NULL;
  }

  std::string skey(name, strlen(name));
  kvdb_entity->kvdb_client_ = cache::KvdbClient::CreateKvdbClient(skey);
  return (KVDB_HANDLE)kvdb_entity;
}

void Kvdb_Destory(KVDB_HANDLE hKvdb) {
  kvdb_client_entity *
  kvdb_entity = (kvdb_client_entity*)hKvdb;
  if (NULL == kvdb_entity) {
    return;
  }

  delete kvdb_entity;
}

bool Kvdb_SetKey(KVDB_HANDLE hKvdb, const char *key, uint8 key_size,
                 const char *value, uint32 value_size) {
  kvdb_client_entity *
  kvdb_entity = (kvdb_client_entity*)hKvdb;
  if (NULL == kvdb_entity) {
    return KVDB_FAILURE;
  }

  return kvdb_entity->kvdb_client_->SetKey(key, key_size, value, value_size);
}

bool Kvdb_GetKey(KVDB_HANDLE hKvdb, const char *key, uint8 key_size,
                 void *buffer, uint32 buffer_size) {
  kvdb_client_entity *
  kvdb_entity = (kvdb_client_entity*)hKvdb;
  if (NULL == kvdb_entity) {
    return KVDB_FAILURE;
  }

  return kvdb_entity->kvdb_client_->GetKey(key, key_size, buffer, buffer_size);
}

bool Kvdb_DeleteKey(KVDB_HANDLE hKvdb, const char *key, uint8 key_size) {
  kvdb_client_entity *
  kvdb_entity = (kvdb_client_entity*)hKvdb;
  if (NULL == kvdb_entity) {
    return KVDB_FAILURE;
  }

  return kvdb_entity->kvdb_client_->DeleteKey(key, key_size);
}

bool Kvdb_BackupDatabase(KVDB_HANDLE hKvdb) {
  kvdb_client_entity *
  kvdb_entity = (kvdb_client_entity*)hKvdb;
  if (NULL == kvdb_entity) {
    return KVDB_FAILURE;
  }

  return kvdb_entity->kvdb_client_->BackupDatabase();
}

bool Kvdb_RestoreDatabase(KVDB_HANDLE hKvdb) {
  kvdb_client_entity *
  kvdb_entity = (kvdb_client_entity*)hKvdb;
  if (NULL == kvdb_entity) {
    return KVDB_FAILURE;
  }

  return kvdb_entity->kvdb_client_->RestoreDatabase();
}

