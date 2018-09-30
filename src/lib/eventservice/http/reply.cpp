/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#include <string>
#include <sstream>
#include "eventservice/http/reply.h"
#include "eventservice/base/logging.h"
#include "eventservice/base/helpmethods.h"

namespace vzes {

namespace status_strings {

const char ok[] =
  "HTTP/1.0 200 OK\r\n";
const char created[] =
  "HTTP/1.0 201 Created\r\n";
const char accepted[] =
  "HTTP/1.0 202 Accepted\r\n";
const char no_content[] =
  "HTTP/1.0 204 No Content\r\n";
const char multiple_choices[] =
  "HTTP/1.0 300 Multiple Choices\r\n";
const char moved_permanently[] =
  "HTTP/1.0 301 Moved Permanently\r\n";
const char moved_temporarily[] =
  "HTTP/1.0 302 Moved Temporarily\r\n";
const char not_modified[] =
  "HTTP/1.0 304 Not Modified\r\n";
const char bad_request[] =
  "HTTP/1.0 400 Bad Request\r\n";
const char unauthorized[] =
  "HTTP/1.0 401 Unauthorized\r\n";
const char forbidden[] =
  "HTTP/1.0 403 Forbidden\r\n";
const char not_found[] =
  "HTTP/1.0 404 Not Found\r\n";
const char internal_server_error[] =
  "HTTP/1.0 500 Internal Server Error\r\n";
const char not_implemented[] =
  "HTTP/1.0 501 Not Implemented\r\n";
const char bad_gateway[] =
  "HTTP/1.0 502 Bad Gateway\r\n";
const char service_unavailable[] =
  "HTTP/1.0 503 Service Unavailable\r\n";

const std::string to_buffer(reply::status_type status) {
  switch (status) {
  case reply::ok:
    return (ok);
  case reply::created:
    return (created);
  case reply::accepted:
    return (accepted);
  case reply::no_content:
    return (no_content);
  case reply::multiple_choices:
    return (multiple_choices);
  case reply::moved_permanently:
    return (moved_permanently);
  case reply::moved_temporarily:
    return (moved_temporarily);
  case reply::not_modified:
    return (not_modified);
  case reply::bad_request:
    return (bad_request);
  case reply::unauthorized:
    return (unauthorized);
  case reply::forbidden:
    return (forbidden);
  case reply::not_found:
    return (not_found);
  case reply::internal_server_error:
    return (internal_server_error);
  case reply::not_implemented:
    return (not_implemented);
  case reply::bad_gateway:
    return (bad_gateway);
  case reply::service_unavailable:
    return (service_unavailable);
  default:
    return (internal_server_error);
  }
}

}  // namespace status_strings

namespace misc_strings {

const char name_value_separator[] = { ':', ' ' };
const char crlf[] = { '\r', '\n' };

HttpHead http_public_header[2] = {
  {"Access-Control-Allow-Origin", "*"},
  {"Connection", "close"}
};

}  // namespace misc_strings

const std::string reply::to_string() {
  bool found = false;
  for (std::size_t i = 0; i < headers.size(); i++) {
    if (headers[i].name == misc_strings::http_public_header[0].name
        || headers[i].name == misc_strings::http_public_header[1].name) {
      found = true;
      break;
    }
  }
  if (!found) {
    headers.push_back(misc_strings::http_public_header[0]);
    headers.push_back(misc_strings::http_public_header[1]);
  }

  std::stringstream ss;
  ss << status_strings::to_buffer(status);
  for (std::size_t i = 0; i < headers.size(); ++i) {
    HttpHead& h = headers[i];
    LOG(L_INFO) << h.name << ":" << h.value;
    ss << h.name << ": " << h.value << "\r\n";
  }
  ss << "\r\n";
  ss.write(content.c_str(), content.size());
  return ss.str();
}

namespace stock_replies {

const char ok[] = "";
const char created[] =
  "<html>"
  "<head><title>Created</title></head>"
  "<body><h1>201 Created</h1></body>"
  "</html>";
const char accepted[] =
  "<html>"
  "<head><title>Accepted</title></head>"
  "<body><h1>202 Accepted</h1></body>"
  "</html>";
const char no_content[] =
  "<html>"
  "<head><title>No Content</title></head>"
  "<body><h1>204 Content</h1></body>"
  "</html>";
const char multiple_choices[] =
  "<html>"
  "<head><title>Multiple Choices</title></head>"
  "<body><h1>300 Multiple Choices</h1></body>"
  "</html>";
const char moved_permanently[] =
  "<html>"
  "<head><title>Moved Permanently</title></head>"
  "<body><h1>301 Moved Permanently</h1></body>"
  "</html>";
const char moved_temporarily[] =
  "<html>"
  "<head><title>Moved Temporarily</title></head>"
  "<body><h1>302 Moved Temporarily</h1></body>"
  "</html>";
const char not_modified[] =
  "<html>"
  "<head><title>Not Modified</title></head>"
  "<body><h1>304 Not Modified</h1></body>"
  "</html>";
const char bad_request[] =
  "<html>"
  "<head><title>Bad Request</title></head>"
  "<body><h1>400 Bad Request</h1></body>"
  "</html>";
const char unauthorized[] =
  "<html>"
  "<head><title>Unauthorized</title></head>"
  "<body><h1>401 Unauthorized</h1></body>"
  "</html>";
const char forbidden[] =
  "<html>"
  "<head><title>Forbidden</title></head>"
  "<body><h1>403 Forbidden</h1></body>"
  "</html>";
const char not_found[] =
  "<html>"
  "<head><title>Not Found</title></head>"
  "<body><h1>404 Not Found</h1></body>"
  "</html>";
const char internal_server_error[] =
  "<html>"
  "<head><title>Internal Server Error</title></head>"
  "<body><h1>500 Internal Server Error</h1></body>"
  "</html>";
const char not_implemented[] =
  "<html>"
  "<head><title>Not Implemented</title></head>"
  "<body><h1>501 Not Implemented</h1></body>"
  "</html>";
const char bad_gateway[] =
  "<html>"
  "<head><title>Bad Gateway</title></head>"
  "<body><h1>502 Bad Gateway</h1></body>"
  "</html>";
const char service_unavailable[] =
  "<html>"
  "<head><title>Service Unavailable</title></head>"
  "<body><h1>503 Service Unavailable</h1></body>"
  "</html>";

std::string to_string(reply::status_type status) {
  switch (status) {
  case reply::ok:
    return ok;
  case reply::created:
    return created;
  case reply::accepted:
    return accepted;
  case reply::no_content:
    return no_content;
  case reply::multiple_choices:
    return multiple_choices;
  case reply::moved_permanently:
    return moved_permanently;
  case reply::moved_temporarily:
    return moved_temporarily;
  case reply::not_modified:
    return not_modified;
  case reply::bad_request:
    return bad_request;
  case reply::unauthorized:
    return unauthorized;
  case reply::forbidden:
    return forbidden;
  case reply::not_found:
    return not_found;
  case reply::internal_server_error:
    return internal_server_error;
  case reply::not_implemented:
    return not_implemented;
  case reply::bad_gateway:
    return bad_gateway;
  case reply::service_unavailable:
    return service_unavailable;
  default:
    return internal_server_error;
  }
}

}  // namespace stock_replies

reply reply::stock_reply(reply::status_type status) {
  reply rep;
  rep.status = status;
  rep.content = stock_replies::to_string(status);
  rep.headers.resize(3);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = HelpMethods::IntToStr(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = "text/html";
  return rep;
}

std::string HttpReqMessage::GetReqParam(const std::string key) {
  KeyValues::iterator iter = key_values.find(key);
  if (iter == key_values.end()) {
    return std::string();
  }
  return iter->second;
}
}  // namespace vzes

