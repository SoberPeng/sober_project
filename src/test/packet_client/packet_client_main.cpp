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
#include "eventservice/net/asyncpacketsocket.h"


const char NOT_FOUND[] =
  "HTTP/1.0 200 OK\r\n"
  "Server:Apache Tomcat/5.0.12\r\n"
  "Content-Type:text/html\r\n\r\n"
  "<html>"
  "<head><title>Not Found</title></head>"
  "<body><h1>"
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
  "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
  "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
  "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
  "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
  "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
  "ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg"
  "hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj"
  "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk"
  "lllllllllllllllllllllllllllllllllllllllllllllllllllllllllll"
  "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm"
  "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn"
  "ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
  "404 Not Found</h1></body>"
  "</html>";

class TcpClient : public boost::noncopyable,
  public boost::enable_shared_from_this<TcpClient>,
  public sigslot::has_slots<> {
 public:
  TcpClient(vzes::EventService::Ptr event_service)
    : event_service_(event_service) {
  }
  bool Start() {
    ASSERT_RETURN_FAILURE(async_client_, false);

    async_client_ = event_service_->CreateAsyncConnect();
    vzes::SocketAddress address("127.0.0.1", 5544);

    async_client_->SignalServerConnected.connect(
      this, &TcpClient::OnConnectEvent);

    return async_client_->Connect(address, 10000);
  }

 private:
  void OnConnectEvent(vzes::AsyncConnecter::Ptr client,
                      vzes::Socket::Ptr s,
                      int err) {

    if (err != 0 || !s) {
      LOG(L_ERROR) << "Connect remote failure";
      return ;
    }


    vzes::AsyncSocket::Ptr socket = event_service_->CreateAsyncSocket(s);

    LOG(L_INFO) << "Accept remote socket " << err;
    vzes::AsyncPacketSocket::Ptr ap_socket(
      new vzes::AsyncPacketSocket(event_service_, socket));

    LOG(L_INFO) << "Accept remote socket";
    if (socket && socket->IsConnected()) {
      //
      ap_socket->SignalPacketWrite.connect(
        this, &TcpClient::OnSocketWriteComplete);
      ap_socket->SignalPacketEvent.connect(
        this, &TcpClient::OnSocketPacketEvent);
      ap_socket->SignalPacketError.connect(
        this, &TcpClient::OnSocketErrorEvent);

      vzes::MemBuffer::Ptr send_buffer = vzes::MemBuffer::CreateMemBuffer();
      send_buffer->WriteBytes(NOT_FOUND, strlen(NOT_FOUND));
      ap_socket->AsyncWritePacket(send_buffer, 0);
      ap_socket->AsyncRead();
      ap_sockets_.push_back(ap_socket);
    }
  }

  ///
  void OnSocketWriteComplete(vzes::AsyncPacketSocket::Ptr ap_socket) {
    LOG(L_INFO) << "Socket Write Event";
    LOG(L_INFO) << "=========================================================";
    // ap_socket->AsyncRead();
  }

  void OnSocketPacketEvent(vzes::AsyncPacketSocket::Ptr ap_socket,
                           vzes::MemBuffer::Ptr data,
                           uint16 flag) {
    LOG(L_INFO) << "Socket Read Event, size = " << data->size();
    LOG(L_INFO) << ".........................................................";
    //LOG(L_INFO).write(data, data_size);
    ap_socket->AsyncWritePacket(NOT_FOUND, strlen(NOT_FOUND), 0);
    ap_socket->AsyncRead();
  }

  void OnSocketErrorEvent(vzes::AsyncPacketSocket::Ptr ap_socket,
                          int err) {
    ap_socket->Close();
    ap_sockets_.remove(ap_socket);
  }
 private:
  vzes::EventService::Ptr                             event_service_;
  vzes::AsyncConnecter::Ptr                           async_client_;
  typedef std::list<vzes::AsyncPacketSocket::Ptr>     APSockets;
  APSockets                                           ap_sockets_;
};

int main(void) {
  // Initialize the logging system
  vzes::LogMessage::LogTimestamps(true);
  vzes::LogMessage::LogContext(vzes::LS_INFO);
  vzes::LogMessage::LogThreads(true);

  vzes::EventService::Ptr event_service =
    vzes::EventService::CreateCurrentEventService("PacketClient");
  TcpClient tcp_client(event_service);
  tcp_client.Start();
  event_service->Run();

  return EXIT_SUCCESS;
}
