/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#ifndef EVENTSERVICE_NET_ASYNC_PACKET_SOCKET_H_
#define EVENTSERVICE_NET_ASYNC_PACKET_SOCKET_H_

#include "eventservice/base/basicincludes.h"
#include "eventservice/net/eventservice.h"
#include "eventservice/net/networktinterface.h"

namespace vzes {

#define PACKET_HEADER_SIZE     (8)
#define PACKET_RECV_BUFF_SIZE  (64 * 1024)
#define PACKET_BODY_SIZE       (PACKET_RECV_BUFF_SIZE - PACKET_HEADER_SIZE)

struct PacketHeader {
  uint8   v;
  uint8   z;
  uint16  flag;
  uint32  data_size;
};

class AsyncPacketSocket : public boost::noncopyable,
  public boost::enable_shared_from_this<AsyncPacketSocket>,
  public sigslot::has_slots<> {
 public:
  typedef boost::shared_ptr<AsyncPacketSocket> Ptr;
  sigslot::signal3<AsyncPacketSocket::Ptr,
          MemBuffer::Ptr, uint16>               SignalPacketEvent;
  sigslot::signal2<AsyncPacketSocket::Ptr, int> SignalPacketError;
  sigslot::signal1<AsyncPacketSocket::Ptr>      SignalPacketWrite;
 public:
  AsyncPacketSocket(EventService::Ptr event_service,
                    AsyncSocket::Ptr socket);
  virtual ~AsyncPacketSocket();

 public:
  bool AsyncWritePacket(const char *data, uint32 size, uint16 flag);
  bool AsyncWritePacket(MemBuffer::Ptr buffer, uint16 flag);
  bool AsyncRead();

  virtual void            Close();
  const SocketAddress     local_addr();
  const SocketAddress     remote_addr();
  std::string             ip_addr();
  bool                    IsOpen() const;
  bool                    IsClose();
  // Socket::Ptr               SocketNumber();

 private:
  bool AnalysisPacket(MemBuffer::Ptr buffer);
  void SignalClose(int error_code, bool is_signal);
  void LiveSignalClose(int error_code, bool is_signal);

  void OnAsyncSocketWriteEvent(AsyncSocket::Ptr socket);

  void OnAsyncSocketReadEvent(AsyncSocket::Ptr socket,
                              MemBuffer::Ptr data_buffer);
  void OnAsyncSocketErrorEvent(AsyncSocket::Ptr socket,
                               int error_code);
 private:
  AsyncSocket::Ptr async_socket_;
  MemBuffer::Ptr   recv_buff_;
};


}  // namespace stp

#endif  // EVENTSERVICE_NET_ASYNC_PACKET_SOCKET_H_
