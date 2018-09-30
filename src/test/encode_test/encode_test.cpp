/*
 * vzsdk
 * Copyright 2013 - 2016, Vzenith Inc.
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


#include <cstdio>
#include "encode/encode.h"
#include <string.h>

void old_encode() {
  char s1[10000];
  char s2[] = "Vzenith is the best company in the world.臻识科技是世界上最好的公司。zenith is the best company in the world.";
  FILE *fp1, *fp2;
  //Encode::CharEncode::Gb2312ToUtf8(s1, s2);
  //Encode::CharEncode::RGN_Gb2312_To_UTF8((unsigned char *)s1, 10000, (unsigned char *)s2, strlen(s2));
  fp1 = fopen("2.txt", "wb+");
  fp2 = fopen("1.txt", "wb+");
  fwrite(s1, sizeof(char), strlen(s1), fp1);
  fwrite(s2, sizeof(char), strlen(s2), fp2);
}

void new_encode() {
  std::string s1, s2;
  s1 = "PHP 是世界上最好的语言。";
  s2 = "Vzenith is the best company in the world.臻识科技是世界上最好的公司。zenith is the best company in the world.";
  FILE *fp1, *fp2;
  Gb2312ToUtf8(s1, s2);
  //Utf8ToGb2312(s1, s2);
  fp1 = fopen("1.txt", "wb+");
  fp2 = fopen("2.txt", "wb+");
  fwrite(s1.c_str(), sizeof(char), s1.size(), fp1);
  fwrite(s2.c_str(), sizeof(char), s2.size(), fp2);
}

int main(void) {
  new_encode();
  return 0;
}