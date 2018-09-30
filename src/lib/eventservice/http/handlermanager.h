/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#ifndef DEVICEGROUP_HTTP_HANDLER_MANAGER_H_
#define DEVICEGROUP_HTTP_HANDLER_MANAGER_H_

#include <map>
#include "eventservice/base/basicincludes.h"
#include "eventservice/net/asynchttpsocket.h"

namespace vzes {

class HttpHandler : public boost::noncopyable {
 public:
  typedef boost::shared_ptr<HttpHandler> Ptr;
  explicit HttpHandler(EventService::Ptr event_service)
    : event_service_(event_service) {
  }
  virtual ~HttpHandler() {
    LOG(L_INFO) << "Delete HttpHandler";
  }
  virtual bool HandleRequest(AsyncHttpSocket::Ptr connect,
                             HttpReqMessage& request) = 0;
 private:
  EventService::Ptr event_service_;
};

class HandlerManager : public boost::noncopyable,
  public boost::enable_shared_from_this<HandlerManager> {
 public:
  typedef boost::shared_ptr<HandlerManager> Ptr;
  HandlerManager();
  virtual ~HandlerManager();
  bool SetDefualtHandler(HttpHandler::Ptr handler);
  bool AddRequestHandler(const std::string path, HttpHandler::Ptr handler);
  void OnNewRequest(AsyncHttpSocket::Ptr connect, HttpReqMessage& request);
 private:
  HttpHandler::Ptr                        defualt_handler_;
  typedef std::map<std::string, HttpHandler::Ptr> Handlers;
  Handlers                                handlers_;
};
}  // namespace vzes

#endif  // DEVICEGROUP_HTTP_HANDLER_MANAGER_H_
