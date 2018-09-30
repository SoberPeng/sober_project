/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#include "eventservice/net/networkudpimpl.h"

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#else
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#endif

#define TIMEOUT_TIMES 4
#ifdef WIN32
#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
void bzero(void* data, int size) {
  memset(data, 0, size);
}
void bcopy(void* des, const void* src, int size) {
  memcpy(des, src, size);
}
#endif

namespace vzes {

AsyncUdpSocketImpl::AsyncUdpSocketImpl(EventService::Ptr es)
  : event_service_(es) {
}

AsyncUdpSocketImpl::~AsyncUdpSocketImpl() {
  LOG(L_INFO) << "Destory socket implement";
  Close();
}

bool AsyncUdpSocketImpl::Init() {
  ASSERT_RETURN_FAILURE(socket_, false);
  ASSERT_RETURN_FAILURE(!event_service_, false);
  ASSERT_RETURN_FAILURE(socket_event_, false);

  socket_ = event_service_->CreateSocket(UDP_SOCKET);
  ASSERT_RETURN_FAILURE(!socket_, false);
  socket_event_ = event_service_->CreateDispEvent(socket_,
                  DE_READ | DE_CLOSE);
  socket_event_->SignalEvent.connect(this, &AsyncUdpSocketImpl::OnSocketEvent);
  ASSERT_RETURN_FAILURE(!socket_event_, false);
  return true;
}

bool AsyncUdpSocketImpl::Bind(const SocketAddress &bind_addr) {
  ASSERT_RETURN_FAILURE(!socket_, false);
  ASSERT_RETURN_FAILURE(!socket_event_, false);
  return socket_->Bind(bind_addr) != SOCKET_ERROR;
}
// Inherit with AsyncSocket
// Async write the data, if the operator complete, SignalWriteCompleteEvent
// will be called
bool AsyncUdpSocketImpl::SendTo(MemBuffer::Ptr buffer,
                                const SocketAddress &addr) {
  ASSERT_RETURN_FAILURE(!IsConnected(), false);
  ASSERT_RETURN_FAILURE(!event_service_, false);
  ASSERT_RETURN_FAILURE(!socket_event_, false);

  BlocksPtr &blocks = buffer->blocks();

  for (BlocksPtr::iterator iter = blocks.begin();
       iter != blocks.end(); iter++) {
    Block::Ptr block = *iter;
    int res = socket_->SendTo(block->buffer, block->buffer_size, addr);
    if (res != block->buffer_size) {
      return false;
    }
  }
  return true;
}

bool AsyncUdpSocketImpl::AsyncRead() {
  ASSERT_RETURN_FAILURE(!socket_, false);
  ASSERT_RETURN_FAILURE(!event_service_, false);
  ASSERT_RETURN_FAILURE(!socket_event_, false);
  socket_event_->AddEvent(DE_READ);
  return event_service_->Add(socket_event_);
}

// Returns the address to which the socket is bound.  If the socket is not
// bound, then the any-address is returned.
SocketAddress AsyncUdpSocketImpl::GetLocalAddress() const {
  return socket_->GetLocalAddress();
}

// Returns the address to which the socket is connected.  If the socket is
// not connected, then the any-address is returned.
SocketAddress AsyncUdpSocketImpl::GetRemoteAddress() const {
  return socket_->GetRemoteAddress();
}

void AsyncUdpSocketImpl::Close() {
  if (socket_) {
    socket_->Close();
    socket_.reset();
  }
  if (socket_event_) {
    socket_event_->Close();
    event_service_->Remove(socket_event_);
    socket_event_.reset();
  }
  RemoveAllSignal();
}

int AsyncUdpSocketImpl::GetError() const {
  ASSERT_RETURN_FAILURE(!socket_, 0);
  return socket_->GetError();
}

void AsyncUdpSocketImpl::SetError(int error) {
  ASSERT_RETURN_VOID(!socket_);
  return socket_->SetError(error);
}

bool AsyncUdpSocketImpl::IsConnected() {
  ASSERT_RETURN_FAILURE(!socket_, false);
  return socket_->GetState() != Socket::CS_CLOSED;
}

int AsyncUdpSocketImpl::GetOption(Option opt, int* value) {
  ASSERT_RETURN_FAILURE(!socket_, -1);
  return socket_->GetOption(opt, value);
}

int AsyncUdpSocketImpl::SetOption(Option opt, int value) {
  ASSERT_RETURN_FAILURE(!socket_, -1);
  return socket_->SetOption(opt, value);
}

///
void AsyncUdpSocketImpl::OnSocketEvent(EventDispatcher::Ptr accept_event,
                                       Socket::Ptr socket,
                                       uint32 event_type,
                                       int err) {
  AsyncUdpSocketImpl::Ptr live_this = shared_from_this();
  // LOG(L_INFO) << "Event Type = " << event_type;
  if(err || (event_type & DE_CLOSE)) {
    LOG(L_ERROR) << "Close event";
    SocketErrorEvent(err);
    return ;
  }
  if (event_type & DE_WRITE) {
    socket_event_->RemoveEvent(DE_WRITE);
    // LOG(L_INFO) << "Socket Write Event " << event_type;
    if (!socket_event_) {
      return ;
    }
  }
  if (event_type & DE_READ) {
    // LOG(L_INFO) << "Socket Read Event " << event_type;
    SocketReadEvent();
  }
}

void AsyncUdpSocketImpl::SocketReadEvent() {
  MemBuffer::Ptr buffer = MemBuffer::CreateMemBuffer();
  SocketAddress   remote_addr;
  int res = socket_->RecvFrom(buffer, &remote_addr);
  int error_code = socket_->GetError();

  if (res > 0) {
    SocketReadComplete(buffer, remote_addr);
  } else if (res == 0) {
    // LOG(L_ERROR) << evutil_socket_error_to_string(error_code);
    // Socket已经关闭了
    BOOST_ASSERT(error_code == 0);
    // SocketReadComplete(MemBuffer::Ptr(), error_code);
    SocketErrorEvent(error_code);
  } else {
    // 接收数据出问题了
    if (IsBlockingError(error_code)) {
      // 数据阻塞，之后继续接收数据
      return;
    } else {
      // Socket出了问题
      // SocketReadComplete(MemBuffer::Ptr(), error_code);
      SocketErrorEvent(error_code);
    }
  }
}

void AsyncUdpSocketImpl::SocketErrorEvent(int err) {
  AsyncUdpSocket::Ptr live_this = shared_from_this();
  SignalSocketErrorEvent(live_this, err);
  Close();
}

////////////////////////////////////////////////////////////////////////////////

void AsyncUdpSocketImpl::SocketReadComplete(MemBuffer::Ptr buffer,
    const SocketAddress &addr) {
  // 确保生命周期内不会因为外部删除而删除了整个对象
  // 如果是Socket Read事件导致的Socket错误，并不会出问题，
  // 但是如果在Write过程中出现了错误，那就会有问题了
  AsyncUdpSocketImpl::Ptr live_this = shared_from_this();
  SignalSocketReadEvent(live_this, buffer, addr);
}

////////////////////////////////////////////////////////////////////////////////


MulticastAsyncUdpSocket::MulticastAsyncUdpSocket(EventService::Ptr es)
  :AsyncUdpSocketImpl(es) {
}

bool MulticastAsyncUdpSocket::Bind(const SocketAddress &bind_addr) {
  SocketAddress multicast_bind_addr(INADDR_ANY, bind_addr.port());
  multicast_ip_addr_ = bind_addr.ipaddr().ToString();
  ChangeMulticastMembership(socket_, multicast_ip_addr_.c_str());
  return AsyncUdpSocketImpl::Bind(multicast_bind_addr);
}

bool MulticastAsyncUdpSocket::AsyncRead() {
  // ChangeMulticastMembership(socket_, multicast_ip_addr_.c_str());
  return AsyncUdpSocketImpl::AsyncRead();
}

#ifdef WIN32
int MulticastAsyncUdpSocket::ChangeMulticastMembership(
  Socket::Ptr socket, const char* multicast_addr) {
  struct ip_mreq      mreq;
  bzero(&mreq, sizeof(struct ip_mreq));
  std::vector<NetworkInterfaceState> net_inter;
  UpdateNetworkInterface(net_inter);
  for (std::size_t i = 0; i < net_inter.size(); i++) {
    // Find network interface
    bool is_found = false;
    for (std::size_t j = 0; j < network_interfaces_.size(); j++) {
      if (network_interfaces_[j].adapter_name == net_inter[i].adapter_name
          && network_interfaces_[j].ip_addr.S_un.S_addr ==
          net_inter[i].ip_addr.S_un.S_addr
          && network_interfaces_[j].state == INTERFACE_STATE_ADD) {
        is_found = true;
        break;
      }
    }
    if (!is_found) {
      //LOG(INFO) << network_interfaces_[i].adapter_name << " : ";
      mreq.imr_interface.s_addr = net_inter[i].ip_addr.S_un.S_addr;
      mreq.imr_multiaddr.s_addr = inet_addr(multicast_addr);
      if (socket_->SetOption(IPPROTO_IP, IP_ADD_MEMBERSHIP,
                             (const char *)&mreq,
                             sizeof(struct ip_mreq)) == -1) {
        net_inter[i].state = INTERFACE_STATE_ERROR;
        // LOG(ERROR) << "IP_ADD_MEMBERSHIP " << inet_ntoa(net_inter[i].ip_addr);
      } else {
        LOG(L_INFO) << "IP_ADD_MEMBERSHIP " << inet_ntoa(net_inter[i].ip_addr);
        net_inter[i].state = INTERFACE_STATE_ADD;
      }
    }
    if (net_inter[i].state != INTERFACE_STATE_NONE) {
      continue;
    }
  }
  network_interfaces_ = net_inter;
  return 0;
}

int MulticastAsyncUdpSocket::UpdateNetworkInterface(
  std::vector<NetworkInterfaceState>& net_inter) {
  /* Declare and initialize variables */
  DWORD dwSize = 0;
  DWORD dwRetVal = 0;
  unsigned int i = 0;
  // Set the flags to pass to GetAdaptersAddresses
  ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
  // default to unspecified address family (both)
  ULONG family = AF_INET;
  LPVOID lpMsgBuf = NULL;
  PIP_ADAPTER_ADDRESSES pAddresses = NULL;
  ULONG outBufLen = WORKING_BUFFER_SIZE;
  ULONG Iterations = 0;
  PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
  PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
  PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
  PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
  IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;
  IP_ADAPTER_PREFIX *pPrefix = NULL;
  // char temp_ip[128];
  do {
    pAddresses = (IP_ADAPTER_ADDRESSES *)MALLOC(outBufLen);
    if (pAddresses == NULL) {
      printf
      ("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
      return 1;
    }
    dwRetVal =
      GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
      FREE(pAddresses);
      pAddresses = NULL;
    } else {
      break;
    }
    Iterations++;
  } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));
  // -------------------------------------------------------------------------
  if (dwRetVal == NO_ERROR) {
    // If successful, output some information from the data we received
    pCurrAddresses = pAddresses;
    while (pCurrAddresses) {
      //printf("\tAdapter name: %s\n", pCurrAddresses->AdapterName);
      pUnicast = pCurrAddresses->FirstUnicastAddress;
      if (pUnicast != NULL) {
        for (i = 0; pUnicast != NULL; i++) {
          if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
            NetworkInterfaceState nif;
            sockaddr_in* v4_addr =
              reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr);
            //printf("\tIpv4 pUnicast : %s\n", hexToCharIP(temp_ip, v4_addr->sin_addr));
            nif.ip_addr = v4_addr->sin_addr;
            nif.adapter_name = pCurrAddresses->AdapterName;
            nif.state = InterfaceState::INTERFACE_STATE_NONE;
            net_inter.push_back(nif);
          }
          pUnicast = pUnicast->Next;
        }
      }
      pCurrAddresses = pCurrAddresses->Next;
    }
  } else {
    if (dwRetVal != ERROR_NO_DATA) {
      if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                        |FORMAT_MESSAGE_FROM_SYSTEM
                        | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        dwRetVal,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        // Default language
                        (LPTSTR)& lpMsgBuf, 0, NULL)) {
        printf("\tError: %s", (const char *)lpMsgBuf);
        LocalFree(lpMsgBuf);
        if (pAddresses)
          FREE(pAddresses);
        return 1;
      }
    }
  }
  if (pAddresses) {
    FREE(pAddresses);
  }
  return 0;
}
#else
int MulticastAsyncUdpSocket::ChangeMulticastMembership(Socket::Ptr socket,
    const char* multicast_addr) {
  struct ip_mreq      mreq;
  bzero(&mreq, sizeof(struct ip_mreq));
  mreq.imr_interface.s_addr = INADDR_ANY;
  mreq.imr_multiaddr.s_addr = inet_addr(multicast_addr);
  if (socket_->SetOption(IPPROTO_IP, IP_ADD_MEMBERSHIP,
                         (const char *)&mreq,
                         sizeof(struct ip_mreq)) == -1) {
    LOG(L_ERROR) << "Add membership error";
    //LOG(ERROR) << "IP_ADD_MEMBERSHIP " << inet_ntoa(net_inter[i].ip_addr);
  }
  return 0;
}
#endif
}  // namespace vzes
