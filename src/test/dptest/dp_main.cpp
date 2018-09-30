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
#include "app/app/app.h"
#include "eventservice/dp/dpclient.h"
#include <stdio.h>

#define DP_MSG_HELLO  "HELLO"
#define DP_MSG_WORLD  "WORLD"

#define DP_MESSAGE_DATA "DP_MESSAGE_DATA"

class EchoClient : public app::AppInterface,
  public boost::noncopyable,
  public boost::enable_shared_from_this<EchoClient>,
  public sigslot::has_slots<> {
 public:
  EchoClient() :AppInterface("EchoClient") {
  }
  virtual ~EchoClient() {
  }

  //////////////////////////////////////////////////////////////////////////////
  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    event_service_ = event_service;
    dp_client_     = vzes::DpClient::CreateDpClient(event_service_);
    ASSERT_RETURN_FAILURE(!dp_client_, false);
    dp_client_->SignalDpMessage.connect(
      this, &EchoClient::OnDpMessage);
    dp_client_->ListenMessage(DP_MSG_HELLO);
    dp_client_->ListenMessage(DP_MSG_WORLD);
    return true;
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    return true;
  }

  virtual bool RunAPP(vzes::EventService::Ptr event_service) {
    LOG(L_INFO) << "Send dp message";
    dp_client_->SendDpMessage(DP_MSG_HELLO,
                              0,
                              DP_MESSAGE_DATA,
                              strlen(DP_MESSAGE_DATA));
    LOG(L_INFO) << "Send dp request start";
    dp_client_->SendDpRequest(DP_MSG_HELLO,
                              0,
                              DP_MESSAGE_DATA,
                              strlen(DP_MESSAGE_DATA),
                              NULL,
                              10000);
    LOG(L_INFO) << "Send dp request done";
    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }
 private:
  void OnDpMessage(vzes::DpClient::Ptr dp_client, vzes::DpMessage::Ptr dp_msg) {
    LOG(L_INFO) << "ECHO CLIENT OnDpMessage";
  }
 private:
  vzes::EventService::Ptr event_service_;
  vzes::DpClient::Ptr     dp_client_;
};

class EchoServer : public app::AppInterface,
  public boost::noncopyable,
  public boost::enable_shared_from_this<EchoServer>,
  public sigslot::has_slots<> {
 public:
  EchoServer() :AppInterface("EchoServer") {
  }
  virtual ~EchoServer() {
  }

  //////////////////////////////////////////////////////////////////////////////
  virtual bool PreInit(vzes::EventService::Ptr event_service) {
    event_service_ = event_service;
    dp_client_     = vzes::DpClient::CreateDpClient(event_service_);
    ASSERT_RETURN_FAILURE(!dp_client_, false);
    dp_client_->SignalDpMessage.connect(
      this, &EchoServer::OnDpMessage);
    dp_client_->ListenMessage(DP_MSG_HELLO);
    dp_client_->ListenMessage(DP_MSG_WORLD);
    return true;
  }

  virtual bool InitApp(vzes::EventService::Ptr event_service) {
    return true;
  }

  virtual bool RunAPP(vzes::EventService::Ptr event_service) {
    //dp_client_->SendDpMessage(DP_MSG_HELLO,
    //  0,
    //  DP_MESSAGE_DATA,
    //  strlen(DP_MESSAGE_DATA));
    return true;
  }

  virtual void OnExitApp(vzes::EventService::Ptr event_service) {
  }
 private:
  void OnDpMessage(vzes::DpClient::Ptr dp_client, vzes::DpMessage::Ptr dp_msg) {
    if (dp_msg->type == vzes::TYPE_MESSAGE) {
      LOG(L_INFO) << "ECHO SERVER Receive dp message";
      dp_client_->SendDpMessage(DP_MSG_HELLO,
                                0,
                                DP_MESSAGE_DATA,
                                strlen(DP_MESSAGE_DATA));
    } else if (dp_msg->type == vzes::TYPE_REQUEST) {
      LOG(L_INFO) << "ECHO SERVER Recive dp request";
      dp_client_->SendDpReply(dp_msg);
    }
  }
 private:
  vzes::EventService::Ptr event_service_;
  vzes::DpClient::Ptr     dp_client_;
};

int main(void) {
  printf("LOGGING = %d\n", LOGGING);
  app::App::Ptr app = app::App::CreateApp();
  if (vzes::LogMessage::Loggable(vzes::INFO)) {
    printf("Message done vzes::LogMessage::Loggable(vzes::INFO) %d\n",
           vzes::LogMessage::GetMinLogSeverity());
  }

  LOG(L_INFO) << "Message done";
  app::AppInterface::Ptr echo_client(new EchoClient());
  app::AppInterface::Ptr echo_server(new EchoServer());
  LOG(L_INFO) << "Message done";
  app->RegisterApp(echo_client);
  app->RegisterApp(echo_server);

  app->AppRun();
  app->ExitApp();
  return EXIT_SUCCESS;
}