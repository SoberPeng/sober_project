/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/


#ifndef HTTP_SERVER4_FILE_HANDLER_HPP
#define HTTP_SERVER4_FILE_HANDLER_HPP

#include <string>
#include "eventservice/http/handlermanager.h"

namespace vzes {

struct reply;

/// The common handler for all incoming requests.
class file_handler : public HttpHandler {
 public:
  /// Construct with a directory containing files to be served.
  explicit file_handler(EventService::Ptr event_service,
                        const std::string& doc_root);
  /// Handle a request and produce a reply.
  virtual bool HandleRequest(AsyncHttpSocket::Ptr connect,
                             HttpReqMessage& request);

 private:
  /// The directory containing the files to be served.
  std::string doc_root_;
  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);
};

}  // namespace vzes

#endif  // HTTP_SERVER4_FILE_HANDLER_HPP

