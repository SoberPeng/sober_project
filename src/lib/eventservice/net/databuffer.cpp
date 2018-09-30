/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#include "eventservice/net/databuffer.h"

#if 0

namespace vzes {


MemBuffer::MemBuffer(const char *data, int data_size) {
  BOOST_ASSERT(data_size < DEFAULT_BLOCK_SIZE);
  memcpy(buffer_, data, data_size);
  buffer_size_ = data_size;
}

MemBuffer::MemBuffer()
  : buffer_size_(0) {
  buffer_[0] = 0;
}

void MemBuffer::Write(const char *data, int data_size) {
  BOOST_ASSERT(data_size < DEFAULT_BLOCK_SIZE);
  memcpy(buffer_, data, data_size);
  buffer_size_ = data_size;
}

MemBufferPool *MemBufferPool::pool_instance_ = NULL;

MemBufferPool *MemBufferPool::Instance() {
  if (!pool_instance_) {
    pool_instance_ = new MemBufferPool();
  }
  return pool_instance_;
}

MemBufferPool::MemBufferPool() {
}

MemBufferPool::~MemBufferPool() {
}

MemBuffer::Ptr MemBufferPool::TakeMemBuffer(std::size_t mini_size) {
  // LOG(L_WARNING) << "Data buffer size = " << data_buffers_.size();
  if (data_buffers_.size()) {
    return TaskPerfectMemBuffer(mini_size);
  }
  return MemBuffer::Ptr(new MemBuffer(), &MemBufferPool::RecyleMemBuffer);
}

MemBuffer::Ptr MemBufferPool::TaskPerfectMemBuffer(std::size_t mini_size) {
  // Small -> Big
  MemBuffer *data_buffer = data_buffers_.front();
  data_buffers_.pop_front();
  // LOG(INFO) << "Task stanza by back size = " << data_buffers_.size();
  return MemBuffer::Ptr(data_buffer, &MemBufferPool::RecyleMemBuffer);
}

void MemBufferPool::InternalRecyleMemBuffer(MemBuffer *stanza) {
  InsertMemBuffer(stanza);
}

void MemBufferPool::RecyleMemBuffer(void *data_buffer) {
  MemBufferPool::Instance()->InternalRecyleMemBuffer(
    (MemBuffer *)data_buffer);
}

void MemBufferPool::InsertMemBuffer(MemBuffer *data_buffer) {
  data_buffers_.push_back(data_buffer);
  // LOG(INFO) << "Task stanza by back size = " << data_buffers_.size();
}


}  // namespace vzes

#endif
