/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#ifndef EVENTSERVICES_EVENTSERVICE_H_
#define EVENTSERVICES_EVENTSERVICE_H_

#include "eventservice/base/basicincludes.h"
#include "eventservice/event/thread.h"
#include "eventservice/net/networkservice.h"
#include "eventservice/net/networktinterface.h"

namespace vzes {

class EventService: public boost::noncopyable,
  public boost::enable_shared_from_this<EventService> {
 public:
  typedef boost::shared_ptr<EventService> Ptr;
 private:
  explicit EventService(Thread::Ptr thread);
 public:
  virtual ~EventService();

  static EventService::Ptr CreateEventService(Thread *thread = NULL,
      const std::string &thread_name = "thread");
  static EventService::Ptr CreateCurrentEventService(
    const std::string &thread_name = "thread");

  bool IsThisThread(Thread *thread);
  bool SetThreadPriority(ThreadPriority priority);

  // Common function
  bool InitEventService(const std::string &thread_name = "thread");
  void Run();
  void UninitEventService();

  // 发送异步消息
  void Post(MessageHandler *phandler,
            uint32 id = 0,
            MessageData::Ptr pdata = MessageData::Ptr(),
            bool time_sensitive = false);

  // 发送定时器消息
  void PostDelayed(int cmsDelay,
                   MessageHandler *phandler,
                   uint32 id = 0,
                   MessageData::Ptr pdata = MessageData::Ptr());

  // 发送同步消息
  void Send(MessageHandler *phandler,
            uint32 id = 0,
            MessageData::Ptr pdata = MessageData::Ptr());

  // 删除phandler或id指定的消息，如果phandler == NULL && id == MQID_ANY，
  // 则删除该EventService对应线程所有未处理的消息, 包括：post、post delayed、
  // send消息。MessageList* removed：[OUT]所有被删除的消息
  void Clear(MessageHandler *phandler,
             uint32 id = MQID_ANY,
             MessageList* removed = NULL);

  // 网络相关的服务
  virtual Socket::Ptr CreateSocket(int type);
  virtual Socket::Ptr CreateSocket(int family, int type);

  virtual EventDispatcher::Ptr CreateDispEvent(Socket::Ptr socket,
      uint32 enabled_events);

  AsyncListener::Ptr    CreateAsyncListener();
  AsyncConnecter::Ptr   CreateAsyncConnect();
  AsyncSocket::Ptr      CreateAsyncSocket(Socket::Ptr socket);
  AsyncUdpSocket::Ptr   CreateUdpServer(const SocketAddress &bind_addr);
  AsyncUdpSocket::Ptr   CreateUdpClient();
  AsyncUdpSocket::Ptr   CreateMulticastSocket(
    const SocketAddress &multicast_addr);

  // Socket::Ptr WrapSocket(SOCKET s);
  virtual bool Add(EventDispatcher::Ptr socket);
  virtual bool Remove(EventDispatcher::Ptr socket);
  //
  void WakeUp();
 private:
  Thread::Ptr thread_;
};

}  // namespace vzes

#endif  // EVENTSERVICES_EVENTSERVICE_H_
