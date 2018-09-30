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

#include <iostream>
#include "eventservice/mem/membuffer.h"
#include <string.h>

#define TEST_DATA_SIZE 1024
const char TEST_DATA[TEST_DATA_SIZE] = {0};

void MemBufferReadWriteBigTest() {
  LOG(L_INFO) << "--------------------------------------------------------";
  char *read_buffer = new char[TEST_DATA_SIZE];
  vzes::MemBuffer::Ptr mb = vzes::MemBuffer::CreateMemBuffer();
  LOG(L_INFO) << "MemBufferReadWriteBigTest Write start = "
              << mb->size() << "\t" << mb->BlocksSize();
  mb->WriteBytes(TEST_DATA, TEST_DATA_SIZE);
  BOOST_ASSERT(mb->size() == TEST_DATA_SIZE);
  LOG(L_INFO) << "MemBufferReadWriteBigTest Write End = "
              << mb->size() << "\t" << mb->BlocksSize();
  mb->CopyBytes(0, read_buffer, TEST_DATA_SIZE);
  BOOST_ASSERT(mb->size() == TEST_DATA_SIZE);
  LOG(L_INFO) << "MemBufferReadWriteBigTest Copy end = "
              << mb->size() << "\t" << mb->BlocksSize();
  mb->ReadBytes(read_buffer, TEST_DATA_SIZE);
  BOOST_ASSERT(mb->size() == 0);
  LOG(L_INFO) << "MemBufferReadWriteBigTest Read End = "
              << mb->size() << "\t" << mb->BlocksSize();
}

void MemBufferReadWriteCorrectTest() {
  LOG(L_INFO) << "--------------------------------------------------------";
  char *read_buffer = new char[TEST_DATA_SIZE];
  vzes::MemBuffer::Ptr mb = vzes::MemBuffer::CreateMemBuffer();
  LOG(L_INFO) << "MemBufferReadWriteCorrectTest Write Start "
              << mb->size() << "\t" << mb->BlocksSize();
  for (uint32 i = 0; i < TEST_DATA_SIZE; i++) {
    mb->WriteUInt32(i);
  }
  LOG(L_INFO) << "MemBufferReadWriteCorrectTest Write Done "
              << mb->size() << "\t" << mb->BlocksSize();
  //for (uint32 i = 0; i < TEST_DATA_SIZE; i++) {
  //  uint32 data = 0;
  //  // 这里面读取数据特别慢，时间都花在定位数据块上面了，必须要优化这个过程
  //  mb->CopyUInt32(i * sizeof(uint32), &data);
  //  if (data != i) {
  //    LOG(L_ERROR) << "Data error ";
  //    return ;
  //  }
  //}
  LOG(L_INFO) << "MemBufferReadWriteCorrectTest Read Start "
              << mb->size() << "\t" << mb->BlocksSize();
  for (uint32 i = 0; i < TEST_DATA_SIZE; i++) {
    uint32 data = 0;
    mb->ReadUInt32(&data);
    if (data != i) {
      LOG(L_ERROR) << "Data error ";
      return ;
    }
  }
  LOG(L_INFO) << "MemBufferReadWriteCorrectTest Read Done "
              << mb->size() << "\t" << mb->BlocksSize();
}

void NormalMemorySpeedTest() {
  LOG(L_INFO) << "--------------------------------------------------------";
  char *buffer = new char[TEST_DATA_SIZE * sizeof(uint32)];
  LOG(L_INFO) << "NormalMemorySpeedTest Star Write";
  for (uint32 i = 0; i < TEST_DATA_SIZE; i++) {
    memcpy(buffer + (i * sizeof(uint32)), (const char *)(&i), sizeof(uint32));
  }
  LOG(L_INFO) << "NormalMemorySpeedTest End Write";
  //for (uint32 i = 0; i < TEST_DATA_SIZE; i++) {
  //  uint32 data = 0;
  //  // 这里面读取数据特别慢，时间都花在定位数据块上面了，必须要优化这个过程
  //  mb->CopyUInt32(i * sizeof(uint32), &data);
  //  if (data != i) {
  //    LOG(L_ERROR) << "Data error ";
  //    return ;
  //  }
  //}
  LOG(L_INFO) << "NormalMemorySpeedTest Start Read";
  for (uint32 i = 0; i < TEST_DATA_SIZE; i++) {
    uint32 data = 0;
    memcpy((void *)&data, buffer + (i * sizeof(uint32)), sizeof(uint32));
    if (data != i) {
      LOG(L_ERROR) << "Data error ";
      return ;
    }
  }
  LOG(L_INFO) << "NormalMemorySpeedTest End Read";
}

void MembufferRawReadTest() {
  LOG(L_INFO) << "--------------------------------------------------------";
  char *read_buffer = new char[TEST_DATA_SIZE];
  vzes::MemBuffer::Ptr mb = vzes::MemBuffer::CreateMemBuffer();
  LOG(L_INFO) << "MembufferRawReadTest Write Start "
              << mb->size() << "\t" << mb->BlocksSize();


  for (uint32 i = 0; i < TEST_DATA_SIZE; i++) {
    mb->WriteUInt32(i);
  }




  LOG(L_INFO) << "MembufferRawReadTest Write Done "
              << mb->size() << "\t" << mb->BlocksSize();
  LOG(L_INFO) << "MembufferRawReadTest Read Start "
              << mb->size() << "\t" << mb->BlocksSize();
  vzes::BlocksPtr blocks = mb->blocks();
  int index = 0;
  for (vzes::BlocksPtr::iterator iter = blocks.begin();
       iter != blocks.end(); iter++) {
    const char *data = (const char *)((*iter)->buffer);
    uint32 data_size = (*iter)->buffer_size / 4;
    for (int i = 0; i < data_size; i++, index++) {
      uint32 c = 0;
      memcpy((void *)&c, data + (i * sizeof(uint32)), sizeof(uint32));
      if (c != index) {
        LOG(L_ERROR) << "Data error ";
        return ;
      }
    }
  }
  LOG(L_INFO) << "MembufferRawReadTest Read Done "
              << mb->size() << "\t" << mb->BlocksSize();
}

int main(void) {
  // Initialize the logging system
  vzes::LogMessage::LogTimestamps(true);
  vzes::LogMessage::LogContext(vzes::LS_INFO);
  vzes::LogMessage::LogThreads(true);

  MemBufferReadWriteBigTest();
  MemBufferReadWriteCorrectTest();
  NormalMemorySpeedTest();
  MembufferRawReadTest();

  return EXIT_SUCCESS;
}