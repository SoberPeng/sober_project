/*
 * vzsdk
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
#include <iostream>
#include "app/app/app.h"
#include "filecache/kvdb/kvdbclient.h"
#include "filecache/cache/cacheclient.h"
#include "filecache/kvdb/kvdbclient_c.h"
#include "app/app/appstarup.h"


#define KVDB_KEY_HELLO     "Hello"
#define KVDB_VALUE_HELLO   "Hello, this is kvdb test case!"

#ifdef WIN32
#define vzsleep(x)         Sleep(x)
#define FC_FILE_PATH_1     "E:/kvdb/filecache_test_1.txt"
#define FC_FILE_PATH_2     "E:/kvdb/filecache_test_2.txt"
#define FC_FILE_PATH_3     "E:/kvdb/filecache_test_3.txt"
#define FC_FILE_PATH_4     "E:/kvdb/filecache_test_4.txt"
#define FC_FILE_PATH_5     "E:/kvdb/filecache_test_5.txt"
#define FC_FILE_PATH_6     "E:/kvdb/filecache_test_6.txt"
#else
#define vzsleep(x)         usleep(x * 1000);
#define FC_FILE_PATH_1     "/tmp/app/exec/filecache_test_1.txt"
#define FC_FILE_PATH_2     "/tmp/app/exec/filecache_test_2.txt"
#define FC_FILE_PATH_3     "/tmp/app/exec/filecache_test_3.txt"
#define FC_FILE_PATH_4     "/tmp/app/exec/filecache_test_4.txt"
#define FC_FILE_PATH_5     "/tmp/app/exec/filecache_test_5.txt"
#define FC_FILE_PATH_6     "/tmp/app/exec/filecache_test_6.txt"
#endif

char fc_content[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

class FileCacheApp : public app::AppInterface,
  public boost::noncopyable,
  public boost::enable_shared_from_this<FileCacheApp>,
  public sigslot::has_slots<> {
 public:
  FileCacheApp() :AppInterface("FileCacheApp") {
  }
  virtual ~FileCacheApp() {
  }

  void kvdb_test_case_1() {
    LOG(L_INFO) << ">>> kvdb test case 1 start";
    std::string value;
    kvdb_client_->SetKey(KVDB_KEY_HELLO,
                         KVDB_VALUE_HELLO,
                         strlen(KVDB_VALUE_HELLO));
    kvdb_client_->GetKey(KVDB_KEY_HELLO, &value);
    if (!strcmp((const char*)KVDB_VALUE_HELLO, (const char*)value.c_str())) {
      LOG(L_INFO) << "kvdb test case 1 successed ^_^";
    } else {
      LOG(L_INFO) << "kvdb test case 1 failed -_-"
                  << ", set value: " << (const char*)KVDB_VALUE_HELLO
                  << ", get value: " << value;
    }

    LOG(L_INFO) << ">>> kvdb test case 1 end";
  }

  void kvdb_test_case_2() {
    // C接口测试
    LOG(L_INFO) << ">>> kvdb test case 2 start";
    bool successd = false;

    do {
      KVDB_HANDLE kvdb_c = Kvdb_Create("ckvdb");
      if (NULL == kvdb_c) {
        LOG(L_INFO) << "create kvdb instance failed!!!";
        break;
      }

      bool result = Kvdb_SetKey(kvdb_c, KVDB_KEY_HELLO, strlen(KVDB_KEY_HELLO),
                                KVDB_VALUE_HELLO, strlen(KVDB_VALUE_HELLO));
      if (!result) {
        LOG(L_INFO) << "set key failed!!!";
        break;
      }

      char buffer[1024] = {0};
      result = Kvdb_GetKey(kvdb_c, KVDB_KEY_HELLO, strlen(KVDB_KEY_HELLO),
                           buffer, 1024);
      if (!result) {
        LOG(L_INFO) << "get key failed!!!";
        break;
      }

      if (strcmp((const char*)KVDB_VALUE_HELLO, buffer)) {
        LOG(L_INFO) << "key value incorrect"
                    << ", set value: " << (const char*)KVDB_VALUE_HELLO
                    << ", get value: " << buffer;
        break;
      }

      successd = true;
    } while (0);

    if (successd) {
      LOG(L_INFO) << "kvdb test case 2 successed ^_^";
    } else {
      LOG(L_INFO) << "kvdb test case 2 failed -_-";
    }

    LOG(L_INFO) << ">>> kvdb test case 2 end";
  }

  void kvdb_test_case_3() {
    // C接口测试
    LOG(L_INFO) << ">>> kvdb test case 3 start";
    bool successd = false;

    do {
      KVDB_HANDLE kvdb_c = Kvdb_Create("ckvdb");
      if (NULL == kvdb_c) {
        LOG(L_INFO) << "create kvdb instance failed!!!";
        break;
      }

      bool result = Kvdb_SetKey(kvdb_c, KVDB_KEY_HELLO, strlen(KVDB_KEY_HELLO),
                                KVDB_VALUE_HELLO, strlen(KVDB_VALUE_HELLO));
      if (!result) {
        LOG(L_INFO) << "set key failed!!!";
        break;
      }

      result = Kvdb_DeleteKey(kvdb_c, KVDB_KEY_HELLO, strlen(KVDB_KEY_HELLO));
      if (!result) {
        LOG(L_INFO) << "delete key failed!!!";
        break;
      }

      char buffer[1024] = {0};
      result = Kvdb_GetKey(kvdb_c, KVDB_KEY_HELLO, strlen(KVDB_KEY_HELLO),
                           buffer, 1024);
      if (result) {
        LOG(L_INFO) << "get key failed, this key is deleted!!!";
        break;
      }

      successd = true;
    } while (0);

    if (successd) {
      LOG(L_INFO) << "kvdb test case 3 successed ^_^";
    } else {
      LOG(L_INFO) << "kvdb test case 3 failed -_-";
    }

    LOG(L_INFO) << ">>> kvdb test case 3 end";
  }

  void kvdb_test_case_4() {
    LOG(L_INFO) << ">>> kvdb test case 4 start";
    cache::KvdbClient::Ptr k1 = cache::KvdbClient::CreateKvdbClient("tkvdb1");
    cache::KvdbClient::Ptr k2 = cache::KvdbClient::CreateKvdbClient("tkvdb2");
	
    char key[64];
    char w_value[1024];
    std::string r_value;
    int i = 0;

    for(int i = 0; i < 3; i++){
      sprintf(key, "t1kkk%d", i+1);
      sprintf(w_value, "t1vvv%d", i+1);
      k1->SetKey(key, strlen(key), w_value, strlen(w_value));

      k1->GetKey(key, &r_value);
      if (!strcmp(w_value, (const char*)r_value.c_str())) {
        LOG(L_INFO) << "kvdb1 test case 4 successed ^_^";
      } else {
        LOG(L_INFO) << "kvdb1 test case 4 failed -_-"
                    << ", set value: " << w_value
                    << ", get value: " << r_value;
      }
    }

    for(int i = 0; i < 3; i++){
      sprintf(key, "t2kkk%d", i+1);
      sprintf(w_value, "t2vvv%d", i+1);
      k2->SetKey(key, strlen(key), w_value, strlen(w_value));

      k2->GetKey(key, &r_value);
      if (!strcmp(w_value, (const char*)r_value.c_str())) {
        LOG(L_INFO) << "kvdb2 test case 4 successed ^_^";
      } else {
        LOG(L_INFO) << "kvdb2 test case 4 failed -_-"
                    << ", set value: " << w_value
                    << ", get value: " << r_value;
      }
    }

    if (k1->BackupDatabase()) {
      LOG(L_INFO) << "BackupDatabase successed ^_^";
    }
    else {
      LOG(L_INFO) << "BackupDatabase failed -_-";
    }
    if (k1->RestoreDatabase()) {
      LOG(L_INFO) << "RestoreDatabase successed ^_^";
    }
    else {
      LOG(L_INFO) << "RestoreDatabase failed -_-";
    }

    LOG(L_INFO) << ">>> kvdb test case 4 end";
  }

  void filecache_test_delete_all() {
    cache_client_->Delete(FC_FILE_PATH_1);
    cache_client_->Delete(FC_FILE_PATH_2);
    cache_client_->Delete(FC_FILE_PATH_3);
    cache_client_->Delete(FC_FILE_PATH_4);
    cache_client_->Delete(FC_FILE_PATH_5);
    cache_client_->Delete(FC_FILE_PATH_6);
  }

  void filecache_test_case_1() {
    LOG(L_INFO) << ">>> filecache test case 1 start";
    //LOG(L_INFO) << "write data: ";
    //LOG(L_INFO).write(fc_content, strlen(fc_content));

    cache_client_->Write(FC_FILE_PATH_1, fc_content, strlen(fc_content));

    vzes::MemBuffer::Ptr read_buffer;
    read_buffer = cache_client_->Read(FC_FILE_PATH_1);
    vzes::BlocksPtr &blocks = read_buffer->blocks();
    vzes::Block::Ptr data_block = blocks.front();
    //LOG(L_INFO) << "read data: ";
    //LOG(L_INFO).write((const char*)data_block->buffer, data_block->buffer_size);
    if (!strncmp((const char*)fc_content, (const char*)data_block->buffer,
                 data_block->buffer_size)) {
      LOG(L_INFO) << "filecache test case 1 successed ^_^";
    } else {
      LOG(L_INFO) << "filecache test case 1 failed -_-"
                  << ", set value: " << (const char*)fc_content
                  << ", get value: " << data_block->buffer;
    }

    LOG(L_INFO) << "MemBuffer reference = " << read_buffer.use_count();
    LOG(L_INFO) << ">>> filecache test case 1 end";
  }


  void filecache_test_case_2() {
    LOG(L_INFO) << ">>> filecache test case 2 start";
    filecache_test_delete_all();

    cache_client_->Write(FC_FILE_PATH_1, fc_content, strlen(fc_content));
    cache_client_->Write(FC_FILE_PATH_2, fc_content, strlen(fc_content));
    cache_client_->Write(FC_FILE_PATH_3, fc_content, strlen(fc_content));
    cache_client_->Write(FC_FILE_PATH_4, fc_content, strlen(fc_content));
    cache_client_->Write(FC_FILE_PATH_5, fc_content, strlen(fc_content));
    cache_client_->Write(FC_FILE_PATH_6, fc_content, strlen(fc_content));

    /* 1-3 in cache and flash; 4-5 not in cache and flash; 6 in cache. */
    vzes::MemBuffer::Ptr read_buffer;
    read_buffer = cache_client_->Read(FC_FILE_PATH_6);
    if (!read_buffer) {
      LOG(L_INFO) << "filecache test case 2 failed -_-";
    } else {
      read_buffer = cache_client_->Read(FC_FILE_PATH_5);
      if (read_buffer) {
        LOG(L_INFO) << "filecache test case 2 failed -_-";
      } else {
        read_buffer = cache_client_->Read(FC_FILE_PATH_2);
        if (!read_buffer) {
          LOG(L_INFO) << "filecache test case 2 failed -_-";
        } else {
          LOG(L_INFO) << "MemBuffer reference = " << read_buffer.use_count();
          LOG(L_INFO) << "filecache test case 2 successed ^_^";
        }
      }
    }

    LOG(L_INFO) << ">>> filecache test case 2 end";
  }

  void filecache_test_case_3() {
    LOG(L_INFO) << ">>> filecache test case 3 start";
    filecache_test_delete_all();

    cache_client_->Write(FC_FILE_PATH_1, fc_content, strlen(fc_content));
    cache_client_->Write(FC_FILE_PATH_2, fc_content, strlen(fc_content));
    cache_client_->Write(FC_FILE_PATH_3, fc_content, strlen(fc_content));
    cache_client_->Write(FC_FILE_PATH_4, fc_content, strlen(fc_content));

    vzsleep(3000); /* make sure file 1-4 have been writen to flash */
    cache_client_->Write(FC_FILE_PATH_5, fc_content, strlen(fc_content));
    cache_client_->Write(FC_FILE_PATH_6, fc_content, strlen(fc_content));

    vzes::MemBuffer::Ptr read_buffer_1;
    read_buffer_1 = cache_client_->Read(FC_FILE_PATH_5);
    if (!read_buffer_1) {
      LOG(L_INFO) << "filecache test case 3 failed -_-";
    } else {
      vzes::MemBuffer::Ptr read_buffer_2;
      read_buffer_2 = cache_client_->Read(FC_FILE_PATH_6);
      if (!read_buffer_2) {
        LOG(L_INFO) << "filecache test case 3 failed -_-";
      } else {
        vzes::MemBuffer::Ptr read_buffer_3;
        read_buffer_3 = cache_client_->Read(FC_FILE_PATH_5);
        if (!read_buffer_3) {
          LOG(L_INFO) << "filecache test case 3 failed -_-";
        } else {
          LOG(L_INFO) << "filecache test case 3 successed ^_^";
        }
      }
    }

    LOG(L_INFO) << ">>> filecache test case 3 end";
  }

  void filecache_test_case_4() {
    LOG(L_INFO) << ">>> filecache test case 4 start";
    bool successed = true;
    filecache_test_delete_all();

    cache_client_->Write(FC_FILE_PATH_1, fc_content, strlen(fc_content));
    cache_client_->Write(FC_FILE_PATH_2, fc_content, strlen(fc_content));
    cache_client_->Write(FC_FILE_PATH_3, fc_content, strlen(fc_content));
    cache_client_->Write(FC_FILE_PATH_4, fc_content, strlen(fc_content));
    vzsleep(3000); /* make sure file 1-4 have been writen to flash */

    {
      vzes::MemBuffer::Ptr read_buffer_1;
      read_buffer_1 = cache_client_->Read(FC_FILE_PATH_1);
      LOG(L_INFO) << "MemBuffer reference = " << read_buffer_1.use_count();
      if (read_buffer_1.use_count() != 2) {
        LOG(L_INFO) << "filecache test case 4 failed -_-";
        successed = false;
      }
    }

    {
      vzes::MemBuffer::Ptr read_buffer_2;
      read_buffer_2 = cache_client_->Read(FC_FILE_PATH_1);
      LOG(L_INFO) << "MemBuffer reference = " << read_buffer_2.use_count();
      if (read_buffer_2.use_count() != 2) {
        LOG(L_INFO) << "filecache test case 4 failed -_-";
        successed = false;
      }
    }

    {
      vzes::MemBuffer::Ptr read_buffer_1;
      read_buffer_1 = cache_client_->Read(FC_FILE_PATH_1);
      LOG(L_INFO) << "MemBuffer reference = " << read_buffer_1.use_count();
      if (read_buffer_1.use_count() != 2) {
        LOG(L_INFO) << "filecache test case 4 failed -_-";
        successed = false;
      }

      vzes::MemBuffer::Ptr read_buffer_2;
      read_buffer_2 = cache_client_->Read(FC_FILE_PATH_1);
      LOG(L_INFO) << "MemBuffer reference = " << read_buffer_2.use_count();
      if (read_buffer_2.use_count() != 3) {
        LOG(L_INFO) << "filecache test case 4 failed -_-";
        successed = false;
      }
    }

    {
      vzes::MemBuffer::Ptr read_buffer_1;
      read_buffer_1 = cache_client_->Read(FC_FILE_PATH_1);
      LOG(L_INFO) << "MemBuffer reference = " << read_buffer_1.use_count();
      if (read_buffer_1.use_count() != 2) {
        LOG(L_INFO) << "filecache test case 4 failed -_-";
        successed = false;
      }
    }

    if (successed) {
      LOG(L_INFO) << "filecache test case 4 successed ^_^";
    }

    LOG(L_INFO) << ">>> filecache test case 4 end";
  }

  //////////////////////////////////////////////////////////////////////////////
  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    event_service_ = event_service;
    kvdb_client_  = cache::KvdbClient::CreateKvdbClient("tkvdb");
    cache_client_ = cache::CacheClient::CreateCacheClient();
    return true;
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    return true;
  }

  virtual bool RunAPP(vzes::EventService::Ptr event_service) {
    // KVDB TEST
    kvdb_test_case_1();
    kvdb_test_case_2();
    kvdb_test_case_3();
    kvdb_test_case_4();
    vzsleep(1000);

    // FileCache TEST
    //filecache_test_case_1();
    //vzsleep(1000);
    //filecache_test_case_2();
    //vzsleep(1000);
    //filecache_test_case_3();
    //vzsleep(1000);
    //filecache_test_case_4();
    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }

 private:
  vzes::EventService::Ptr event_service_;
  cache::CacheClient::Ptr cache_client_;
  cache::KvdbClient::Ptr  kvdb_client_;
};

int main(void) {
  app::App::Ptr app = app::App::CreateApp();
  app::AppInterface::Ptr filecache(new FileCacheApp());
  app->RegisterApp(filecache);
  app->AppRun();
  app->ExitApp();
  return EXIT_SUCCESS;
}
