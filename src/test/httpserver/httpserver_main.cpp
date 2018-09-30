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
#include "eventservice/net/asynchttpsocket.h"
#include "eventservice/http/handlermanager.h"
#include "eventservice/http/file_handler.h"
#include "eventservice/net/networktinterface.h"
#include <vector>

#ifdef WIN32
#define DEFAULT_ROOT_DIR "E:/work/stp/html/pages"
#else
#define DEFAULT_ROOT_DIR "/mnt/hgfs/project/stp_proxy/stp_web/html/pages/"
#endif
class HTTPServer : public sigslot::has_slots<> {
 public:
  HTTPServer(vzes::EventService::Ptr event_service)
    : event_service_(event_service) {
  }

  bool InitHttpServer() {
    listener_ = event_service_->CreateAsyncListener();
    vzes::SocketAddress listen_addr("0.0.0.0", 8080);
    listener_->SignalNewConnected.connect(
      this, &HTTPServer::OnListenerAccpetEvent);
    handler_manager_.reset(new vzes::HandlerManager());
    vzes::HttpHandler::Ptr file_handle(
      new vzes::file_handler(event_service_, DEFAULT_ROOT_DIR));
    handler_manager_->SetDefualtHandler(file_handle);
    return listener_->Start(listen_addr);
  }
 private:
  void OnListenerAccpetEvent(vzes::AsyncListener::Ptr listener,
                             vzes::Socket::Ptr socket,
                             int err) {

    vzes::AsyncSocket::Ptr as = event_service_->CreateAsyncSocket(socket);
    ASSERT_RETURN_VOID(!as);
    vzes::SocketAddress remote_addr = as->GetRemoteAddress();
    vzes::AsyncHttpSocket::Ptr http_socket(
      new vzes::AsyncHttpSocket(as, remote_addr));

    async_http_sockets_.push_back(http_socket);

    http_socket->SignalHttpPacketWrite.connect(
      this, &HTTPServer::OnHttpSocketWrite);
    http_socket->SignalHttpPacketEvent.connect(
      this, &HTTPServer::OnHttpSocketEvent);
    http_socket->SignalHttpPacketError.connect(
      this, &HTTPServer::OnHttpSocketError);

    LOG(L_WARNING) << "AsyncPacketSocket = " << async_http_sockets_.size();
    http_socket->StartReadNextPacket();
    return ;
  }

  void OnHttpSocketWrite(vzes::AsyncHttpSocket::Ptr async_socket) {
    LOG(L_INFO) << "Socket Write Event";
    async_socket->Close();
    RemoveSocket(async_socket);
  }

  void OnHttpSocketEvent(vzes::AsyncHttpSocket::Ptr async_socket,
                         vzes::HttpReqMessage &http_req_message) {
    async_socket->DumpHttpReqMessage(http_req_message);
    handler_manager_->OnNewRequest(async_socket, http_req_message);
  }

  void OnHttpSocketError(vzes::AsyncHttpSocket::Ptr async_socket,
                         int error_code) {
    RemoveSocket(async_socket);
  }

  void RemoveSocket(vzes::AsyncHttpSocket::Ptr async_socket) {
    for (std::size_t i = 0; i < async_http_sockets_.size(); i++) {
      if (async_http_sockets_[i] == async_socket) {
        async_http_sockets_.erase(async_http_sockets_.begin() + i);
      }
    }
    LOG(L_WARNING) << "AsyncClients size = " << async_http_sockets_.size();
  }
 private:
  vzes::AsyncListener::Ptr                         listener_;
  vzes::EventService::Ptr                          event_service_;
  std::vector<vzes::AsyncHttpSocket::Ptr>          async_http_sockets_;
  vzes::HandlerManager::Ptr                        handler_manager_;
};

int main(void) {
  // Initialize the logging system
  vzes::LogMessage::LogTimestamps(true);
  vzes::LogMessage::LogContext(vzes::LS_INFO);
  vzes::LogMessage::LogThreads(true);


  vzes::EventService::Ptr event_service =
    vzes::EventService::CreateCurrentEventService("HttpServer");
  HTTPServer http_server(event_service);
  http_server.InitHttpServer();
  event_service->Run();

  return EXIT_SUCCESS;
}