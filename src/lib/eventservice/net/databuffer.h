/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#ifndef EVENTSERVICE_NET_MemBuffer_H_
#define EVENTSERVICE_NET_MemBuffer_H_

#if 0

#include <vector>
#include "eventservice/base/basicincludes.h"

namespace vzes {

// http://man7.org/linux/man-pages/man7/tcp.7.html
// tcp_rmem (since Linux 2.4)
//     This is a vector of 3 integers: [min, default, max].  These
//     parameters are used by TCP to regulate receive buffer sizes.
//     TCP dynamically adjusts the size of the receive buffer from
//     the defaults listed below, in the range of these values,
//     depending on memory available in the system.

//     min       minimum size of the receive buffer used by each TCP
//               socket.  The default value is the system page size.
//               (On Linux 2.4, the default value is 4 kB, lowered to
//               PAGE_SIZE bytes in low-memory systems.)  This value
//               is used to ensure that in memory pressure mode,
//               allocations below this size will still succeed.
//               This is not used to bound the size of the receive
//               buffer declared using SO_RCVBUF on a socket.

//     default   the default size of the receive buffer for a TCP
//               socket.  This value overwrites the initial default
//               buffer size from the generic global
//               net.core.rmem_default defined for all protocols.
//               The default value is 87380 bytes.  (On Linux 2.4,
//               this will be lowered to 43689 in low-memory
//               systems.)  If larger receive buffer sizes are
//               desired, this value should be increased (to affect
//               all sockets).  To employ large TCP windows, the
//               net.ipv4.tcp_window_scaling must be enabled
//               (default).

//     max       the maximum size of the receive buffer used by each
//               TCP socket.  This value does not override the global
//               net.core.rmem_max.  This is not used to limit the
//               size of the receive buffer declared using SO_RCVBUF
//               on a socket.  The default value is calculated using
//               the formula

//                   max(87380, min(4 MB, tcp_mem[1]*PAGE_SIZE/128))

//               (On Linux 2.4, the default is 87380*2 bytes, lowered
//               to 87380 in low-memory systems).

// DEFAULT_BLOCK_SIZE 设置为8184 是为了保证MemBuffer总体占用的空间大小为8196
#define DEFAULT_BLOCK_SIZE 64 * 1024 - 12

class MemBuffer : public boost::noncopyable,
  public boost::enable_shared_from_this<MemBuffer> {
 public:
  typedef boost::shared_ptr<MemBuffer> Ptr;
 public:
  MemBuffer(const char *data, int data_size);
  MemBuffer();
  void Write(const char *data, int data_size);
  char buffer_[DEFAULT_BLOCK_SIZE];
  int  buffer_size_;
};

typedef vzes::MemBuffer  MemBuffer;
typedef std::list<MemBuffer::Ptr>    MemBufferLists;
typedef std::vector<MemBuffer::Ptr>  MemBufferVector;

class MemBufferPool : public boost::noncopyable,
  public boost::enable_shared_from_this<MemBufferPool> {
 public:
  typedef boost::shared_ptr<MemBufferPool> Ptr;
  MemBufferPool();
  virtual ~MemBufferPool();
  static MemBufferPool *Instance();
  // Thread safed
  MemBuffer::Ptr TakeMemBuffer(std::size_t mini_size);
  static void RecyleMemBuffer(void *data_buffer);
 private:
  void InsertMemBuffer(MemBuffer *data_buffer);
  MemBuffer::Ptr TaskPerfectMemBuffer(std::size_t mini_size);
  void InternalRecyleMemBuffer(MemBuffer *data_buffer);
 private:
  typedef std::list<MemBuffer *> MemBuffers;
  MemBuffers data_buffers_;
 private:
  static MemBufferPool *pool_instance_;
};

}  // namespace vzes

#endif

#endif  // EVENTSERVICE_NET_MemBuffer_H_
