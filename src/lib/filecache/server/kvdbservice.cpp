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

#include "kvdbservice.h"
#include "eventservice/base/logging.h"
#include "eventservice/base/common.h"

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

#ifdef WIN32
#include <direct.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

namespace cache {

/** 路径缓存长度 */
#define KVDB_TMP_BUFFER_LEN      1024

KvdbService::KvdbService(const char *foldpath,
                         struct KvdbCache *cache,
                         int cache_cnt) {
  int i;
  int j;
  int counter;
  struct KvdbRecord *record;
  struct KvdbRecord *last_record;

  if (foldpath == NULL || strlen(foldpath) > KVDB_FILE_PATH_MAXLEN) {
#ifdef WIN32
    strcpy(foldpath_, "D:\\kvdb");
#else
#ifdef LITEOS
    strcpy(foldpath_, "/usr/kvdb");
#else
    strcpy(foldpath_, "/mnt/usr/kvdb");
#endif
#endif
    LOG(L_ERROR) << "Invalid foldpath: " << foldpath;
  } else {
    strcpy(foldpath_, foldpath);
  }
  // 创建文件夹
  FoldCreate(foldpath_);
  // 创建缓存
  for (i = 0; i < KVDB_CACHE_MAXNUM; i++) {
    cache_list_[i] = NULL;
  }

  if (cache_cnt < 0) {
    cache_cnt = 0;
  }
  if (cache_cnt > KVDB_CACHE_MAXNUM) {
    cache_cnt = KVDB_CACHE_MAXNUM;
  }
  cache_cnt_ = cache_cnt;
  if (cache_cnt_ > 0) {
    for (i = 0; i < cache_cnt_; i++) {
      if (cache[i].counter > KVDB_CACHE_EACH_MAXCNT) { // 最大缓存个数
        counter = KVDB_CACHE_EACH_MAXCNT;
      } else {
        counter = cache[i].counter;
      }
      cache_size_[i] = cache[i].size;
      // 分配节点
      last_record = NULL;
      for (j = 0; j < counter; j++) {
        record = (struct KvdbRecord *) malloc(sizeof(struct KvdbRecord));
        if (record == NULL) {
          LOG(L_ERROR) << "Malloc node";
          break;
        }
        record->value = (uint8 *) malloc(cache_size_[i] + 1);
        if (record->value == NULL) {
          LOG(L_ERROR) << "Malloc node";
          free(record);
          record = NULL;
          break;
        }
        record->status = 0;
        record->next = NULL;
        if (last_record == NULL) {
          cache_list_[i] = record;
          last_record = cache_list_[i];
        } else {
          last_record->next = record;
        }
        last_record = record;
      }
    }
  }
}

KvdbService::~KvdbService() {
}

struct KvdbRecord *KvdbService::NodeFind(const std::string &key) {
  int i;
  int j;
  struct KvdbRecord *record;

  for (i = 0; i < cache_cnt_; i++) {
    record = cache_list_[i];  // 指向缓存头结点
    for (j = 0; j < KVDB_CACHE_EACH_MAXCNT && record != NULL; j++) {
      if (record->status & KVDB_STATUS_BIT_VALID) {  // 有效数据
        if (!strcmp(key.c_str(), record->key)) {  // 找到节点
          return record;
        }
      }
      record = record->next;
    }
  }
  return NULL;
}

struct KvdbRecord *KvdbService::NodeOldest(int length) {
  int i;
  int j;
  struct KvdbRecord *record;
  struct KvdbRecord *record_oldest = NULL;
  time_t old_time = time(NULL);

