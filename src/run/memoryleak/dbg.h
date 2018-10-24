/****************************************************************************
 *
 *  Copyright (C) 2000-2001 RealNetworks, Inc. All rights reserved.
 *
 *  This program is free software.  It may be distributed under the terms
 *  in the file LICENSE, found in the top level of the source distribution.
 *
 */

#ifndef _DBG_H 
#define _DBG_H 

#include <assert.h> 

#ifdef NDEBUG
inline void dbgout(const char* fmt, ...) {}
inline void dump_alloc_heaps(void) {}
#else 
void dbgout(const char* fmt, ...);

void* operator new(size_t n, char *file, int line);
void* operator new[](size_t n, char *file, int line);

void  operator delete(void* p);
void  operator delete[](void* p);

void* operator new(size_t n);
void* operator new[](size_t n);

#define new new(__FILE__,__LINE__)

void dump_alloc_heaps(void);

#endif 

#define assert_or_ret(cond) { assert(cond); if( !(cond) ) return; } 
#define assert_or_retv(val,cond) { assert(cond); if( !(cond) ) return (val); } 

#endif //ndef _DBG_H 