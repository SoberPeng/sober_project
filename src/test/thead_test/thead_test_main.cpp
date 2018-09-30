/*
 * vzevent
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
#include "eventservice/base/logging.h"
#include "eventservice/net/eventservice.h"

struct TestMessage : public vzes::MessageData {
  typedef boost::shared_ptr<TestMessage> Ptr;
  uint32 index;
};

class TimeEvent : public vzes::MessageHandler {
 public:
  TimeEvent(vzes::EventService::Ptr main_es)
    : main_es_(main_es) {
  }
  virtual ~TimeEvent() {
  }
  void Start() {
    second_es_ = vzes::EventService::CreateEventService(NULL, "second_es");
    TestMessage::Ptr tmsg(new TestMessage);
    tmsg->index = 0;
    main_es_->PostDelayed(1000, this, 0, tmsg);
  }
  virtual void OnMessage(vzes::Message *msg) {
    TestMessage::Ptr tmsg = boost::dynamic_pointer_cast<TestMessage>(msg->pdata);
    tmsg->index ++;
    vzes::Thread *current_thread = vzes::Thread::Current();
    if (main_es_->IsThisThread(current_thread)) {
      LOG(L_INFO) << tmsg->index
                  << "\tMain thread message, switch to second thread";
      second_es_->PostDelayed(1000, this, 0, tmsg);
    } else if (second_es_->IsThisThread(current_thread)) {
      LOG(L_INFO) << tmsg->index
                  << "\tSecond thread message, switch to main thread";
      main_es_->Post(this, 0, tmsg);
    } else {
      LOG(L_ERROR) << "Error! there is unknow thread run this message";
    }
  }
 private:
  vzes::EventService::Ptr   main_es_;
  vzes::EventService::Ptr   second_es_;

};

int main(void) {

  // Initialize the logging system
  vzes::LogMessage::LogTimestamps(true);
  vzes::LogMessage::LogContext(vzes::LS_INFO);
  vzes::LogMessage::LogThreads(true);

  vzes::EventService::Ptr main_es
    = vzes::EventService::CreateCurrentEventService("MainThread");

  TimeEvent te(main_es);
  te.Start();
  main_es->Run();

  return EXIT_SUCCESS;
}