  for (i = 0; i < cache_cnt_; i++) {
    if (cache_size_[i] >= length) {
      record = cache_list_[i];  // 指向缓存头结点
      for (j = 0; j < KVDB_CACHE_EACH_MAXCNT && record != NULL; j++) {
        if (record->status & KVDB_STATUS_BIT_VALID) {  // 有效数据
          if (record->time_last < old_time) {  // 找到节点
            old_time = record->time_last;
            record_oldest = record;
          }
        } else {  // 空闲节点
          return record;
        }
        record = record->next;
      }
    }
  }
  return record_oldest;
}

KvdbError KvdbService::Replace(const std::string &key,
                               const uint8 *value,
                               int length) {
  int i;
  struct KvdbRecord *record;
  uint8 buf[KVDB_TMP_BUFFER_LEN + 1];
  FILE *fp;
  size_t len;
  bool is_need_write_file = true;

  if (key.length() == 0 || key.length() > KVDB_KEY_MAXLEN) {
    LOG(L_ERROR) << "error key";
    return KVDB_ERR_INVALID_PARAM;
  }
  // 查询缓存
  record = NodeFind(key);
  if (record != NULL) {  // 找到同名节点
    if (length == record->length) {  // 长度相同
      if (!memcmp(value, record->value, length)) {  // 内容相同
        record->time_last = time(NULL);  // 更新访问时间
        return KVDB_ERR_OK;  // 与缓存数据相同，不做后续操作
      }
    }
  } else if (length > 0) {  // 没有缓存，读回文件进行比对
    MAKE_FILE_NAME((char *)buf, KVDB_TMP_BUFFER_LEN, foldpath_, key.c_str());
    fp = fopen((char *) buf, "rb");
    if (fp != NULL) {
      fseek(fp, 0, SEEK_END);
      int file_length = (int) ftell(fp);
      fseek(fp, 0, SEEK_SET);
      if (file_length == length) {  // 内容一样长
        is_need_write_file = false;
        while ((len = fread(buf, 1, KVDB_TMP_BUFFER_LEN, fp)) > 0) {
          if (memcmp(value, buf, len)) {  // 检测到内容不相同
            is_need_write_file = true;
            break;
          }
        }
      }
      fclose(fp);
    }
  }
  // 将缓存原节点失效
  if (record != NULL) {
    record->status &= ~KVDB_STATUS_BIT_VALID;
  }
  // 找到空闲节点
  if (length > 0) {
    record = NodeOldest(length);
    if (record == NULL) {  // 没有找到空闲或者最旧的节点，可能是批量同时刻写满
      for (i = 0; i < cache_cnt_; i++) {
        if (length <= cache_size_[i]) {
          break;
        }
      }
      if (i < cache_cnt_) {
        record = cache_list_[i];
      } else {
        LOG(L_INFO) << "Exception: Saved at the first node!";
      }
    }
    if (record != NULL) {  // 找到适合记录的节点
      strcpy(record->key, key.c_str());
      memcpy(record->value, value, length);
      record->length = length;
      record->time_last = time(NULL);
      record->status |= KVDB_STATUS_BIT_VALID;
    }
  }
  // 写文件
  if (length == 0) {
    MAKE_FILE_NAME((char *)buf, KVDB_TMP_BUFFER_LEN, foldpath_, key.c_str());
    if (::remove((char *) buf) != 0) {
      return KVDB_ERR_WRITE_FILE;
    } else {
      return KVDB_ERR_OK;
    }
  }
  if (is_need_write_file) {
    // 操作文件
    MAKE_FILE_NAME((char *)buf, KVDB_TMP_BUFFER_LEN, foldpath_, key.c_str());
    fp = fopen((char *) buf, "wb");
    if (fp == NULL) {
      return KVDB_ERR_OPEN_FILE;
    }
    size_t w_len = fwrite(value, 1, length, fp);
    if ((int) w_len != length) {
      fclose(fp);
      return KVDB_ERR_WRITE_FILE;
    }
    fclose(fp);
  }

  return KVDB_ERR_OK;
}

KvdbError KvdbService::Replace(const std::string &key,
                               const std::string &value) {
  return Replace(key, (const uint8 *) value.c_str(), (int) value.length());
}

KvdbError KvdbService::Remove(const std::string &key) {
  struct KvdbRecord *record;
  // 准备文件路径
  char filename_temp[KVDB_FILE_PATH_MAXLEN + 5];
  if (key.length() == 0 || key.length() > KVDB_KEY_MAXLEN) {
    return KVDB_ERR_INVALID_PARAM;
  }

  // 删除缓存
  record = NodeFind(key);
  if (record != NULL) {
    record->status &= ~KVDB_STATUS_BIT_VALID;
  }
  MAKE_FILE_NAME(filename_temp, KVDB_FILE_PATH_MAXLEN + 5, foldpath_, key.c_str());
  // 操作文件
  if (::remove(filename_temp) != 0) {
    return KVDB_ERR_WRITE_FILE;
  }
  return KVDB_ERR_OK;
}

KvdbError KvdbService::Clear(const char *target_fold) {
  char buf[KVDB_TMP_BUFFER_LEN];
  KvdbError rtn = KVDB_ERR_OK;
  int i;
  int j;
  struct KvdbRecord *record;

  // 清空缓存
  for (i = 0; i < cache_cnt_; i++) {
    record = cache_list_[i];  // 指向缓存头结点
    for (j = 0; j < KVDB_CACHE_EACH_MAXCNT && record != NULL; j++) {
      record->status &= ~KVDB_STATUS_BIT_VALID;
      record = record->next;
    }

  }
#ifdef WIN32
  WIN32_FIND_DATA FindFileData;

  strcpy(buf, target_fold);
  strcat(buf, "\\*.*");
  HANDLE hFind = FindFirstFile(buf, &FindFileData);
  if (INVALID_HANDLE_VALUE == hFind)
    return rtn;

  do {
    if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      ::remove(FindFileData.cFileName);
    }
  } while (FindNextFile(hFind, &FindFileData));

