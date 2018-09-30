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

#include "eventservice/tls/tls.h"
#include "eventservice/event/thread.h"

#include <map>


namespace vzes {

#ifdef WIN32
Tls::Tls() {
}

Tls::~Tls() {
}

void Tls::CreateKey(TLS_KEY *key) {
  *key = TlsAlloc();
}

void Tls::DeleteKey(TLS_KEY key) {
  TlsFree(key);
}

void Tls::SetKey(TLS_KEY key, void *value) {
  TlsSetValue(key, value);
}

void *Tls::GetKey(TLS_KEY key) {
  return TlsGetValue(key);
}
#elif LITEOS

typedef std::map<pthread_t, void *> LiteOSKeys;

Tls::Tls() {
}

Tls::~Tls() {
}

void Tls::CreateKey(TLS_KEY *key) {
  *key = (void *)(new LiteOSKeys());
}

void Tls::DeleteKey(TLS_KEY key) {
  LiteOSKeys *liteos_keys = (LiteOSKeys *)key;
  delete liteos_keys;
}

void Tls::SetKey(TLS_KEY key, void *value) {
  LiteOSKeys *liteos_keys = (LiteOSKeys *)key;
  pthread_t id = pthread_self();
  (*liteos_keys)[id] = value;
}

void *Tls::GetKey(TLS_KEY key) {
  LiteOSKeys *liteos_keys = (LiteOSKeys *)key;
  pthread_t id = pthread_self();
  return (*liteos_keys)[id];
}
#elif POSIX
Tls::Tls() {
}

Tls::~Tls() {
}

void Tls::CreateKey(TLS_KEY *key) {
  pthread_key_create(key, NULL);
}

void Tls::DeleteKey(TLS_KEY key) {
  pthread_key_delete(key);
}

void Tls::SetKey(TLS_KEY key, void *value) {
  pthread_setspecific(key, value);
}

void *Tls::GetKey(TLS_KEY key) {
  return pthread_getspecific(key);
}

//typedef std::map<pthread_t, void *> LiteOSKeys;
//
//Tls::Tls() {
//}
//
//Tls::~Tls() {
//}
//
//void Tls::CreateKey(TLS_KEY *key) {
//  *key = (void *)(new LiteOSKeys());
//}
//
//void Tls::DeleteKey(TLS_KEY key) {
//  LiteOSKeys *liteos_keys = (LiteOSKeys *)key;
//  delete liteos_keys;
//}
//
//void Tls::SetKey(TLS_KEY key, void *value) {
//  LiteOSKeys *liteos_keys = (LiteOSKeys *)key;
//  pthread_t id = pthread_self();
//  (*liteos_keys)[id] = value;
//}
//
//void *Tls::GetKey(TLS_KEY key) {
//  LiteOSKeys *liteos_keys = (LiteOSKeys *)key;
//  pthread_t id = pthread_self();
//  return (*liteos_keys)[id];
//}
#endif
}  // namespace vzes
