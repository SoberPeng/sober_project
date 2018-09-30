/*
* vzes
* Copyright 2013 - 2016, Vzenith Inc.
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

#include "eventservice/dp/dpnetserver.h"

namespace vzes {

class DpClientManager;
extern DpClientManager *GetDpServiceInstance();

DpNetServer::DpNetServer(EventService::Ptr event_service)
  : event_service_(event_service) {
}

bool DpNetServer::Start(const char *host, unsigned short port)
{
  ASSERT_RETURN_FAILURE(listenser_, false);

  listenser_ = event_service_->CreateAsyncListener();
  ASSERT_RETURN_FAILURE(listenser_.get() == NULL, false);

  SocketAddress address(host, port);
  listenser_->SignalNewConnected.connect(this,
    &DpNetServer::OnLisenerAcceptEvent);

  return listenser_->Start(address, false);
}

void DpNetServer::OnLisenerAcceptEvent(AsyncListener::Ptr listener, Socket::Ptr s, int err)
{
  AsyncSocket::Ptr async_socket = event_service_->CreateAsyncSocket(s);

  LOG(L_INFO) << "Accept remote socket";
  if (async_socket && async_socket->IsConnected()) {
    //
    async_socket->SignalSocketWriteEvent.connect(this,
      &DpNetServer::OnSocketWriteComplete);
    async_socket->SignalSocketReadEvent.connect(this,
      &DpNetServer::OnSocketReadComplete);
    async_socket->SignalSocketErrorEvent.connect(this,
      &DpNetServer::OnSocketErrorEvent);

    async_socket->AsyncRead();

    sockets_.push_back(async_socket);
  }
}

void DpNetServer::OnLisenerErrorEvent(AsyncListener::Ptr listener, int err)
{
  LOG(L_INFO) << "OnLisenerErrorEvent";
}

void DpNetServer::OnSocketWriteComplete(AsyncSocket::Ptr async_socket)
{
  LOG(L_INFO) << "Socket Write Event";
}

void DpNetServer::OnSocketReadComplete(AsyncSocket::Ptr async_socket, MemBuffer::Ptr data)
{
  // LOG(L_INFO).write(data->buffer_, data->buffer_size_);
  // LOG(L_INFO) << data->size();
  // async_socket->AsyncWrite(NOT_FOUND, strlen(NOT_FOUND));
  async_socket->AsyncRead();
}

void DpNetServer::OnSocketErrorEvent(AsyncSocket::Ptr async_socket, int err)
{
  async_socket->Close();
  sockets_.remove(async_socket);
}

}

