/*
 * vzes
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


#if defined(_MSC_VER) && _MSC_VER < 1300
#pragma warning(disable:4786)
#endif


#include <cassert>


#ifdef POSIX
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#ifndef LITEOS
#include <sys/epoll.h>
#endif
#endif

#ifdef WIN32
// #define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#undef SetPort
#endif

#ifdef POSIX
#include <netinet/tcp.h>  // for TCP_NODELAY
#define IP_MTU 14 // Until this is integrated from linux/in.h to netinet/in.h
typedef void* SockOptArg;
#endif  // POSIX

#include <algorithm>
#include <map>

#include "eventservice/base/basictypes.h"
#include "eventservice/base/byteorder.h"
#include "eventservice/base/common.h"
#include "eventservice/base/logging.h"
#include "eventservice/base/nethelpers.h"
#include "eventservice/net/networkservice.h"
#include "eventservice/base/timeutils.h"
#include "eventservice/base/winping.h"
#include "eventservice/base/win32socketinit.h"
#include "eventservice/net/networktinterfaceimpl.h"

#ifdef WIN32
typedef char* SockOptArg;
#endif

namespace vzes {

// Standard MTUs, from RFC 1191
const uint16 PACKET_MAXIMUMS[] = {
  65535,    // Theoretical maximum, Hyperchannel
  32000,    // Nothing
  17914,    // 16Mb IBM Token Ring
  8166,     // IEEE 802.4
  //4464,   // IEEE 802.5 (4Mb max)
  4352,     // FDDI
  //2048,   // Wideband Network
  2002,     // IEEE 802.5 (4Mb recommended)
  //1536,   // Expermental Ethernet Networks
  //1500,   // Ethernet, Point-to-Point (default)
  1492,     // IEEE 802.3
  1006,     // SLIP, ARPANET
  //576,    // X.25 Networks
  //544,    // DEC IP Portal
  //512,    // NETBIOS
  508,      // IEEE 802/Source-Rt Bridge, ARCNET
  296,      // Point-to-Point (low delay)
  68,       // Official minimum
  0,        // End of list marker
};

static const int IP_HEADER_SIZE = 20u;
static const int IPV6_HEADER_SIZE = 40u;
static const int ICMP_HEADER_SIZE = 8u;
static const int ICMP_PING_TIMEOUT_MILLIS = 10000u;

PhysicalSocket::PhysicalSocket(SOCKET s)
  : s_(s), enabled_events_(0), error_(0),
    state_((s == INVALID_SOCKET) ? CS_CLOSED : CS_CONNECTED) {
#ifdef WIN32
  // EnsureWinsockInit() ensures that winsock is initialized. The default
  // version of this function doesn't do anything because winsock is
  // initialized by constructor of a static object. If neccessary vzes
  // users can link it with a different version of this function by replacing
  // win32socketinit.cc. See win32socketinit.cc for more details.
  EnsureWinsockInit();
#endif
  if (s_ != INVALID_SOCKET) {
    enabled_events_ = DE_READ | DE_WRITE ;

    int type = SOCK_STREAM;
    socklen_t len = sizeof(type);
    VZ_VERIFY(0 == getsockopt(s_, SOL_SOCKET, SO_TYPE, (SockOptArg)&type, &len));
    udp_ = (SOCK_DGRAM == type);
  }
}

PhysicalSocket::~PhysicalSocket() {
  Close();
}

// Creates the underlying OS socket (same as the "socket" function).
bool PhysicalSocket::Create(int family, int type) {
  Close();
  s_ = ::socket(family, type, 0);
  udp_ = (SOCK_DGRAM == type);
  UpdateLastError();
  if (s_ == INVALID_SOCKET) {
    LOG(L_ERROR) << "create socket failed, error= " << error_;
  }
  if (udp_) {
    enabled_events_ = DE_READ | DE_WRITE;
    state_ = CS_CONNECTED;
  }
  SetSocketNonblock();
  return s_ != INVALID_SOCKET;
}


bool PhysicalSocket::SetSocketNonblock() {
  BOOST_ASSERT(s_ != INVALID_SOCKET);
  // Must be a non-blocking
#ifdef WIN32
  u_long argp = 1;
  ioctlsocket(s_, FIONBIO, &argp);
#else
  fcntl(s_, F_SETFL, fcntl(s_, F_GETFL, 0) | O_NONBLOCK);
#endif
  return true;
}

SocketAddress PhysicalSocket::GetLocalAddress() const {
  sockaddr_storage addr_storage = {0};
  socklen_t addrlen = sizeof(addr_storage);
  sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
  int result = ::getsockname(s_, addr, &addrlen);
  SocketAddress address;
  if (result >= 0) {
    SocketAddressFromSockAddrStorage(addr_storage, &address);
  } else {
    LOG(LS_WARNING) << "GetLocalAddress: unable to get local addr, socket="
                    << s_;
  }
  return address;
}

SocketAddress PhysicalSocket::GetRemoteAddress() const {
  sockaddr_storage addr_storage = {0};
  socklen_t addrlen = sizeof(addr_storage);
  sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
  int result = ::getpeername(s_, addr, &addrlen);
  SocketAddress address;
  if (result >= 0) {
    SocketAddressFromSockAddrStorage(addr_storage, &address);
  } else {
    LOG(LS_WARNING) << "GetRemoteAddress: unable to get remote addr, socket="
                    << s_;
  }
  return address;
}

int PhysicalSocket::Bind(const SocketAddress& bind_addr) {
  sockaddr_storage addr_storage;
  size_t len = bind_addr.ToSockAddrStorage(&addr_storage);
  sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
  int err = ::bind(s_, addr, static_cast<int>(len));
  UpdateLastError();
  if (err == -1) {
    LOG(L_ERROR) << "socket bind failed, error= " << error_;
  }
#ifdef _DEBUG
  if (0 == err) {
    dbg_addr_ = "Bound @ ";
    dbg_addr_.append(GetLocalAddress().ToString());
  }
#endif  // _DEBUG
  return err;
}

int PhysicalSocket::Connect(const SocketAddress& addr) {
  // TODO: Implicit creation is required to reconnect...
  // ...but should we make it more explicit?
  if (state_ != CS_CLOSED) {
    SetError(EALREADY);
    return SOCKET_ERROR;
  }
  return DoConnect(addr);
}

int PhysicalSocket::DoConnect(const SocketAddress& connect_addr) {
  if ((s_ == INVALID_SOCKET) &&
      !Create(connect_addr.family(), SOCK_STREAM)) {
    return SOCKET_ERROR;
  }
  sockaddr_storage addr_storage;
  size_t len = connect_addr.ToSockAddrStorage(&addr_storage);
  sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
  int err = ::connect(s_, addr, static_cast<int>(len));
  UpdateLastError();
  if (err == 0) {
    state_ = CS_CONNECTED;
  } else if (IsBlockingError(error_)) {
    state_ = CS_CONNECTING;
    enabled_events_ |= DE_CONNECT;
  } else {
    LOG(L_ERROR) << "socket connect failed, error= " << error_;
    return SOCKET_ERROR;
  }

  enabled_events_ |= DE_READ | DE_WRITE;
  return 0;
}

int PhysicalSocket::GetOption(Option opt, int* value) {
  int slevel;
  int sopt;
  if (TranslateOption(opt, &slevel, &sopt) == -1)
    return -1;
  socklen_t optlen = sizeof(*value);
  int ret = ::getsockopt(s_, slevel, sopt, (SockOptArg)value, &optlen);
  if (ret != -1 && opt == OPT_DONTFRAGMENT) {
#ifdef LINUX
    *value = (*value != IP_PMTUDISC_DONT) ? 1 : 0;
#endif
  }
  return ret;
}

int PhysicalSocket::SetOption(Option opt, int value) {
  int slevel;
  int sopt;
  if (TranslateOption(opt, &slevel, &sopt) == -1)
    return -1;
  if (opt == OPT_DONTFRAGMENT) {
#ifdef LINUX
    value = (value) ? IP_PMTUDISC_DO : IP_PMTUDISC_DONT;
#endif
  }
  return ::setsockopt(s_, slevel, sopt, (SockOptArg)&value, sizeof(value));
}

int PhysicalSocket::SetOption(int level,
                              int optname,
                              const char *value,
                              int optlen) {
  return ::setsockopt(s_, level, optname, value, optlen);
}

int PhysicalSocket::Send(const void *pv, size_t cb) {
  int sent = ::send(s_, reinterpret_cast<const char *>(pv), (int)cb,
#ifdef LINUX
                    // Suppress SIGPIPE. Without this, attempting to send on a socket whose
                    // other end is closed will result in a SIGPIPE signal being raised to
                    // our process, which by default will terminate the process, which we
                    // don't want. By specifying this flag, we'll just get the error EPIPE
                    // instead and can handle the error gracefully.
                    MSG_NOSIGNAL
#else
                    0
#endif
                   );
  UpdateLastError();
  // We have seen minidumps where this may be false.
  ASSERT(sent <= static_cast<int>(cb));
  // if ((sent < 0) && IsBlockingError(error_)) {
  enabled_events_ |= DE_WRITE;
  // }
  return sent;
}

int PhysicalSocket::Send(MemBuffer::Ptr buffer) {
  int res = 0;
  BlocksPtr &blocks = buffer->blocks();
  BlocksPtr::iterator iter = blocks.begin();
  while (iter != blocks.end()) {
    Block::Ptr block = *iter;
    int sent = Send(block->buffer, block->buffer_size);
    if (sent == block->buffer_size) {
      // 把Block的size清空
      block->buffer_size = 0;
      // 删除这个Blocks
      iter = blocks.erase(iter);
      // 更新发送长度
      res += sent;
      continue;
    } else if (sent > 0 && sent < block->buffer_size) {
      // 证明发送了一部分数据
      block->ReadBytes(NULL, sent);
      res += sent;
      break;
    } else if (sent == SOCKET_ERROR) {
      break;
    } else if (sent == 0) {
      break;
    }
  }

  if (res) {
    buffer->ReduceSize(res);
  }
  return res;
}

int PhysicalSocket::SendTo(const void* buffer,
                           size_t length,
                           const SocketAddress& addr) {
  sockaddr_storage saddr;
  size_t len = addr.ToSockAddrStorage(&saddr);
  int sent = ::sendto(
               s_, static_cast<const char *>(buffer), static_cast<int>(length),
#ifdef LINUX
               // Suppress SIGPIPE. See above for explanation.
               MSG_NOSIGNAL,
#else
               0,
#endif
               reinterpret_cast<sockaddr*>(&saddr), static_cast<int>(len));
  UpdateLastError();
  // We have seen minidumps where this may be false.
  ASSERT(sent <= static_cast<int>(length));
  if ((sent < 0) && IsBlockingError(error_)) {
    enabled_events_ |= DE_WRITE;
  }
  return sent;
}

int PhysicalSocket::Recv(void* buffer, size_t length) {
  int received = ::recv(s_, static_cast<char*>(buffer),
                        static_cast<int>(length), 0);
  bool success = false;
  UpdateLastError();
  if (received == 0) {
    // Connection reset by peer
    LOG(LS_WARNING) << "EOF from socket; deferring close event";
    return received;
  } else if (received < 0) {
    if (IsBlockingError(error_)) {
      // No messages are available
      success = true;
    }
  } else {
    // An error occurred
    success = true;
  }

  if (udp_ || success) {
    enabled_events_ |= DE_READ;
  }
  if (!success) {
    LOG_F(LS_VERBOSE) << "Error = " << error_;
  }
  return received;
}

int PhysicalSocket::Recv(MemBuffer::Ptr buffer) {
  int res = 0;
  int size = 0;
  while (true) {
    Block::Ptr block = Block::TakeBlock();
    res = Recv(block->buffer, DEFAULT_BLOCK_SIZE);
    if (res == DEFAULT_BLOCK_SIZE) {
      block->buffer_size = res;
      buffer->AppendBlock(block);
      size += res;
      continue;
    } else if (res > 0) {
      block->buffer_size = res;
      buffer->AppendBlock(block);
      size += res;
      break;
    } else if (res == 0) {
      // Connection reset by peer
      size = 0;
      break;
    } else {
      if (IsBlockingError(error_)) {
        // No messages are available
        if (size == 0) {
          // Read none
          size = -1;
        }
      } else {
        LOG(L_ERROR) << "recv error occured";
        // An error occurred
        size = -1;
      }
      break;
    }
  }

  return size;
}

int PhysicalSocket::RecvFrom(void* buffer, size_t length, SocketAddress *out_addr) {
  sockaddr_storage addr_storage;
  socklen_t addr_len = sizeof(addr_storage);
  sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
  int received = ::recvfrom(s_, static_cast<char*>(buffer),
                            static_cast<int>(length), 0, addr, &addr_len);
  UpdateLastError();
  if ((received >= 0) && (out_addr != NULL))
    SocketAddressFromSockAddrStorage(addr_storage, out_addr);
  bool success = (received >= 0) || IsBlockingError(error_);
  if (udp_ || success) {
    enabled_events_ |= DE_READ;
  }
  if (!success) {
    LOG_F(LS_VERBOSE) << "Error = " << error_;
  }
  return received;
}

int PhysicalSocket::RecvFrom(MemBuffer::Ptr buffer, SocketAddress *out_addr) {
#ifdef LITEOS
#define RECV_BUFF_LEN  (4*1024)
#else
#define RECV_BUFF_LEN  (64*1024)
#endif
  char temp_buffer[RECV_BUFF_LEN] = {0};
  int res = RecvFrom(temp_buffer, RECV_BUFF_LEN, out_addr);
  if (res > 0) {
    buffer->WriteBytes(temp_buffer, res);
  }
  return res;
}

int PhysicalSocket::Listen(int backlog) {
  int err = ::listen(s_, backlog);
  UpdateLastError();
  if (err == 0) {
    state_ = CS_CONNECTING;
    enabled_events_ |= DE_ACCEPT;
#ifdef _DEBUG
    dbg_addr_ = "Listening @ ";
    dbg_addr_.append(GetLocalAddress().ToString());
#endif  // _DEBUG
  }
  return err;
}

Socket::Ptr PhysicalSocket::Accept(SocketAddress *out_addr) {
  sockaddr_storage addr_storage;
  socklen_t addr_len = sizeof(addr_storage);
  sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
  SOCKET s = ::accept(s_, addr, &addr_len);
  UpdateLastError();
  if (s == INVALID_SOCKET) {
    return Socket::Ptr();
  }
  enabled_events_ |= DE_ACCEPT;
  if (out_addr != NULL) {
    SocketAddressFromSockAddrStorage(addr_storage, out_addr);
  }

  PhysicalSocket::Ptr dispatcher(new PhysicalSocket(s));
  dispatcher->SetSocketNonblock();
  return dispatcher;
}

int PhysicalSocket::Close() {
  if (s_ == INVALID_SOCKET)
    return 0;
  int err = ::closesocket(s_);
  UpdateLastError();
  s_ = INVALID_SOCKET;
  state_ = CS_CLOSED;
  enabled_events_ = 0;
  //if (resolver_) {
  //  resolver_->Destroy(false);
  //  resolver_ = NULL;
  //}
  return err;
}

void PhysicalSocket::UpdateLastError() {
  error_ = LAST_SYSTEM_ERROR;
}

int PhysicalSocket::TranslateOption(Option opt, int* slevel, int* sopt) {
  switch (opt) {
  case OPT_DONTFRAGMENT:
#ifdef WIN32
    *slevel = IPPROTO_IP;
    *sopt = IP_DONTFRAGMENT;
    break;
#elif defined(IOS) || defined(OSX) || defined(BSD)
    LOG(LS_WARNING) << "Socket::OPT_DONTFRAGMENT not supported.";
    return -1;
#elif defined(POSIX)
    *slevel = IPPROTO_IP;
    *sopt = IP_MTU_DISCOVER;
    break;
#endif
  case OPT_RCVBUF:
    *slevel = SOL_SOCKET;
    *sopt = SO_RCVBUF;
    break;
  case OPT_SNDBUF:
    *slevel = SOL_SOCKET;
    *sopt = SO_SNDBUF;
    break;
  case OPT_NODELAY:
    *slevel = IPPROTO_TCP;
    *sopt = TCP_NODELAY;
    break;
  case OPT_MULTICAST_MEMBERSHIP:
    *slevel = IPPROTO_IP;
    *sopt = IP_ADD_MEMBERSHIP;
    break;
  default:
    ASSERT(false);
    return -1;
  }
  return 0;
}


uint32 PhysicalSocket::GetRequestedEvents() {
  LOG(L_INFO) << "enabled_events_ = " << (uint32)enabled_events_
              << "\t" << (uint32)s_;
  return enabled_events_;
}

SOCKET PhysicalSocket::GetSocket() {
  return s_;
}

bool PhysicalSocket::CheckSignalClose() {
  if (s_ == INVALID_SOCKET || state_ == CS_CLOSED) {
    return true;
  }
  return false;
}

void PhysicalSocket::OnEvent(uint32 ff) {

  if (((ff & DE_CONNECT) != 0)) {
    if (ff != DE_CONNECT)
      LOG(LS_VERBOSE) << "Signalled with DE_CONNECT: " << ff;
    enabled_events_ &= ~DE_CONNECT;
#ifdef _DEBUG
    dbg_addr_ = "Connected @ ";
    dbg_addr_.append(GetRemoteAddress().ToString());
#endif  // _DEBUG
    SignalConnectEvent(shared_from_this());
  }
  if (((ff & DE_ACCEPT) != 0)) {
    enabled_events_ &= ~DE_ACCEPT;
    SignalReadEvent(shared_from_this());
  }
  if ((ff & DE_READ) != 0) {
    enabled_events_ &= ~DE_READ;
    SignalReadEvent(shared_from_this());
  }
  if (((ff & DE_WRITE) != 0)) {
    enabled_events_ &= ~DE_WRITE;
    SignalWriteEvent(shared_from_this());
  }
  if (((ff & DE_CLOSE) != 0)) {
    Close();
  }
}

#ifdef WIN32
static uint32 FlagsToEvents(uint32 events) {
  uint32 ffFD = FD_CLOSE;
  if (events & DE_READ)
    ffFD |= FD_READ;
  if (events & DE_WRITE)
    ffFD |= FD_WRITE;
  if (events & DE_CONNECT)
    ffFD |= FD_CONNECT;
  if (events & DE_ACCEPT)
    ffFD |= FD_ACCEPT;
  return ffFD;
}
#endif  // WIN32
////////////////////////////////////////////////////////////////////////////////

EventDispatcher::EventDispatcher(Dispatcher::Ptr dispatcher, uint32 events)
  : disp_(dispatcher),
    event_close_(false),
    enabled_events_(events) {
}

EventDispatcher::~EventDispatcher() {
  event_close_ = true;
}

void EventDispatcher::Close() {
  event_close_ = true;
  SignalEvent.disconnect_all();
  if (disp_) {
    disp_.reset();
  }
}


void EventDispatcher::RemoveEvent(uint32 event_type) {
  enabled_events_ &= ~event_type;
}

void EventDispatcher::AddEvent(uint32 event_type) {
  enabled_events_ |= event_type;
}

bool EventDispatcher::CheckEventClose() {
  return event_close_ || (disp_ && disp_->CheckSignalClose());
}

void EventDispatcher::DisableEvent() {
  event_close_ = true;
}

void EventDispatcher::EnableEvent() {
  event_close_ = false;
}

SOCKET EventDispatcher::GetSocket() {
  if (disp_) {
    return disp_->GetSocket();
  }
  return INVALID_SOCKET;
}

void EventDispatcher::OnEvent(uint32 ff, int err) {
  // Make sure we deliver connect/accept first. Otherwise, consumers may see
  // something like a READ followed by a CONNECT, which would be odd.
  //LOG(L_INFO) << "socket "<< (uint32)disp_->GetSocket()
  //            << ", received event type = " << ff;
  SignalEvent(shared_from_this(),
              boost::dynamic_pointer_cast<Socket>(disp_),
              ff,
              err);
  if (disp_) {
    // when close or err event occured, uplayer will destory disp_.
    disp_->OnEvent(ff);
  }
  //if (CheckEventClose() || (ff & DE_CLOSE)) {
  //  return ;
  //}
  //uint32 new_events = enabled_events_ & ~ff;
  //if (new_events == DE_CLOSE) {
  //  return ;
  //}
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
NetworkService::NetworkService()
  : fWait_(false) {
#ifdef WIN32
  socket_ev_ = WSACreateEvent();
  signal_wakeup_ = WSACreateEvent();
  ASSERT(signal_wakeup_ != NULL);
  ASSERT(socket_ev_ != NULL);
#else
  signal_wakeup_ = Signal::CreateSignal();
#if 0 //defined(POSIX) && !defined(LITEOS)
  epoll_ = epoll_create1(0);
  ASSERT(epoll_ >= 0);

  struct epoll_event listen_event = {0};
  SOCKET s = signal_wakeup_->GetSocket();
  listen_event.events = EPOLLIN;
  listen_event.data.fd = s;
  int ret = epoll_ctl(epoll_, EPOLL_CTL_ADD, s, &listen_event);
  ASSERT(ret == 0);
#endif
#endif
}

NetworkService::~NetworkService() {
#ifdef WIN32
  WSACloseEvent(socket_ev_);
  WSACloseEvent(signal_wakeup_);
#else
  delete signal_wakeup_;
#if 0 //defined(POSIX) && !defined(LITEOS)
  close(epoll_);
#endif
#endif
  ASSERT(dispatchers_.empty());
}

bool NetworkService::InitNetworkService() {
  return true;
}

Socket::Ptr NetworkService::CreateSocket(int type) {
  return CreateSocket(AF_INET, type);
}

Socket::Ptr NetworkService::CreateSocket(int family, int type) {
  PhysicalSocket::Ptr dispatcher(new PhysicalSocket());
  if (dispatcher->Create(family, type)) {
    //SOCKET sock = dispatcher->GetSocket();
    //LOG(L_INFO) << "create socket \t" << (uint32)sock;
    return dispatcher;
  }
  return Socket::Ptr();
}

Socket::Ptr NetworkService::WrapSocket(SOCKET s) {
  PhysicalSocket::Ptr dispatcher(new PhysicalSocket(s));
  if (dispatcher->SetSocketNonblock()) {
    return dispatcher;
  }
  return Socket::Ptr();
}

void NetworkService::Add(EventDispatcher::Ptr pdispatcher) {
  CritScope cs(&crit_);
  SOCKET sock = pdispatcher->GetSocket();

#if 0 //defined(POSIX) && !defined(LITEOS)
  struct epoll_event listen_event = {0};
  uint32 ff = pdispatcher->get_enable_events();
  if (ff & (DE_READ | DE_ACCEPT)) {
    listen_event.events |= EPOLLIN;
  }
  if (ff & (DE_WRITE | DE_CONNECT)) {
    listen_event.events |= EPOLLOUT;
  }

  listen_event.data.fd = sock;
  int ret = epoll_ctl(epoll_, EPOLL_CTL_MOD, sock, &listen_event);
  if (ret != 0) {
    if (errno == ENOENT) {
      ret = epoll_ctl(epoll_, EPOLL_CTL_ADD, sock, &listen_event);
      if (ret != 0) {
        LOG_E(LS_ERROR, EN, errno) << "epoll add";
      }
    } else {
      LOG_E(LS_ERROR, EN, errno) << "epoll mod";
    }
  }
#endif

  // Prevent duplicates. This can cause dead dispatchers to stick around.
  DispatcherList::iterator pos = std::find(dispatchers_.begin(),
                                 dispatchers_.end(),
                                 pdispatcher);
  if (pos != dispatchers_.end()) {
    //LOG(L_ERROR) << "socket already exist can't add \t" << (uint32)sock;
    return;
  }
  //LOG(L_INFO) << "Add socket \t" << (uint32)sock;
  dispatchers_.push_back(pdispatcher);
}

void NetworkService::Remove(EventDispatcher::Ptr pdispatcher) {
  CritScope cs(&crit_);
  SOCKET sock = pdispatcher->GetSocket();
  DispatcherList::iterator pos = std::find(dispatchers_.begin(),
                                 dispatchers_.end(),
                                 pdispatcher);
  if (pos != dispatchers_.end()) {
    pdispatcher->DisableEvent();
#if 0 //defined(POSIX) && !defined(LITEOS)
    struct epoll_event listen_event = {0};
    int ret = epoll_ctl(epoll_, EPOLL_CTL_DEL, sock, &listen_event);
    if (ret != 0) {
      LOG(L_ERROR) << "Can't remove socket form epoll, err = " << errno;
    }
#endif
  }
}

#ifdef WIN32
bool NetworkService::Wait(int cmsWait, bool process_io) {
  int cmsTotal = cmsWait;
  int cmsElapsed = 0;
  uint32 msStart = Time();

  fWait_ = true;
  while (fWait_) {
    std::vector<WSAEVENT> events;

    // 首先确保把Socket的事件添加到第一位
    events.push_back(socket_ev_);
    // 将Wake UP放到这里面
    events.push_back(signal_wakeup_);
    // 然后将所有Socket相关的事件添加到这里面
    {
      CritScope cr(&crit_);
      // 清除已经无效的Dispatcher对象
      // ClearRemovedDispacher();
      for (DispatcherList::iterator iter = dispatchers_.begin();
           iter != dispatchers_.end();) {
        EventDispatcher::Ptr disp = *iter;
        SOCKET s = disp->GetSocket();
        if (s == INVALID_SOCKET || disp->CheckEventClose()) {
          dispatchers_.erase(iter++);
          continue;
        }
        WSAEventSelect(s, events[0], FlagsToEvents(disp->get_enable_events()));
        iter++;
      }
    }

    // Which is shorter, the delay wait or the asked wait?
    // 计算超时时间
    int cmsNext;
    if (cmsWait == kForever) {
      cmsNext = cmsWait;
    } else {
      cmsNext = _max(0, cmsTotal - cmsElapsed);
    }

    // Wait for one of the events to signal
    // events.size() 等于 1
    DWORD dw = WSAWaitForMultipleEvents(static_cast<DWORD>(events.size()),
                                        &events[0],
                                        false,
                                        cmsNext,
                                        false);
    if (dw == WSA_WAIT_FAILED) {
      // Failed?
      // TODO: need a better strategy than this!
      int error = WSAGetLastError();
      ASSERT(false);
      return false;
    } else if (dw == WSA_WAIT_TIMEOUT) {
      // Timeout?
      return true;
    } else {
      // Figure out which one it is and call it
      CritScope cr(&crit_);
      int index = dw - WSA_WAIT_EVENT_0;
      // ASSERT(index == 0);
      if (index > 0) {
        // 停止消息循环，自然退出
        ResetWakeEvent();
        fWait_ = false;
      } else {
        for (DispatcherList::iterator iter = dispatchers_.begin();
             iter != dispatchers_.end();) {
          EventDispatcher::Ptr disp = *iter;
          SOCKET s = disp->GetSocket();
          if (s == INVALID_SOCKET || disp->CheckEventClose()) {
            iter++;
            continue;
          }

          WSANETWORKEVENTS wsaEvents;
          int err = WSAEnumNetworkEvents(s, events[0], &wsaEvents);
          if (err == 0) {
            uint32 ff = 0;
            int errcode = 0;
            if (wsaEvents.lNetworkEvents & FD_READ)
              ff |= DE_READ;
            if (wsaEvents.lNetworkEvents & FD_WRITE)
              ff |= DE_WRITE;
            if (wsaEvents.lNetworkEvents & FD_CONNECT) {
              if (wsaEvents.iErrorCode[FD_CONNECT_BIT] == 0) {
                ff |= DE_CONNECT;
              } else {
                ff |= DE_CLOSE;
                errcode = wsaEvents.iErrorCode[FD_CONNECT_BIT];
              }
            }

            if (wsaEvents.lNetworkEvents & FD_ACCEPT)
              ff |= DE_ACCEPT;
            if (wsaEvents.lNetworkEvents & FD_CLOSE) {
              ff |= DE_CLOSE;
              errcode = wsaEvents.iErrorCode[FD_CLOSE_BIT];
            }

            if (disp->CheckEventClose()) {
              iter++;
              continue;
            } else if (ff != 0) {
              dispatchers_.erase(iter++);
              disp->OnEvent(ff, errcode);
            } else {
              iter++;
            }
          }
        }
      }
      // Reset the network event until new activity occurs
      WSAResetEvent(socket_ev_);
    }

    // Break?
    if (!fWait_)
      break;
    cmsElapsed = TimeSince(msStart);
    if ((cmsWait != kForever) && (cmsElapsed >= cmsWait)) {
      break;
    }
  }
  // Done
  return true;
}

void NetworkService::WakeUp() {
  WSASetEvent(signal_wakeup_);
}

void NetworkService::ResetWakeEvent() {
  if (signal_wakeup_ != NULL) {
    WSAResetEvent(signal_wakeup_);
  }
}

#else
#ifdef POSIX //LITEOS
bool NetworkService::Wait(int cmsWait, bool process_io) {
  // Calculate timing information
  struct timeval *ptvWait = NULL;
  struct timeval tvWait;
  struct timeval tvStop;
  if (cmsWait != kForever) {
    // Calculate wait timeval
    tvWait.tv_sec = cmsWait / 1000;
    tvWait.tv_usec = (cmsWait % 1000) * 1000;
    ptvWait = &tvWait;

    // Calculate when to return in a timeval
    gettimeofday(&tvStop, NULL);
    tvStop.tv_sec += tvWait.tv_sec;
    tvStop.tv_usec += tvWait.tv_usec;
    if (tvStop.tv_usec >= 1000000) {
      tvStop.tv_usec -= 1000000;
      tvStop.tv_sec += 1;
    }
  }

  // Zero all fd_sets. Don't need to do this inside the loop since
  // select() zeros the descriptors not signaled

  fd_set fdsRead;
  fd_set fdsWrite;

  fWait_ = true;
  while (fWait_) {
    int fdmax = -1;
    FD_ZERO(&fdsRead);
    FD_ZERO(&fdsWrite);
    // 将signal wake up 事件加入进去
    {
      SOCKET s = signal_wakeup_->GetSocket();
      uint32 ff = signal_wakeup_->GetEvents();
      if (s > fdmax) {
        fdmax = s;
      }
      if (ff & (DE_READ | DE_ACCEPT)) {
        FD_SET(s, &fdsRead);
      }
      if (ff & (DE_WRITE | DE_CONNECT)) {
        FD_SET(s, &fdsWrite);
      }
    }

    // 将网络相关的Socket添加进去
    {
      CritScope cr(&crit_);

      for (DispatcherList::iterator iter = dispatchers_.begin();
           iter != dispatchers_.end();) {
        EventDispatcher::Ptr disp = *iter;
        SOCKET s = disp->GetSocket();
        if (s == INVALID_SOCKET || disp->CheckEventClose()) {
          dispatchers_.erase(iter++);
          continue;
        }
        if (s > fdmax) {
          fdmax = s;
        }
        uint32 ff = disp->get_enable_events();
        if (ff & (DE_READ | DE_ACCEPT)) {
          FD_SET(s, &fdsRead);
        }
        if (ff & (DE_WRITE | DE_CONNECT)) {
          FD_SET(s, &fdsWrite);
        }
        iter++;
      }
    }
    // Wait then call handlers as appropriate
    // < 0 means error
    // 0 means timeout
    // > 0 means count of descriptors ready
    int n = select(fdmax + 1, &fdsRead, &fdsWrite, NULL, ptvWait);

    // If error, return error.
    if (n < 0) {
      if (errno != EINTR) {
        LOG_E(LS_ERROR, EN, errno) << "select";
        LOG(L_INFO) << "Socket number: " << dispatchers_.size() << "max fd: " << fdmax;
        return false;
      }
      // Else ignore the error and keep going. If this EINTR was for one of the
      // signals managed by this PhysicalSocketServer, the
      // PosixSignalDeliveryDispatcher will be in the signaled state in the next
      // iteration.
    } else if (n == 0) {
      // If timeout, return success
      return true;
    } else {
      // We have signaled descriptors
      int count = 0;
      CritScope cr(&crit_);
      // 检查是否有Wakeup事件
      SOCKET wake_socket = signal_wakeup_->GetSocket();
      if (FD_ISSET(wake_socket, &fdsRead)) {
        FD_CLR(wake_socket, &fdsRead);
        ResetWakeEvent();
        fWait_ = false;
        count += 1;
      } else if (FD_ISSET(wake_socket, &fdsWrite)) {
        FD_CLR(wake_socket, &fdsWrite);
        ResetWakeEvent();
        fWait_ = false;
        count += 1;
      }

      for (DispatcherList::iterator iter = dispatchers_.begin();
           (count < n) && (iter != dispatchers_.end());) {
        // 检查网络是否有事件
        EventDispatcher::Ptr disp = *iter;
        SOCKET fd                 = disp->GetSocket();
        uint32 ff                 = 0;
        int errcode               = 0;

        // Skip invalid item.
        if (fd == INVALID_SOCKET || disp->CheckEventClose()) {
          iter++;
          continue;
        }
        // Reap any error code, which can be signaled through reads or writes.
        // TODO: Should we set errcode if getsockopt fails?
        if (FD_ISSET(fd, &fdsRead) || FD_ISSET(fd, &fdsWrite)) {
          socklen_t len = sizeof(errcode);
          ::getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcode, &len);
        }

        // Check readable descriptors. If we're waiting on an accept, signal
        // that. Otherwise we're waiting for data, check to see if we're
        // readable or really closed.
        // TODO: Only peek at TCP descriptors.
        if (FD_ISSET(fd, &fdsRead)) {
          FD_CLR(fd, &fdsRead);
          if (disp->get_enable_events() & DE_ACCEPT) {
            ff |= DE_ACCEPT;
          } else if (errcode || disp->CheckEventClose()) {
            ff |= DE_CLOSE;
          } else {
            ff |= DE_READ;
          }
        }

        // Check writable descriptors. If we're waiting on a connect, detect
        // success versus failure by the reaped error code.
        if (FD_ISSET(fd, &fdsWrite)) {
          FD_CLR(fd, &fdsWrite);
          if (disp->get_enable_events() & DE_CONNECT) {
            if (!errcode) {
              ff |= DE_CONNECT;
            } else {
              ff |= DE_CLOSE;
            }
          } else {
            ff |= DE_WRITE;
          }
        }

        // Tell the descriptor about the event.
        if (ff != 0) {
          dispatchers_.erase(iter++);
          disp->OnEvent(ff, errcode);
          count += 1;
        } else {
          iter++;
        }
      }
    }

    // Recalc the time remaining to wait. Doing it here means it doesn't get
    // calced twice the first time through the loop
    if (ptvWait) {
      ptvWait->tv_sec = 0;
      ptvWait->tv_usec = 0;
      struct timeval tvT;
      gettimeofday(&tvT, NULL);
      if ((tvStop.tv_sec > tvT.tv_sec)
          || ((tvStop.tv_sec == tvT.tv_sec)
              && (tvStop.tv_usec > tvT.tv_usec))) {
        ptvWait->tv_sec = tvStop.tv_sec - tvT.tv_sec;
        ptvWait->tv_usec = tvStop.tv_usec - tvT.tv_usec;
        if (ptvWait->tv_usec < 0) {
          ASSERT(ptvWait->tv_sec > 0);
          ptvWait->tv_usec += 1000000;
          ptvWait->tv_sec -= 1;
        }
      }
    }
  }
  return true;
}
#elif 0 //POSIX
// 目前Dispatcher管理方案导致Epoll方案性能低下，暂时关闭
bool NetworkService::Wait(int cmsWait, bool process_io) {
  // Calculate timing information
  struct timeval tvStop;
  int wait_time = cmsWait;
  if (wait_time != kForever) {
    // Calculate when to return in a timeval
    gettimeofday(&tvStop, NULL);
    tvStop.tv_sec += wait_time / 1000;
    tvStop.tv_usec += (wait_time % 1000) * 1000;
    if (tvStop.tv_usec >= 1000000) {
      tvStop.tv_usec -= 1000000;
      tvStop.tv_sec += 1;
    }
  }

  for (DispatcherList::iterator iter = dispatchers_.begin();
       iter != dispatchers_.end();) {
    EventDispatcher::Ptr disp = *iter;
    SOCKET s = disp->GetSocket();
    if (s == INVALID_SOCKET || disp->CheckEventClose()) {
      dispatchers_.erase(iter++);
      continue;
    }
    iter++;
  }

  fWait_ = true;
  while (fWait_) {
    int size = 64; //dispatchers_.size() * 4;
    struct epoll_event *event_buff = new struct epoll_event [64];
    boost::shared_ptr<struct epoll_event> epoll_events(event_buff);

    // Wait then call handlers as appropriate
    // < 0 means error
    // 0 means timeout
    // > 0 means count of descriptors ready
    int event_num = epoll_wait(epoll_, event_buff, 64, wait_time);
    // If error, return error.
    if (event_num < 0) {
      if (errno != EINTR) {
        LOG_E(LS_ERROR, EN, errno) << "epoll_wait";
        return false;
      }
      // Else ignore the error and keep going. If this EINTR was for one of the
      // signals managed by this PhysicalSocketServer, the
      // PosixSignalDeliveryDispatcher will be in the signaled state in the next
      // iteration.
    } else if (event_num == 0) {
      // If timeout, return success
      return true;
    } else {
      for (int i=0; i<event_num; i++) {
        // 检查是否有Wakeup事件
        int fd = event_buff[i].data.fd;
        SOCKET wake_socket = signal_wakeup_->GetSocket();
        if (wake_socket == fd) {
          ResetWakeEvent();
          fWait_ = false;
        } else {
          EventDispatcher::Ptr disp;
          SOCKET s;
          DispatcherList::iterator iter;
          for (iter = dispatchers_.begin(); iter != dispatchers_.end(); iter++) {
            disp = *iter;
            s = disp->GetSocket();
            if (s == fd) {
              break;
            }
          }

          uint32 enable_event = disp->get_enable_events();
          uint32 ff   = 0;
          int errcode = 0;
          // Reap any error code, which can be signaled through reads or writes.
          if ((event_buff[i].events & EPOLLIN) || (event_buff[i].events & EPOLLERR)) {
            socklen_t len = sizeof(errcode);
            ::getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcode, &len);
          }

          // Check readable descriptors. If we're waiting on an accept, signal
          // that. Otherwise we're waiting for data, check to see if we're
          // readable or really closed.
          // TODO: Only peek at TCP descriptors.
          if ((event_buff[i].events & EPOLLIN)
              && (enable_event & (DE_READ | DE_ACCEPT))) {
            if (enable_event & DE_ACCEPT) {
              ff |= DE_ACCEPT;
            } else if (errcode || disp->CheckEventClose()) {
              ff |= DE_CLOSE;
            } else {
              ff |= DE_READ;
            }
          }

          // Check writable descriptors. If we're waiting on a connect, detect
          // success versus failure by the reaped error code.
          if ((event_buff[i].events & EPOLLOUT)
              && (enable_event & (DE_WRITE | DE_CONNECT))) {
            if (enable_event & DE_CONNECT) {
              if (!errcode) {
                ff |= DE_CONNECT;
              } else {
                ff |= DE_CLOSE;
              }
            } else {
              ff |= DE_WRITE;
            }
          }

          dispatchers_.erase(iter);
          // Tell the descriptor about the event.
          disp->OnEvent(ff, errcode);
        }
      }
    }

    // Recalc the time remaining to wait. Doing it here means it doesn't get
    // calced twice the first time through the loop
    if (wait_time != kForever) {
      wait_time = 0;
      struct timeval tvT;
      gettimeofday(&tvT, NULL);
      if ((tvStop.tv_sec > tvT.tv_sec)
          || ((tvStop.tv_sec == tvT.tv_sec)
              && (tvStop.tv_usec > tvT.tv_usec))) {
        wait_time = (tvStop.tv_sec - tvT.tv_sec)*1000
                    + (tvStop.tv_usec - tvT.tv_usec)/1000;
      } else {
      }
    }
  }
  return true;
}
#endif

void NetworkService::WakeUp() {
  signal_wakeup_->SignalWakeup();
}

void NetworkService::ResetWakeEvent() {
  signal_wakeup_->ResetWakeEvent();
}
#endif


EventDispatcher::Ptr NetworkService::CreateDispEvent(Socket::Ptr socket,
    uint32 enabled_events) {
  EventDispatcher::Ptr event_disp(
    new EventDispatcher(
      boost::dynamic_pointer_cast<PhysicalSocket>(socket), enabled_events));
  return event_disp;
}



}  // namespace vzes