  FindClose(hFind);

#else
  DIR *dir;
  struct stat statbuf;
  struct dirent *dirent;

  // 清空文件
  if (target_fold == NULL) {
    target_fold = foldpath_;
  }
  dir = opendir(target_fold);
  if (dir == NULL) {
    return KVDB_ERR_OPEN_FOLD;
  }
  while ((dirent = readdir(dir)) != NULL) {
    snprintf(buf, KVDB_TMP_BUFFER_LEN, "%s/%s", target_fold, dirent->d_name);
    if (stat(buf, &statbuf) == -1) {
      LOG(L_ERROR) << "Stat file error";
      rtn = KVDB_ERR_DELETE_FILE;
      continue;
    }
    if (S_ISREG(statbuf.st_mode)) {
      if (::remove(buf) != 0) {
        LOG(L_ERROR) << "Delete file error";
        rtn = KVDB_ERR_DELETE_FILE;
      }
    }
  }
  closedir(dir);
#endif
  return rtn;
}

KvdbError KvdbService::Seek(const std::string &key,
                            uint8 *get_data,
                            int *length) {
  struct KvdbRecord *record;
  int rd_len = 0;
  int len = 0;
  uint8 buf[KVDB_TMP_BUFFER_LEN + 1];
  FILE *fp;

  if (key.length() == 0 || key.length() > KVDB_KEY_MAXLEN) {
    LOG(L_ERROR) << "Error lenght";
    return KVDB_ERR_INVALID_PARAM;
  }
  if (get_data == NULL || length == NULL || *length <= 0) {
    LOG(L_ERROR) << "Error parameter";
    return KVDB_ERR_INVALID_PARAM;
  }

  // 现在缓存中查找
  record = NodeFind(key);
  if (record != NULL) {  // 找到节点
    if (*length > record->length) {
      *length = record->length;
    }
    memcpy(get_data, record->value, *length);
    return KVDB_ERR_OK;
  }
  // 准备文件路径
  MAKE_FILE_NAME((char *)buf, KVDB_TMP_BUFFER_LEN, foldpath_, key.c_str());
  // 操作文件
  fp = fopen((char *) buf, "rb");
  if (fp == NULL) {
    LOG(L_ERROR) << "Error: fopen";
    return KVDB_ERR_OPEN_FILE;
  }
  // 获取文件大小
  fseek(fp, 0, SEEK_END);
  int file_length = (int) ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (*length > file_length) {
    *length = file_length;
  }
  // 查找空闲缓存
  record = NodeOldest(file_length);
  // 读文件
  while (rd_len < *length) {
    len = fread(buf, 1, KVDB_TMP_BUFFER_LEN, fp);
    if (len <= 0) {
      break;
    }
    memcpy(get_data, buf, len);
    if (record != NULL) {
      memcpy(record->value + rd_len, buf, len);
    }
    rd_len += len;
    get_data += len;
  }
  fclose(fp);
  if (record != NULL) {
    record->time_last = time(NULL);
    record->length = rd_len;
    strcpy(record->key, key.c_str());
    record->status |= KVDB_STATUS_BIT_VALID;
  }
  *length = rd_len;
  return KVDB_ERR_OK;
}

