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
#include "eventservice/net/eventservice.h"
#include "eventservice/base/logging.h"

class Base : public boost::noncopyable,
  public boost::enable_shared_from_this<Base> {
 public:
  typedef boost::shared_ptr<Base> Ptr;
  Base() {
    LOG(L_INFO) << "Create base class";
  }
  virtual ~Base() {
    LOG(L_INFO) << "Destory base class";
  }
 private:
};

class Drive : public Base,
  public boost::enable_shared_from_this<Drive> {
 public:
  typedef boost::shared_ptr<Drive> Ptr;
  Drive() {
    LOG(L_INFO) << "Create Drive class";
  }
  virtual ~Drive() {
    LOG(L_INFO) << "Destory Drive class";
  }
 private:
};

int main(void) {
  // Initialize the logging system
  vzes::LogMessage::LogTimestamps(true);
  vzes::LogMessage::LogContext(vzes::LS_INFO);
  vzes::LogMessage::LogThreads(true);

  //const char a = 0;
  //const char b = 0;
  //ASSERT_RETURN_FAILURE(a == b, -1);

  {
    Drive *drive = new Drive();
    Drive::Ptr pdrive(drive);
    Base::Ptr  pbase = pdrive;
  }
  {
    Drive *drive = new Drive();
    Base::Ptr pbase(drive);
    Drive::Ptr  pdrive = boost::dynamic_pointer_cast<Drive>(pbase);
  }
  return EXIT_SUCCESS;
}