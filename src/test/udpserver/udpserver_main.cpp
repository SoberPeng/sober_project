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
#include "eventservice/net/eventservice.h"
#include "eventservice/base/logging.h"

const char NOT_FOUND[] =
  "HTTP/1.0 200 OK\r\n"
  "Server:Apache Tomcat/5.0.12\r\n"
  "Content-Type:text/html\r\n\r\n"
  "<html>"
  "<head><title>Not Found</title></head>"
  "<body><h1>404 Not Found</h1></body>"
  "</html>";

class UdpServer : public boost::noncopyable,
  public boost::enable_shared_from_this<UdpServer>,
  public sigslot::has_slots<> {
 public:
  UdpServer(vzes::EventService::Ptr event_service)
    : event_service_(event_service) {
  }

  bool Start() {
    ASSERT_RETURN_FAILURE(!event_service_, false);
    vzes::SocketAddress bind_addr("0.0.0.0", 22100);
    async_udp_socket_ = event_service_->CreateUdpServer(bind_addr);
    ASSERT_RETURN_FAILURE(!async_udp_socket_, false);

    //async_udp_socket_->SignalSocketWriteEvent.connect(
    //  this, &UdpServer::OnSocketWriteComplete);
    async_udp_socket_->SignalSocketReadEvent.connect(
      this, &UdpServer::OnSocketReadComplete);
    async_udp_socket_->SignalSocketErrorEvent.connect(
      this, &UdpServer::OnSocketErrorEvent);
    return async_udp_socket_->AsyncRead();
  }
 private:
  ///
  void OnSocketWriteComplete(vzes::AsyncUdpSocket::Ptr async_socket) {
    LOG(L_INFO) << "Socket Write Event";
  }

  void OnSocketReadComplete(vzes::AsyncUdpSocket::Ptr async_socket,
                            vzes::MemBuffer::Ptr data,
                            const vzes::SocketAddress &remote_addr) {
    LOG(L_INFO) << data->size();
    async_socket->SendTo(NOT_FOUND, strlen(NOT_FOUND), remote_addr);
    async_socket->AsyncRead();
  }

  void OnSocketErrorEvent(vzes::AsyncUdpSocket::Ptr async_socket,
                          int err) {
    LOG(L_ERROR) << "Error event";
    async_socket->Close();
  }
 private:
  vzes::EventService::Ptr       event_service_;
  vzes::AsyncUdpSocket::Ptr     async_udp_socket_;
};

int main(void) {
  // Initialize the logging system
  vzes::LogMessage::LogTimestamps(true);
  vzes::LogMessage::LogContext(vzes::LS_INFO);
  vzes::LogMessage::LogThreads(true);

  vzes::EventService::Ptr event_service =
    vzes::EventService::CreateCurrentEventService("UdpServer");
  UdpServer tcp_server(event_service);
  tcp_server.Start();
  event_service->Run();

  return EXIT_SUCCESS;
}