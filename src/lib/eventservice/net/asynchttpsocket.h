/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#ifndef EVENTSERVICE_NET_ASYNC_HTTP_SOCKET_H_
#define EVENTSERVICE_NET_ASYNC_HTTP_SOCKET_H_

#include <vector>
#include "eventservice/base/basicincludes.h"
#include "eventservice/net/eventservice.h"
#include "eventservice/net/networktinterface.h"
#include "eventservice/http/http_parser.h"
#include "eventservice/http/reply.h"

namespace vzes {


class AsyncHttpSocket : public boost::noncopyable,
  public boost::enable_shared_from_this<AsyncHttpSocket>,
  public sigslot::has_slots<> {
 public:
  typedef boost::shared_ptr<AsyncHttpSocket> Ptr;

  sigslot::signal2<AsyncHttpSocket::Ptr, HttpReqMessage&> SignalHttpPacketEvent;
  sigslot::signal2<AsyncHttpSocket::Ptr, int> SignalHttpPacketError;
  sigslot::signal1<AsyncHttpSocket::Ptr>      SignalHttpPacketWrite;
 public:
  AsyncHttpSocket(AsyncSocket::Ptr async_socket,
                  SocketAddress &remote_addr);
  virtual ~AsyncHttpSocket();

 public:
  bool AsyncWritePacket(const char *data, uint32 size);
  bool AsyncWriteRepMessage(reply &reply);
  bool StartReadNextPacket();

  virtual void            Close();
  const SocketAddress     remote_addr();
  bool                    IsOpen() const;
  bool                    IsClose();
  // Socket::Ptr             SocketNumber();

  void DumpHttpReqMessage(HttpReqMessage &http_req_message);
  reply &http_reply() {
    return reply_;
  }
  bool ResponseWithStockReply(reply::status_type status);

 private:
  bool AnalisysPacket(MemBuffer::Ptr buffer);
  void LiveSignalClose(int error_code, bool is_signal);
  void SignalClose(int error_code, bool is_signal);

  void OnAsyncSocketWriteEvent(AsyncSocket::Ptr socket);

  void OnAsyncSocketReadEvent(AsyncSocket::Ptr socket,
                              MemBuffer::Ptr data_buffer);
  void OnAsyncSocketErrorEvent(AsyncSocket::Ptr socket,
                               int error_code);
 public:
  int OnHttpMessageBegin(http_parser *parser);
  int OnHttpUrl(http_parser *parser,
                const char *at,
                std::size_t length);
  int OnHttpStatus(http_parser *parser,
                   const char *at,
                   std::size_t length);
  int OnHttpHeaderField(http_parser *parser,
                        const char *at,
                        std::size_t length);
  int OnHttpHeaderValue(http_parser *parser,
                        const char *at,
                        std::size_t length);
  int OnHttpHeadersComplete(http_parser *parser);
  int OnHttpBody(http_parser *parser,
                 const char *at,
                 std::size_t length);
  int OnHttpMessageComplete(http_parser *parser);
  int OnHttpChunkHeader(http_parser *parser);
  int OnHttpChunkComplete(http_parser *parser);
  void ParserRequestURL(const char *req, uint32 size, KeyValues &key_values);
 private:
  AsyncSocket::Ptr      async_socket_;
  char                  recv_buffer_[DEFAULT_BLOCK_SIZE * 4];
  uint32                recv_size_;
  http_parser_settings  http_settings_;
  http_parser           http_parser_;
  HttpReqMessage        http_req_message_;
  reply                 reply_;
};

}  // namespace vzes

#endif  // EVENTSERVICE_NET_ASYNC_HTTP_SOCKET_H_
