#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <process.h>

#if defined(DEBUG_PRINTF)
#define DBG(msg, ...) fprintf(stderr, "PTHREAD: DEBUG: " msg, __VA_ARGS__)
#define DBG_TID(msg, ...) fprintf(stderr, "[%05d:%02d] PTHREAD: DEBUG:" msg, getpid(), _gettid(), ##__VA_ARGS__)
#else
#define DBG(msg, ...) do {} while(0)
#define DBG_TID(msg, ...) do {} while(0)
#endif

#define WRN(msg, ...) fprintf(stderr, "PTHREAD: WARNING: " msg, __VA_ARGS__)
#define WRN_TID(msg, ...) fprintf(stderr, "[%05d:%02d] PTHREAD: WARNING: " msg, getpid(), _gettid(), ##__VA_ARGS__)

#endif // __DEBUG_H__
