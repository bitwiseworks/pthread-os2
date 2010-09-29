#ifndef __TLS_H__
#define __TLS_H__

#include <os2.h>

#ifdef __cplusplus
extern "C" {
#endif

ULONG    TlsAlloc( void);
BOOL     TlsFree( ULONG);
PVOID	 TlsGetValue( ULONG);
BOOL     TlsSetValue( ULONG, PVOID);
void     TlsAllocThreadLocalMemory( void);
void     TlsFreeThreadLocalMemory( void);

#ifdef __cplusplus
}
#endif

#endif // __TLS_H__

