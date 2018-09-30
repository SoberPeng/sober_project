/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include "eventservice/http/mime_types.h"
#include "eventservice/http/reply.h"
#include "eventservice/http/file_handler.h"
#include "eventservice/base/helpmethods.h"

namespace vzes {

file_handler::file_handler(EventService::Ptr event_service,
                           const std::string& doc_root)
  : HttpHandler(event_service),
    doc_root_(doc_root) {
}

bool file_handler::HandleRequest(AsyncHttpSocket::Ptr connect,
                                 HttpReqMessage& request) {
  if (!connect) {
    LOG(L_ERROR) << "Async Http Socket is null";
    return false;
  }
  reply& rep = connect->http_reply();
  do {
    // Decode url to path.
    std::string request_path;
    if (!url_decode(request.req_url, request_path)) {
      rep = reply::stock_reply(reply::bad_request);
      break;
    }

    // Request path must be absolute and not contain "..".
    if (request_path.empty() || request_path[0] != '/'
        || request_path.find("..") != std::string::npos) {
      rep = reply::stock_reply(reply::bad_request);
      break;
    }

    // If path ends in slash (i.e. is a directory) then add "index.html".
    if (request_path[request_path.size() - 1] == '/') {
      request_path += "index.html";
    }

    // Determine the file extension.
    std::size_t last_slash_pos = request_path.find_last_of("/");
    std::size_t last_dot_pos = request_path.find_last_of(".");
    std::string extension;
    if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos) {
      extension = request_path.substr(last_dot_pos + 1);
    }

    // Open the file to send back.
    std::string full_path = doc_root_ + request_path;
    std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
    if (!is) {
      rep = reply::stock_reply(reply::not_found);
      break;
    }

    // Fill out the reply to be sent to the client.
    rep.status = reply::ok;
    char buf[512];
    while (is.read(buf, sizeof(buf)).gcount() > 0) {
      rep.content.append(buf, (uint32)(is.gcount()));
    }
    rep.headers.resize(3);
    rep.headers[0].name = "Content-Length";
    rep.headers[0].value = HelpMethods::IntToStr(rep.content.size());
    rep.headers[1].name = "Cache-Control";
    rep.headers[1].value = "max-age=3600";
    rep.headers[2].name = "Content-Type";
    rep.headers[2].value = extension_to_type(extension);
    LOG(L_WARNING) << rep.headers[1].value;
  } while(0);
  return connect->AsyncWriteRepMessage(rep);
  // return true;
}

bool file_handler::url_decode(const std::string& in, std::string& out) {
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i) {
    if (in[i] == '%') {
      if (i + 3 <= in.size()) {
        int value = 0;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value) {
          out += static_cast<char>(value);
          i += 2;
        } else {
          return false;
        }
      } else {
        return false;
      }
    } else if (in[i] == '+') {
      out += ' ';
    } else if (in[i] == '?') {
      break;
    } else {
      out += in[i];
    }
  }
  return true;
}

}  // namespace vzes

