/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#include "eventservice/http/mime_types.h"
#include <iostream>

namespace vzes {

struct mapping {
  const char* extension;
  const char* mime_type;
} mappings[] = {
  { "gif", "image/gif" },
  { "htm", "text/html" },
  { "html", "text/html" },
  { "jpg", "image/jpeg" },
  { "png", "image/png" },
  { "css", "text/css" },
  { "js", "application/x-javascript" },
  { "svg", "image/svg+xml" },
  { "woff", "application/font-woff" },
  { "woff2", "application/font-woff2" },
  { "otf", "application/x-font-opentype" },
  { "eot", "application/vnd.ms-fontobject" },
  { "ttf", "application/x-font-truetype" },
  { "json", "application/json" },
  { 0, 0 }  // Marks end of list.
};

std::string extension_to_type(const std::string& extension) {
  for (mapping* m = mappings; m->extension; ++m) {
    if (m->extension == extension) {
      return m->mime_type;
    }
  }

  return "text/plain";
}

}  // namespace vzes

