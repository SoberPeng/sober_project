/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#ifndef HTTP_SERVER4_MIME_TYPES_HPP
#define HTTP_SERVER4_MIME_TYPES_HPP

#include <string>

namespace vzes {

/// Convert a file extension into a MIME type.
std::string extension_to_type(const std::string& extension);

}  // namespace vzes

#endif  // HTTP_SERVER4_MIME_TYPES_HPP

