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

#ifndef EVENTSERVICE_MEM_MEMBUFFER_H_
#define EVENTSERVICE_MEM_MEMBUFFER_H_

#include "eventservice/base/basicincludes.h"

namespace vzes {

#define DEFAULT_BLOCK_SIZE 768

struct Block : public boost::noncopyable {
  typedef boost::shared_ptr<Block> Ptr;
  Block() {
    buffer_size  = 0;
    buffer[0]    = 0;
    encode_flag_ = false;
  }
  size_t  RemainSize() const {
    return DEFAULT_BLOCK_SIZE - buffer_size;
  }

  static Block::Ptr TakeBlock();

  size_t  WriteBytes(const char* val, size_t len);
  size_t  ReadBytes(char* val, size_t len);
  size_t  ReadString(std::string* val, size_t len);
  size_t  CopyBytes(size_t pos, char* val, size_t len);
  size_t  CopyString(size_t pos, std::string* val, size_t len);

  uint8   buffer[DEFAULT_BLOCK_SIZE];
  size_t  buffer_size;
  bool    encode_flag_;
};

typedef std::list<Block *> Blocks;
typedef std::list<Block::Ptr> BlocksPtr;

// 这个类一般适用于在大数据量传输通过种使用，目前只在两个地方使用
// 1. Filecahe存放图片的大数据应用
// 2. 网络数据传输
// 使用注意，多次写入少量数据的速度是非常慢的，尽量一次写入大量数据
class MemBuffer : public boost::noncopyable {
 public:
  typedef boost::shared_ptr<MemBuffer> Ptr;
  virtual ~MemBuffer();
  static MemBuffer::Ptr CreateMemBuffer();
 private:
  MemBuffer();
 public:
  size_t Length() const ;
  size_t size() const;
  size_t BlocksSize() const {
    return blocks_.size();
  }

  void DumpData();

  BlocksPtr &blocks() {
    return blocks_;
  };

  void AppendBuffer(MemBuffer::Ptr buffer);
  void AppendBlock(Block::Ptr block);
  void ReduceSize(size_t size) {
    size_ = size_ - size;
  }
  void Clear();

  // 开启\关闭数据编码标识。对MemBuffer中的所有Blocks设置该标识，在通过
  // NetWorkInterface AsyncWrite()发包时，如果Socket同时也设置了编码方式
  // （通过SetEncodeType()设置），则先对Block数据进行编码后再发送。
  bool EnableEncode(bool enable);

  // Copy a next value from the buffer. Return false if there isn't
  // enough data left for the specified type.
  bool CopyUInt8(size_t pos, uint8* val);
  bool CopyUInt16(size_t pos, uint16* val);
  bool CopyUInt32(size_t pos, uint32* val);
  bool CopyUInt64(size_t pos, uint64* val);
  bool CopyBytes(size_t pos, char* val, size_t len);

  // Appends next |len| bytes from the buffer to |val|. Returns false
  // if there is less than |len| bytes left.
  bool CopyString(size_t pos, std::string* val, size_t len);


  // Read a next value from the buffer. Return false if there isn't
  // enough data left for the specified type.
  bool ReadUInt8(uint8* val);
  bool ReadUInt16(uint16* val);
  bool ReadUInt32(uint32* val);
  bool ReadUInt64(uint64* val);
  bool ReadBytes(char* val, size_t len);
  bool ReadBuffer(MemBuffer::Ptr buffer, size_t len);

  // Appends next |len| bytes from the buffer to |val|. Returns false
  // if there is less than |len| bytes left.
  bool ReadString(std::string* val, size_t len);

  std::string ToString();

  // Write value to the buffer. Resizes the buffer when it is
  // neccessary.
  void WriteUInt8(uint8 val);
  void WriteUInt16(uint16 val);
  void WriteUInt32(uint32 val);
  void WriteUInt64(uint64 val);
  void WriteString(const std::string& val);
  void WriteBytes(const char* val, size_t len);
 private:
  void WriteNewBytes(const char* val, size_t len);
  BlocksPtr::iterator GetPostion(size_t pos, size_t *block_pos);
 private:
  size_t    size_;
  BlocksPtr blocks_;
};

}

#endif  // EVENTSERVICE_MEM_MEMBUFFER_H_

