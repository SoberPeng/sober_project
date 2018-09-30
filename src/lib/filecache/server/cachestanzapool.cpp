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

#include "filecache/server/cachestanzapool.h"
#include "eventservice/base/logging.h"
#include <string.h>

namespace cache {

CachedStanzaPool *CachedStanzaPool::pool_instance_ = NULL;

CachedStanzaPool *CachedStanzaPool::Instance() {
  if (!pool_instance_) {
    pool_instance_ = new CachedStanzaPool();
  }
  return pool_instance_;
}

CachedStanzaPool::CachedStanzaPool() {
}

CachedStanzaPool::~CachedStanzaPool() {
  //stanzas_.clear();
}

CachedStanza::Ptr CachedStanzaPool::TakeStanza() {
  vzes::CritScope stanza_mutex(&pool_mutex_);
  //std::list.size()的效率极低
  if (stanzas_.size()) {
    CachedStanza::Ptr perfect_stanza;
    perfect_stanza.reset(stanzas_.front(), &CachedStanzaPool::RecyleBuffer);
    stanzas_.pop_front();
    //LOG(L_INFO) << "Task stanza by back size = " << stanzas_.size()
    //            << " perfect_stanza use count " << perfect_stanza.use_count();
    return perfect_stanza;
  } else {
    LOG(L_WARNING) << "take stanza failed, no available stanzas";
    return CachedStanza::Ptr();
  }
}

void CachedStanzaPool::RecyleStanza(CachedStanza *stanza) {
  vzes::CritScope stanza_mutex(&pool_mutex_);
  stanza->ResetDefualtState();
  stanzas_.push_back(stanza);
  //LOG(L_INFO) << "Recycle stanza, available stanza count =  " << stanzas_.size();
}

void CachedStanzaPool::SetDefaultCachedSize(std::size_t stanza_size) {
  LOG(L_INFO) << "Set default stanza pool size: " << stanza_size;
  for (std::size_t i = 0; i < stanza_size; i++) {
    stanzas_.push_back(new CachedStanza());
  }
}

void CachedStanzaPool::RecyleBuffer(void *stanza) {
  CachedStanzaPool::Instance()->RecyleStanza((CachedStanza *)stanza);
}

////////////////////////////////////////////////////////////////////////////////

uint32 CachedStanza::stanza_count = 0;

CachedStanza::CachedStanza()
  : is_saved_(false) {
  cache_data_ = MemBuffer::CreateMemBuffer();
  stanza_count++;
  LOG(L_WARNING) << "Create stanza, count = " << stanza_count;
}

CachedStanza::~CachedStanza() {
  stanza_count--;
  LOG(L_WARNING) << "Delete Stanza, count = " << stanza_count;
}

bool CachedStanza::IsSaved() {
  vzes::CritScope stanza_mutex(&stanza_mutex_);
  return is_saved_;
}

void CachedStanza::SetData(MemBuffer::Ptr data) {
  cache_data_ = data;
}

void CachedStanza::SaveConfimation() {
  vzes::CritScope stanza_mutex(&stanza_mutex_);
  BOOST_ASSERT(!is_saved_);
  is_saved_ = true;
}

void CachedStanza::ResetDefualtState() {
  if (cache_data_) {
    cache_data_->Clear();
  }

  path_.clear();
  is_saved_ = false;
}

}
