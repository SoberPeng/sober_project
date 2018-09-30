/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#ifndef HTTP_SERVER4_REPLY_HPP
#define HTTP_SERVER4_REPLY_HPP

#include <string>
#include <vector>
#include <map>

namespace vzes {

struct HttpHead {
  std::string name;
  std::string value;
};

struct UrlFields {
  std::string schema;
  std::string host;
  std::string port;
  std::string path;
  std::string query;
  std::string fragment;
  std::string userinfo;
};

typedef std::map<std::string, std::string> KeyValues;

struct HttpReqMessage {
  std::string method;
  std::string req_url;
  UrlFields   url_fields;
  KeyValues   key_values;
  std::vector<HttpHead> http_headers;
  std::string body;
  std::string GetReqParam(const std::string key);
};

/// A reply to be sent to a client.
struct reply {
  /// The status of the reply.
  enum status_type {
    ok = 200,
    created = 201,
    accepted = 202,
    no_content = 204,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    not_modified = 304,
    bad_request = 400,
    unauthorized = 401,
    forbidden = 403,
    not_found = 404,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503
  } status;

  /// The headers to be included in the reply.
  std::vector<HttpHead> headers;

  /// The content to be sent in the reply.
  std::string content;

  /// Convert the reply into a vector of buffers. The buffers do not own the
  /// underlying memory blocks, therefore the reply object must remain valid and
  /// not be changed until the write operation has completed.
  const std::string to_string();

  /// Get a stock reply.
  static reply stock_reply(status_type status);
};

namespace stock_replies {
std::string to_string(reply::status_type status);
}

} // namespace http

#endif // HTTP_SERVER4_REPLY_HPP

