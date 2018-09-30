/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#include "eventservice/http/handlermanager.h"
#include <iostream>

namespace vzes {

HandlerManager::HandlerManager() {
}

HandlerManager::~HandlerManager() {
  LOG(L_INFO) << "Delete HandlerManager";
}

bool HandlerManager::SetDefualtHandler(HttpHandler::Ptr handler) {
  if (defualt_handler_) {
    LOG(L_INFO) << "It was set the defualt handler";
    return false;
  }
  defualt_handler_ = handler;
  return true;
}

bool HandlerManager::AddRequestHandler(const std::string path,
                                       HttpHandler::Ptr handler) {
  Handlers::iterator iter = handlers_.find(path);
  if (iter != handlers_.end()) {
    LOG(L_INFO) << "It has beend a hand";
    return false;
  }
  handlers_.insert(
    std::map<std::string, HttpHandler::Ptr>::value_type(path, handler));
  return true;
}

void HandlerManager::OnNewRequest(AsyncHttpSocket::Ptr connect,
                                  HttpReqMessage& request) {
  LOG(L_INFO) << request.method << " : " << request.req_url;
  Handlers::iterator iter = handlers_.find(request.req_url);
  if (iter != handlers_.end()) {
    iter->second->HandleRequest(connect, request);
  } else {
    defualt_handler_->HandleRequest(connect, request);
  }
}
}  // namespace vzes
