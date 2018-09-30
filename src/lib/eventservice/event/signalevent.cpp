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

#include "eventservice/event/signalevent.h"
#include "eventservice/base/timeutils.h"

#ifdef POSIX
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#endif

#ifdef LITEOS

#if __cplusplus
extern "C" {
#endif
  int vz_creat_select_fd(char *);
#if __cplusplus
}
#endif
#endif // LITEOS
namespace vzes {

#ifdef WIN32
class SignalEventWin32Impl : public SignalEvent,
  public boost::noncopyable,
  public boost::enable_shared_from_this<SignalEventWin32Impl>,
  public sigslot::has_slots<> {
 public:
  SignalEventWin32Impl()
    : timeout_millsecond_(0) {
    signal_wakeup_ = WSACreateEvent();
  }

  virtual ~SignalEventWin32Impl() {
    WSACloseEvent(signal_wakeup_);
  }

  virtual int WaitSignal(uint32 wait_millsecond) {
    int res = SIGNAL_EVENT_FAILURE;
    uint32 wait_time = wait_millsecond;
    timeout_millsecond_ = Time() + wait_millsecond;
    WSAResetEvent(signal_wakeup_);
    // do {
    HANDLE events[1] = {signal_wakeup_};
    DWORD dw = WSAWaitForMultipleEvents(1,
                                        &events[0],
                                        false,
                                        wait_time,
                                        false);
    if (dw == WSA_WAIT_FAILED) {
      res = SIGNAL_EVENT_FAILURE;
    } else if (dw == WSA_WAIT_TIMEOUT) {
      res = SIGNAL_EVENT_TIMEOUT;
    } else {
      res = SIGNAL_EVENT_DONE;
    }
    // } while(0);
    timeout_millsecond_ = 0;
    return res;
  }

  virtual bool IsTimeout() {
    CritScope cr(&crit_);
    if (timeout_millsecond_ == 0) {
      return true;
    }
    uint32 invt = Time() - timeout_millsecond_;
    return invt > 0 ? false : true;
  }
  virtual void TriggerSignal() {
    CritScope cr(&crit_);
    WSASetEvent(signal_wakeup_);
  }
 private:
  CriticalSection crit_;
  WSAEVENT        signal_wakeup_;
  uint32          timeout_millsecond_;
};

SignalEvent::Ptr SignalEvent::CreateSignalEvent() {
  return SignalEvent::Ptr(new SignalEventWin32Impl());
}
#else

#ifdef LITEOS
#include <stdio.h>
#define STATIC_SUFFIX "vz_sel_"
#define MAX_PIPE_FILE 32

class PosixSignal : public Signal {
 public:
  PosixSignal() : signaled_(false) {
    char pipe_file[MAX_PIPE_FILE] = {0};
    snprintf(pipe_file, MAX_PIPE_FILE, "vz_sel_%d", pipe_count_++);
    LOG(L_INFO) << "Create pipe file " << pipe_file;
    fd_ = vz_creat_select_fd(pipe_file);
    if (fd_ < 0) {
      LOG(L_ERROR) << "Failure to open the /dev/VzLiteOSPoll";
      ASSERT(false);
    }
  }
  virtual ~PosixSignal() {
    if (fd_ != -1) {
      close(fd_);
      fd_ = -1;
    }
  }
  virtual void ResetWakeEvent() {
    // It is not possible to perfectly emulate an auto-resetting event with
    // pipes.  This simulates it by resetting before the event is handled.
    CritScope cs(&crit_);
    if (signaled_) {
      int val = 0;
      int len = sizeof(val);
      if (VZ_VERIFY(len == read(fd_, &val, len))) {
        signaled_ = false;
      }
    }
  }

  virtual void SignalWakeup() {
    CritScope cs(&crit_);
    if (!signaled_) {
      int val = 0;
      int len = sizeof(val);
      if (VZ_VERIFY(len == write(fd_, &val, len))) {
        signaled_ = true;
      }
    }
  }

