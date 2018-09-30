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

#ifndef _FILECHACHE_KVDB_KVDBCLIENT_C_H_
#define _FILECHACHE_KVDB_KVDBCLIENT_C_H_

#include "eventservice/base/basictypes.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */


#define KVDB_SUCCEED    true
#define KVDB_FAILURE    false

typedef void* KVDB_HANDLE;

/**
 * 创建kvdb client实例
 *
 * @param name
 *   kvdb名称,唯一的标识一个kvdb对象
 *   有效值: "0-9"、"A～Z"、"a～z"、"_"（下划线）,如，"kvdb"、"SKVDB"
 * @return 成功: 实例句柄，失败: NULL
 */
KVDB_HANDLE Kvdb_Create(const char *name);

/**
 * 销毁kvdb client实例
 *
 * @param hKvdb
 *   待销毁的kvdb实例句柄
 * @return 无
 */
void Kvdb_Destory(KVDB_HANDLE hKvdb);

/**
 * 存储[key, value]
 *
 * @param hKvdb
 *   kvdb实例句柄
 * @param key
 *   待存储的key buffer,有效值: "A～Z"、"a～z"、"_"（下划线）
 * @param key_size
 *   key的长度，单位Byte，最长为64
 * @param value
 *   value buffer
 * @param value_size
 *   buffer的长度，单位Byte，最长为8K
 * @return 成功: KVDB_SUCCEED，失败: KVDB_FAILURE
 */
bool Kvdb_SetKey(KVDB_HANDLE hKvdb, const char *key, uint8 key_size,
                 const char *value, uint32 value_size);

/**
 * 查询[key, value]
 *
 * @param hKvdb
 *   kvdb实例句柄
 * @param key
 *   待查询的key
 * @param key_size
 *   key的长度，单位Byte
 * @param value
 *   value输出buffer
 * @param value_size
 *   输出buffer的长度，单位Byte
 * @return 成功: KVDB_SUCCEED，失败: KVDB_FAILURE
 */
bool Kvdb_GetKey(KVDB_HANDLE hKvdb, const char *key, uint8 key_size,
                 void *buffer, uint32 buffer_size);

/**
 * 删除key
 *
 * @param hKvdb
 *   kvdb实例句柄
 * @param key
 *   待删除的key
 * @param key_size
 *   key的长度，单位Byte
 * @return 成功: KVDB_SUCCEED，失败: KVDB_FAILURE
 */
bool Kvdb_DeleteKey(KVDB_HANDLE hKvdb, const char *key, uint8 key_size);

/**
 * 备份Kvdb文件
 *
 * @param hKvdb
 *   kvdb实例句柄
 * @return 成功: KVDB_SUCCEED，失败: KVDB_FAILURE
 */
bool Kvdb_BackupDatabase(KVDB_HANDLE hKvdb);

/**
 * 重置Kvdb文件
 *
 * @param hKvdb
 *   kvdb实例句柄
 * @return 成功: KVDB_SUCCEED，失败: KVDB_FAILURE
 */
bool Kvdb_RestoreDatabase(KVDB_HANDLE hKvdb);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  // _FILECHACHE_KVDB_KVDBCLIENT_C_H_
