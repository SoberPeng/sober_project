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

#ifndef EVENTSERVICES_NET_NETWORKSERVICE_H__
#define EVENTSERVICES_NET_NETWORKSERVICE_H__

#include <vector>
#include <map>

#include "eventservice/base/asyncfile.h"
#include "eventservice/base/scoped_ptr.h"
#include "eventservice/base/socketserver.h"
#include "eventservice/base/criticalsection.h"
#include "eventservice/net/networktinterface.h"
#include "eventservice/event/signalevent.h"


namespace vzes {


class Dispatcher {
 public:
  typedef boost::shared_ptr<Dispatcher> Ptr;
 public:
  virtual ~Dispatcher() {}
  virtual uint32 GetRequestedEvents() = 0;
  virtual SOCKET GetSocket() = 0;
  virtual bool CheckSignalClose() = 0;
  virtual void OnEvent(uint32 ff) = 0;
};
////////////////////////////////////////////////////////////////////////////////


class PhysicalSocket : public Socket,
  public sigslot::has_slots<>,
  public Dispatcher {
 public:
  typedef boost::shared_ptr<PhysicalSocket> Ptr;
  PhysicalSocket(SOCKET s = INVALID_SOCKET);

  virtual ~PhysicalSocket();

  // With Socket Interface
  virtual Socket::Ptr Accept(SocketAddress *out_addr);

  virtual SocketAddress GetLocalAddress() const;
  virtual SocketAddress GetRemoteAddress() const;

  virtual int Bind(const SocketAddress& bind_addr);
  virtual int Connect(const SocketAddress& addr);
  virtual int Send(const void *pv, size_t cb);
  virtual int Send(MemBuffer::Ptr buffer);
  virtual int SendTo(const void* buffer,
                     size_t length,
                     const SocketAddress& addr);
  virtual int Recv(void* buffer, size_t length);
  virtual int Recv(MemBuffer::Ptr buffer);
  virtual int RecvFrom(void* buffer, size_t length, SocketAddress *out_addr);
  virtual int RecvFrom(MemBuffer::Ptr buffer, SocketAddress *out_addr);
  virtual int Listen(int backlog);
  virtual int Close();
  virtual int GetError() const {
    return error_;
  }
  virtual void SetError(int error) {
    error_ = error;
  }
  virtual ConnState GetState() const {
    return state_;
  }
  virtual int GetOption(Option opt, int* value);
  virtual int SetOption(Option opt, int value);
  virtual int SetOption(int level,
                        int optname,
                        const char *value,
                        int optlen);

  // For Dispatcher
  virtual uint32 GetRequestedEvents();
  virtual SOCKET GetSocket();
  virtual bool CheckSignalClose();
  virtual void OnEvent(uint32 ff);

  // Help Function
  int DoConnect(const SocketAddress& connect_addr);
  // Creates the underlying OS socket (same as the "socket" function).
  virtual bool Create(int family, int type);
  //
  bool SetSocketNonblock();

 protected:
  // void OnResolveResult(SignalThread* thread);

  void UpdateLastError();
  static int TranslateOption(Option opt, int* slevel, int* sopt);

  SOCKET s_;
  uint8 enabled_events_;
  bool udp_;
  int error_;
  ConnState state_;
  // AsyncResolver* resolver_;

#ifdef _DEBUG
  std::string dbg_addr_;
#endif  // _DEBUG;
};

class NetworkService;

class EventDispatcher : public boost::noncopyable,
  public boost::enable_shared_from_this<EventDispatcher> {
 public:
  typedef boost::shared_ptr<EventDispatcher> Ptr;
  sigslot::signal4<EventDispatcher::Ptr,
          Socket::Ptr,
          uint32,
          int> SignalEvent;
 protected:
  friend class NetworkService;
  EventDispatcher(Dispatcher::Ptr dispatcher, uint32 events);
 public:
  virtual ~EventDispatcher();
  void Close();
  virtual void RemoveEvent(uint32 event_type);
  virtual void AddEvent(uint32 event_type);
  uint32 get_enable_events() const {
    return enabled_events_;
  }
 protected:
  SOCKET GetSocket();
  bool CheckEventClose();
  void DisableEvent();
  void EnableEvent();
  //

  void OnEvent(uint32 ff, int err);
 private:
  Dispatcher::Ptr disp_;
  uint32  enabled_events_;
  bool  event_close_;
};

////////////////////////////////////////////////////////////////////////////////
// A socket server that provides the real sockets of the underlying OS.
class NetworkService : public SocketServer {
 public:
  typedef boost::shared_ptr<NetworkService> Ptr;
  NetworkService();
  virtual ~NetworkService();
  bool InitNetworkService();

  virtual Socket::Ptr CreateSocket(int type);
  virtual Socket::Ptr CreateSocket(int family, int type);

  virtual EventDispatcher::Ptr CreateDispEvent(Socket::Ptr socket,
      uint32 enabled_events);

  // Internal Factory for Accept
  Socket::Ptr WrapSocket(SOCKET s);

  // SocketServer:
  virtual bool Wait(int cms, bool process_io);
  virtual void WakeUp();

  void Add(EventDispatcher::Ptr dispatcher);
  void Remove(EventDispatcher::Ptr dispatcher);
 private:
  void ResetWakeEvent();
 private:
  typedef std::list<EventDispatcher::Ptr> DispatcherList;

  DispatcherList dispatchers_;
  CriticalSection crit_;
  bool fWait_;
#ifdef WIN32
  WSAEVENT socket_ev_;
  WSAEVENT signal_wakeup_;
#else
  Signal *signal_wakeup_;
#if defined(POSIX) && !defined(LITEOS)
  int epoll_;
#endif
#endif
};

} // namespace vzes

#endif // EVENTSERVICES_NET_NETWORKSERVICE_H__