  virtual uint32 GetEvents() const {
    return DE_READ;
  }
  virtual SOCKET GetSocket() {
    return fd_;
  }
 private:
  int  fd_;
  bool signaled_;
  CriticalSection crit_;
  static int pipe_count_ ;
};

int PosixSignal::pipe_count_ = 0;
#elif POSIX

class PosixSignal : public Signal {
 public:
  PosixSignal() : signaled_(false) {
    if (pipe(afd_) < 0)
      LOG(LERROR) << "pipe failed";
  }
  virtual ~PosixSignal() {
    close(afd_[0]);
    close(afd_[1]);
  }
  virtual void ResetWakeEvent() {
    // It is not possible to perfectly emulate an auto-resetting event with
    // pipes.  This simulates it by resetting before the event is handled.
    CritScope cs(&crit_);
    if (signaled_) {
      int val = 0;
      int len = sizeof(val);
      if (VZ_VERIFY(len == read(afd_[0], &val, len))) {
        signaled_ = false;
      }
    }
  }
  virtual void SignalWakeup() {
    CritScope cs(&crit_);
    if (!signaled_) {
      int val = 0;
      int len = sizeof(val);
      if (VZ_VERIFY(len == write(afd_[1], &val, len))) {
        signaled_ = true;
      }
    }
  }
  virtual uint32 GetEvents() const {
    return DE_READ;
  }
  virtual SOCKET GetSocket() {
    return afd_[0];
  }
 private:
  int afd_[2];
  bool signaled_;
  CriticalSection crit_;
};

#endif

class SignalEventLinuxImpl : public SignalEvent,
  public boost::noncopyable,
  public boost::enable_shared_from_this<SignalEventLinuxImpl>,
  public sigslot::has_slots<> {
 public:
  SignalEventLinuxImpl()
    : timeout_millsecond_(0) {
    signal_ = Signal::CreateSignal();
  }

  virtual ~SignalEventLinuxImpl() {
    delete signal_;
  }

  virtual int WaitSignal(uint32 wait_millsecond) {

    int res = SIGNAL_EVENT_FAILURE;

    timeout_millsecond_ = Time() + wait_millsecond;

    struct timeval *ptvWait = NULL;
    struct timeval tvWait;
    tvWait.tv_sec   = wait_millsecond / 1000;
    tvWait.tv_usec  = (wait_millsecond % 1000) * 1000;
    ptvWait         = &tvWait;
    int    fd       = signal_->GetSocket();

    fd_set fdsRead;
    FD_ZERO(&fdsRead);
    FD_SET(fd, &fdsRead);
    int fdmax = fd;

    // Wait then call handlers as appropriate
    // < 0 means error
    // 0 means timeout
    // > 0 means count of descriptors ready
    int n = select(fdmax + 1, &fdsRead, NULL, NULL, ptvWait);
    // If error, return error.
    if (n < 0) {
      if (errno != EINTR) {
        LOG_E(LS_ERROR, EN, errno) << "select";
        res = SIGNAL_EVENT_FAILURE;
      }
      // Else ignore the error and keep going. If this EINTR was for one of the
      // signals managed by this PhysicalSocketServer, the
      // PosixSignalDeliveryDispatcher will be in the signaled state in the next
      // iteration.
    } else if (n == 0) {
      // If timeout, return success
      ResetWakeEvent();
      res = SIGNAL_EVENT_TIMEOUT;
    } else {
      if (FD_ISSET(fd, &fdsRead)) {
        FD_CLR(fd, &fdsRead);
      }
      ResetWakeEvent();
      res = SIGNAL_EVENT_DONE;
    }

    timeout_millsecond_ = 0;
    return res;
  }

  virtual bool IsTimeout() {
    CritScope cr(&crit_);
    if (timeout_millsecond_ == 0) {
      return true;
    }
    int32 invt = Time() - timeout_millsecond_;
    return invt > 0 ? false : true;
  }

  virtual void TriggerSignal() {
    signal_->SignalWakeup();
  }
  virtual void ResetWakeEvent() {
    signal_->ResetWakeEvent();
  }
 private:
  CriticalSection crit_;
  Signal          *signal_;
  uint32          timeout_millsecond_;
};

SignalEvent::Ptr SignalEvent::CreateSignalEvent() {
  return SignalEvent::Ptr(new SignalEventLinuxImpl());
}

////////////////////////////////////////////////////////////////////////////////
Signal *Signal::CreateSignal() {
  return new PosixSignal();
}
#endif
} // namespace vzes
