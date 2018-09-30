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
#include "eventservice/event/thread.h"
#include "eventservice/event/signalevent.h"

class SignalEventTest : public vzes::MessageHandler {
 public:
  SignalEventTest(vzes::Thread *thread)
    : main_thread_(thread),
      second_thread_(NULL) {
    signal_event_ = vzes::SignalEvent::CreateSignalEvent();
    time_out_ = 10;
  }
  virtual ~SignalEventTest() {
  }
  void Start() {
    second_thread_ = new vzes::Thread();
    second_thread_->Start();
    main_thread_->PostDelayed(1000, this, 0);
  }
  virtual void OnMessage(vzes::Message *msg) {
    vzes::Thread *current_thread = vzes::Thread::Current();
    if (current_thread == main_thread_) {
      // Wait Two Second
      LOG(L_INFO) << "1. Post message to scond thread";
      second_thread_->PostDelayed(5000, this, 0);
      time_out_--;
      LOG(L_INFO) << "2. signal event with "<<time_out_<<" sconds";
      signal_event_->WaitSignal(time_out_ * 1000);
      LOG(L_INFO) << "[4]. Signal done";
    } else if (second_thread_ == current_thread) {
      LOG(L_INFO) << "3. Receive thread message";
      if (!signal_event_->IsTimeout()) {
        LOG(L_INFO) << "3.1 Trigger this signal event";
        signal_event_->TriggerSignal();
        LOG(L_INFO) << "3.2 Trigger finish";
      } else {
        LOG(L_INFO) << "~3.1 The trigger is time out";
      }
      LOG(L_INFO) << "4. Post message to main thread";
      main_thread_->PostDelayed(1000, this, 0);
    } else {
      LOG(L_ERROR) << "Error! there is unknow thread run this message";
    }
  }
 private:
  vzes::Thread                *main_thread_;
  vzes::Thread                *second_thread_;
  vzes::SignalEvent::Ptr      signal_event_;
  uint32                      time_out_;
};

int main(void) {

  // Initialize the logging system
  vzes::LogMessage::LogTimestamps(true);
  vzes::LogMessage::LogContext(vzes::LS_INFO);
  vzes::LogMessage::LogThreads(true);

  vzes::Thread *thread = vzes::Thread::Current();

  SignalEventTest te(thread);

  te.Start();
  // thread->Start();
  thread->Run();
  thread->Stop();

  return EXIT_SUCCESS;
}