KvdbError KvdbService::Seek(const std::string &key, std::string &get_value) {
  char buf[KVDB_TMP_BUFFER_LEN + 1];
  int len;
  struct KvdbRecord *record;
  int rd_len = 0;
  FILE *fp;

  if (key.length() == 0 || key.length() > KVDB_KEY_MAXLEN) {
    LOG(L_ERROR) << "Error key";
    return KVDB_ERR_INVALID_PARAM;
  }

  get_value.clear();
  // 现在缓存中查找
  record = NodeFind(key);
  if (record != NULL) {  // 找到节点
    record->value[record->length] = '\0';
    get_value.append((char *) record->value);
    return KVDB_ERR_OK;
  }
  // 准备文件路径
  MAKE_FILE_NAME((char *)buf, KVDB_TMP_BUFFER_LEN, foldpath_, key.c_str());
  // 操作文件
  fp = fopen(buf, "rb");
  if (fp == NULL) {
    LOG(L_ERROR) << "Error: fopen ";
    return KVDB_ERR_OPEN_FILE;
  }
  // 获取文件大小
  fseek(fp, 0, SEEK_END);
  int file_length = (int) ftell(fp);
  fseek(fp, 0, SEEK_SET);
  // 查找空闲缓存
  record = NodeOldest(file_length);
  // 读文件
  for (;;) {
    len = fread(buf, 1, KVDB_TMP_BUFFER_LEN, fp);
    if (len <= 0) {
      break;
    }
    buf[len] = '\0';
    get_value.append(buf);
    if (record != NULL) {
      memcpy(record->value + rd_len, buf, len);
    }
    rd_len += len;
  }
  fclose(fp);
  if (record != NULL) {
    record->time_last = time(NULL);
    record->length = rd_len;
    strcpy(record->key, key.c_str());
    record->status |= KVDB_STATUS_BIT_VALID;
  }
  return KVDB_ERR_OK;
}

KvdbError KvdbService::Backup(const char *backup_fold) {
  if (backup_fold == NULL) {
    LOG(L_ERROR) << "Error: backup fold == NULL! ";
    return KVDB_ERR_INVALID_PARAM;
  }
  return FoldCopy(backup_fold, foldpath_);
}

KvdbError KvdbService::Restore(const char *backup_fold) {
  if (backup_fold == NULL) {
    LOG(L_ERROR) << "Error: restore fold == NULL! ";
    return KVDB_ERR_INVALID_PARAM;
  }
  return FoldCopy(foldpath_, backup_fold);
}

#ifdef _DEBUG
void KvdbService::DebugCacheShow() {
  int i;
  int j;
  struct KvdbRecord *record;

  for (i = 0; i < cache_cnt_; i++) {
    record = cache_list_[i];
    for (j = 0; j < KVDB_CACHE_MAXNUM && record != NULL; j++) {
      if (record->status & KVDB_STATUS_BIT_VALID) {
        printf("[%d-%d], t=%ld, key=\"%s\", len=%d, V=[%02X,%02X,%02X,%02X].\n",
               i,
               j,
               record->time_last,
               record->key,
               record->length,
               record->value[0],
               record->value[1],
               record->value[2],
               record->value[3]);
      }
      record = record->next;
    }
  }
}
#endif

////////////////////////////////////////////////////////////////////////////////

