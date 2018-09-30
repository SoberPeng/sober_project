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

#ifndef SRC_LIBVZTINYDB_KVDBSERVICE_H_
#define SRC_LIBVZTINYDB_KVDBSERVICE_H_

#include <string>
#include <stdio.h>
#include <time.h>
#include "eventservice/base/basicincludes.h"

namespace cache {

#define KVDB_FILE_PATH_MAXLEN    128   ///< 文件全路径名称长度
#define KVDB_KEY_MAXLEN          64    ///< 关键字字符数限制
#define KVDB_CACHE_MAXNUM        4     ///< 缓存个数
#define KVDB_CACHE_EACH_MAXCNT   1000  ///< 每种缓存的最多节点数


#ifdef WIN32
#define MAKE_FILE_NAME(X,N,Y,Z) snprintf(X, N, "%s\\%s", Y, Z)
#else
#define MAKE_FILE_NAME(X,N,Y,Z) snprintf(X, N, "%s/%s", Y, Z)
#endif

/**@name    错误类型定义
* @{
*/
typedef int KvdbError;
#define KVDB_ERR_OK              0  ///< 正确
#define KVDB_ERR_OPEN_FILE       1  ///< 打开文件错误
#define KVDB_ERR_WRITE_FILE      2  ///< 写文件错误
#define KVDB_ERR_READ_FILE       3  ///< 读文件错误
#define KVDB_ERR_PARSE_FILE      4  ///< 解析文件错误
#define KVDB_ERR_INVALID_PARAM   5  ///< 无效参数错误
#define KVDB_ERR_SEEK_NULL       6  ///< 查找结果为空
#define KVDB_ERR_FULL            7  ///< 数据库满错误
#define KVDB_ERR_DELETE_FILE     8  ///< 文件删除错误
#define KVDB_ERR_OPEN_FOLD       9  ///< 打开文件夹错误
#define KVDB_ERR_UNKNOWN         99 ///< 未知错误
/**@} */

/**@name    状态标识位定义
* @{
*/
#define KVDB_HEAD_FLAG           0x47  ///< 起始标识
#define KVDB_STATUS_BIT_VALID    0x01  ///< 有效位，0=空闲、1=有效
/**@} */

/**
* KVDB缓存构造结构体（类外访问）
*/
struct KvdbCache {
  int size;        ///< 节点大小
  int counter;     ///< 节点个数
};

/**
* KVDB缓存结构体（类内使用）
*/
struct KvdbRecord {
  uint8   status;     ///<  状态标识，VZ_TDB_HEAD_BIT_VALID
  time_t  time_last;  ///<  上次访问时间
  char    key[KVDB_KEY_MAXLEN + 1];  ///< 关键字KEY
  int     length;     ///
  uint8  *value;      ///< 关键字VALUE，不同档位对应不同长度
  struct  KvdbRecord *next;  ///< 下一个数据
};

class KvdbService {
 public:
  /**
  * @brief 构造函数
  * 根据文件名创建skvdb和普通kvdb，业务层实现不同操作规则
  * 缓存序列需要根据长度由小到达排列
  * @param[in] foldpath    文件夹路径
  * @param[in] cache       缓存类型定义序列
  * @param[in] cache_cnt   缓存类型定义个数
  */
  KvdbService(const char *foldpath, struct KvdbCache *cache, int cache_cnt);
  /**
  * @brief 析构函数
  */
  ~KvdbService();
  /**
  * @brief 修改或增加一条记录
  * 如果已保存相同关键字记录就修改，否则新增
  * @param[in] key    关键字
  * @param[in] value    值
  * @param[in] length   值长度
  *
  * @return 返回操作结果
  */
  KvdbError Replace(const std::string &key, const uint8 *value, int length);
  /**
  * @brief 修改或增加一条记录
  * 如果已保存相同关键字记录就修改，否则新增
  * @param[in] key    关键字
  * @param[in] value    值
  *
  * @return 返回操作结果
  */
  KvdbError Replace(const std::string &key, const std::string &value);
  /**
  * @brief 删除记录
  * @param[in] key    关键字
  *
  * @return 返回操作结果
  */
  KvdbError Remove(const std::string &key);
  /**
  * @brief 清空记录
  * @param[in] target_fold    待清空的文件夹目录
  *
  * @return 返回操作结果
  */
  KvdbError Clear(const char *target_fold = NULL);
  /**
    * @brief 查找记录
    * 根据关键字查找记录
    * @param[in] key     待查关键字
    * @param[out] get_value     查询结果
    * @param[inout] length     输入：缓存大小；输出：实际读取长度
    *
    * @return 返回操作结果
    */
  KvdbError Seek(const std::string &key, uint8 *get_data, int *length);
  /**
  * @brief 查找记录
  * 根据关键字查找记录
  * @param[in] key     待查关键字
  * @param[out] get_value     查询结果
  *
  * @return 返回操作结果
  */
  KvdbError Seek(const std::string &key, std::string &get_value);
  /**
  * @brief 备份
  *  将当前文件夹文件备份到备份文件夹
  * @param[in] backup_fold    备份文件夹路径
  *
  * @return 返回操作结果
  */
  KvdbError Backup(const char *backup_fold);
  /**
  * @brief 还原
  *  将备份文件夹文件备份到当前文件夹
  * @param[in] backup_fold    备份文件夹路径
  *
  * @return 返回操作结果
  */
  KvdbError Restore(const char *backup_fold);

#ifdef _DEBUG
  /**
  * @brief 打印缓存
  *  调试代码，打印缓存中有效节点信息
  */
  void DebugCacheShow();
#endif

 private:
  /**
  * @brief 创建文件夹
  */
  KvdbError FoldCreate(const char *fold_path);
  /**
  * @brief 复制文件夹
  */
  KvdbError FoldCopy(const char *target_fold, const char *src_fold);
  /**
  * @brief 复制文件
  */
  KvdbError FileCopy(const char *target_filename, const char *src_filename);
  /**
  * @brief 查找节点
  */
  struct KvdbRecord *NodeFind(const std::string &key);
  /**
  * @brief 查找空闲或最久节点
  */
  struct KvdbRecord *NodeOldest(int length);

  char foldpath_[KVDB_FILE_PATH_MAXLEN + 1];
  struct KvdbRecord *cache_list_[KVDB_CACHE_MAXNUM];
  int cache_size_[KVDB_CACHE_MAXNUM];
  int cache_cnt_;
};
}

#endif  // SRC_LIBVZTINYDB_KVDBSERVICE_H_
