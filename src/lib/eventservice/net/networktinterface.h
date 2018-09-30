/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#ifndef EVENTSERVICE_NET_NETWORKDINTERFACE_H_
#define EVENTSERVICE_NET_NETWORKDINTERFACE_H_

#include "eventservice/base/basicincludes.h"
#include "eventservice/base/socket.h"
#include "eventservice/mem/membuffer.h"

namespace vzes {

/* 数据包编码类型定义 */
typedef enum {
  PKT_ENCODE_BASE64,
  PKT_ENCODE_BINARY,
  PKT_ENCODE_NONE,
} PACKET_ENCODE_TYPE;

class AsyncSocket {
 public:
  typedef boost::shared_ptr<AsyncSocket> Ptr;

  // 错误事件
  sigslot::signal2<AsyncSocket::Ptr, int>
  SignalSocketErrorEvent;
  // 写完成事件
  sigslot::signal1<AsyncSocket::Ptr>
  SignalSocketWriteEvent;
  // 读完成事件
  sigslot::signal2<AsyncSocket::Ptr, MemBuffer::Ptr>
  SignalSocketReadEvent;

  // Async write the data, if the operator complete, SignalWriteCompleteEvent
  // will be called
  virtual bool AsyncWrite(MemBuffer::Ptr buffer) = 0;
  virtual bool AsyncWrite(const char *data, std::size_t size);
  virtual bool AsyncRead() = 0;

  // Returns the address to which the socket is bound.  If the socket is not
  // bound, then the any-address is returned.
  virtual SocketAddress GetLocalAddress() const = 0;

  // Returns the address to which the socket is connected.  If the socket is
  // not connected, then the any-address is returned.
  virtual SocketAddress GetRemoteAddress() const = 0;

  // 设置Socket编码方式，如果编码方式不为PKT_ENCODE_NONE，那么在发送数据的时候
  // 先检查每块MemBuffer Block的编码标志，如果Block设置了编码标志，则先对Block
  // 源数据进行编码后再发送。
  virtual void SetEncodeType(PACKET_ENCODE_TYPE encode_type) = 0;

  virtual void Close() = 0;
  virtual int GetError() const = 0;
  virtual void SetError(int error) = 0;
  virtual bool IsConnected() = 0;
  virtual bool IsClose() {
    return !IsConnected();
  }
  virtual bool IsOpen() {
    return IsConnected();
  }

  const SocketAddress remote_addr() {
    return GetRemoteAddress();
  }

  const SocketAddress local_addr() {
    return GetLocalAddress();
  }

  virtual int GetOption(Option opt, int* value) = 0;
  virtual int SetOption(Option opt, int value) = 0;

  void RemoveAllSignal();
};

class AsyncListener {
 public:
  typedef boost::shared_ptr<AsyncListener> Ptr;
  // 新连接到来的事件
  sigslot::signal3<AsyncListener::Ptr,
          Socket::Ptr,
          int>  SignalNewConnected;
  virtual bool Start(const SocketAddress addr, bool addr_reused = false) = 0;
  virtual void Close() = 0;
  virtual const SocketAddress BindAddress() = 0;
};

class  AsyncConnecter {
 public:
  typedef boost::shared_ptr<AsyncConnecter> Ptr;
  // 连接成功的事件
  sigslot::signal3<AsyncConnecter::Ptr,
          Socket::Ptr,
          int>  SignalServerConnected;
  virtual bool Connect(const SocketAddress addr, uint32 time_out) = 0;
  virtual void Close() = 0;
  virtual const SocketAddress ConnectAddress() = 0;
};

////////////////////////////////////////////////////////////////////////////////

class AsyncUdpSocket {
 public:
  typedef boost::shared_ptr<AsyncUdpSocket> Ptr;

  // 错误事件
  sigslot::signal2<AsyncUdpSocket::Ptr, int>
  SignalSocketErrorEvent;
  //// 写完成事件
  sigslot::signal1<AsyncUdpSocket::Ptr>
  SignalSocketWriteEvent;
  // 读完成事件
  sigslot::signal3<AsyncUdpSocket::Ptr, MemBuffer::Ptr, const SocketAddress &>
  SignalSocketReadEvent;

  // Async write the data, if the operator complete, SignalWriteCompleteEvent
  // will be called
  virtual bool SendTo(MemBuffer::Ptr buffer,
                      const SocketAddress &addr) = 0;
  virtual bool SendTo(const char *data,
                      std::size_t size,
                      const SocketAddress &addr);
  virtual bool AsyncRead() = 0;

  // Returns the address to which the socket is bound.  If the socket is not
  // bound, then the any-address is returned.
  virtual SocketAddress GetLocalAddress() const = 0;

  // Returns the address to which the socket is connected.  If the socket is
  // not connected, then the any-address is returned.
  virtual SocketAddress GetRemoteAddress() const = 0;


  virtual void Close() = 0;
  virtual int GetError() const = 0;
  virtual void SetError(int error) = 0;
  virtual bool IsConnected() = 0;
  virtual bool IsClose() {
    return !IsConnected();
  }
  virtual bool IsOpen() {
    return IsConnected();
  }

  const SocketAddress remote_addr() {
    return GetRemoteAddress();
  }

  const SocketAddress local_addr() {
    return GetLocalAddress();
  }

  virtual int GetOption(Option opt, int* value) = 0;
  virtual int SetOption(Option opt, int value) = 0;

  void RemoveAllSignal();
};

}  // namespace vzes

#endif  // EVENTSERVICE_NET_NETWORKDINTERFACE_H_