KvdbError KvdbService::FoldCreate(const char *fold_path) {
  int i;
  int len;
  char buf[KVDB_TMP_BUFFER_LEN];

  if (fold_path == NULL || strlen(fold_path) == 0) {
    return KVDB_ERR_INVALID_PARAM;
  }
  strncpy(buf, fold_path, 512);
  len = strlen(fold_path);

#ifdef WIN32
  for (i = 3; i < len; i++) {
    if (buf[i] == '\\') {
      buf[i] = '\0';
      ::CreateDirectory(buf, NULL);
      buf[i] = '\\';
    }
  }
  ::CreateDirectory(buf, NULL);
#else
  for (i = 1; i < len; i++) {
    if (buf[i] == '/') {
      buf[i] = '\0';
      if (access(buf, 0) != 0) {
        mkdir(buf, S_IRWXU | S_IRWXG | S_IRWXO);  // 777
      }
      buf[i] = '/';
    }
  }
  if (len > 0 && access(buf, 0) != 0) {
    mkdir(buf, S_IRWXU | S_IRWXG | S_IRWXO);  // 777
  }
  if (access(buf, 0) != 0) {
    return KVDB_ERR_OPEN_FILE;
  }
#endif
  return KVDB_ERR_OK;
}

KvdbError KvdbService::FoldCopy(const char *target_fold,
                                const char *src_fold) {
  char buf_src[KVDB_TMP_BUFFER_LEN];
  char buf_target[KVDB_TMP_BUFFER_LEN];
  KvdbError rtn = KVDB_ERR_OK;

#ifdef WIN32
  FoldCreate(target_fold);  // 创建文件夹
  Clear(target_fold);  // 清空目标文件夹

  WIN32_FIND_DATA FindFileData;

  strcpy(buf_src, src_fold);
  strcat(buf_src, "\\*.*");
  HANDLE hFind = FindFirstFile(buf_src, &FindFileData);
  if (INVALID_HANDLE_VALUE == hFind)
    return rtn;
  do {
    if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      snprintf(buf_src, KVDB_TMP_BUFFER_LEN, "%s\\%s",
               src_fold, FindFileData.cFileName);
      snprintf(buf_target, KVDB_TMP_BUFFER_LEN, "%s\\%s",
               target_fold, FindFileData.cFileName);
      rtn = FileCopy(buf_target, buf_src);  // 拷贝文件
      if (rtn != KVDB_ERR_OK)
        break;
    }
  } while (FindNextFile(hFind, &FindFileData));

  FindClose(hFind);
#else
  DIR *dir;
  struct stat statbuf;
  struct dirent *dirent;

  FoldCreate(target_fold);  // 创建文件夹
  Clear(target_fold);  // 清空目标文件夹
  dir = opendir(src_fold);
  if (dir == NULL) {
    return KVDB_ERR_OPEN_FILE;
  }
  while ((dirent = readdir(dir)) != NULL) {  // 遍历文件夹
    snprintf(buf_src, KVDB_TMP_BUFFER_LEN, "%s/%s", src_fold, dirent->d_name);
    if (stat(buf_src, &statbuf) == -1) {  // 获取文件状态
      LOG(L_ERROR) << "Stat file error";
      rtn = KVDB_ERR_DELETE_FILE;
      continue;
    }
    if (S_ISREG(statbuf.st_mode)) {  // 普通文件
      snprintf(buf_target, KVDB_TMP_BUFFER_LEN, "%s/%s", target_fold, dirent->d_name);
      rtn = FileCopy(buf_target, buf_src);  // 拷贝文件
      if (rtn != KVDB_ERR_OK)
        break;
    }
  }
  closedir(dir);
#endif
  return rtn;
}

KvdbError KvdbService::FileCopy(const char *target_filename,
                                const char *src_filename) {
  FILE *srcfile;
  FILE *targetfile;
  int c;
  KvdbError rtn = KVDB_ERR_OK;

  if (src_filename == NULL || target_filename == NULL) {
    return KVDB_ERR_INVALID_PARAM;
  }
  srcfile = fopen(src_filename, "rb");
  if (srcfile == NULL) {
    return KVDB_ERR_OPEN_FILE;
  }
  targetfile = fopen(target_filename, "wb");
  if (targetfile == NULL) {
    fclose(srcfile);
    return KVDB_ERR_OPEN_FILE;
  }
  for (;;) {
    c = fgetc(srcfile);
    if (feof(srcfile)) {
      break;
    }
    fputc(c, targetfile);
  }
  fclose(srcfile);
  fclose(targetfile);
  return rtn;
}

}
