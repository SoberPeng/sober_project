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

#include "eventservice/base/basicincludes.h"
#include "eventservice/net/eventservice.h"
#include "eventservice/event/signalevent.h"

#ifndef EVENTSERVICE_DP_DPCLIENT_H_
#define EVENTSERVICE_DP_DPCLIENT_H_

namespace vzes {

typedef std::string DpBuffer;

enum {
  TYPE_INVALID = -1,            // 非法TYPE
  TYPE_MESSAGE = 0,
  TYPE_REQUEST = 1,
  TYPE_REPLY = 2,
  TYPE_ERROR_TIMEOUT = 3,
  TYPE_ERROR_FORMAT = 4,
  TYPE_GET_SESSION_ID = 5,
  TYPE_SUCCEED = 6,
  TYPE_FAILURE = 7,
  TYPE_ADD_MESSAGE = 8,
  TYPE_REMOVE_MESSAGE = 9,
};

struct DpMessage : public MessageData {
  typedef boost::shared_ptr<DpMessage> Ptr;
  uint32            type;
  std::string       method;
  uint32            session_id;
  DpBuffer          req_buffer;
  DpBuffer          *res_buffer;
  SignalEvent::Ptr  signal_event;
};


class DpClient {
 public:
  typedef boost::shared_ptr<DpClient> Ptr;
  // 回调函数，当有请求或者收到广播的时候会从这个信号里面回调
  sigslot::signal2<DpClient::Ptr, DpMessage::Ptr> SignalDpMessage;

  // 创建一个DpClient对象，只能够使用这个函数创建DpClient
  // 如果返回空的智能指针，证明出了问题，否则返回一个正常的DPClient对象
  // 创建的时候要指定EventService，标明消息是在哪个线程中执行的
  static DpClient::Ptr CreateDpClient(EventService::Ptr es);

  // 删除一个DpClient对象，程序退出的时候，应该删除这个对象
  static void DestoryDpClient(DpClient::Ptr dp_client);

  // 发送一个广播消息，消息数据和内容由自己指定，详细请看星形结构设计文档
  virtual bool SendDpMessage(const std::string method,
                             uint32 session_id,
                             const char *data,
                             uint32 data_size) = 0;

  // 阻塞性的发送一个星形结构请求，请求的结果会放到res_buffer里面
  // 有一个超时时间，要么超时，要么得到结果
  virtual bool SendDpRequest(const std::string method,
                             uint32 session_id,
                             const char *data,
                             uint32 data_size,
                             DpBuffer *res_buffer,
                             uint32 timeout_millisecond) = 0;

  // 回复一个星形结构消息，需要将DPMessage的智能指针对象传给这个对象，以保证回复成功
  // 回复的内容要提前放到res_buffer里面，否则回复没有内容
  virtual bool SendDpReply(DpMessage::Ptr dp_msg) = 0;

  // 注册一个监听消息，可以循环调用
  virtual void ListenMessage(const std::string method) = 0;
  // 取消一个消息监听
  virtual void RemoveMessage(const std::string method) = 0;
};

}

#endif  // EVENTSERVICE_DP_DPCLIENT_H_

