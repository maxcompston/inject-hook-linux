/* Copyright (c) 2015, Simone 'evilsocket' Margaritelli
   Copyright (c) 2015, Jorrit 'Chainfire' Jongma
   Copyright (c) 2019, Max Compston, Embedded Software Solutions
   See LICENSE file for details */

#ifndef HOOK_H
#define HOOK_H

#include <sys/types.h>
#include <unistd.h>

#include "linker.h"

static size_t page_size;
#define ALIGN_ADDR(addr) ((void*)((size_t)(addr) & ~(page_size - 1)))

// No need to call manually, use REGISTERHOOK
void _libhook_register(const char* name, uintptr_t* original, uintptr_t hook);

// Hook all REGISTERHOOK'd hooks
// Test if you actually need the parameters set to 1, using 0 is safer if it actually works,
// as these are brute-force methods that have a small chance of data corruption
void libhook_hook(int patch_module_ro, int patch_module_rw);

// HOOKLOG( "some message %d %d %d", 1, 2, 3 );
#ifndef LOG_TO_FILE
#define HOOKLOG(F,...) \
    __android_log_print( ANDROID_LOG_DEBUG, "LIBHOOK", F, ##__VA_ARGS__ )
#else
#define HOOKLOG(F,...) \
    log_log( ANDROID_LOG_DEBUG, "LIBHOOK", __FILE__, __LINE__, F, ##__VA_ARGS__ )
#endif

// DEFINEHOOK( returnType, functionName, argSpec ) { ... }
//
// DEFINEHOOK( int, functionName, (int arg1, int arg2) ) {
//     return ORIGINAL( functionName, arg1, arg2 );
// }
#define DEFINEHOOK( RET_TYPE, NAME, ARGS ) \
    typedef RET_TYPE (* NAME ## _t)ARGS; \
    NAME ## _t original_ ## NAME; \
    RET_TYPE hook_ ## NAME ARGS

// ORIGINAL( functionName, arg1, arg2, ... );
#define ORIGINAL( NAME, ... ) \
    (original_ ## NAME)( __VA_ARGS__ )

// REGISTERHOOK( functionName );
#define REGISTERHOOK( NAME ) \
    _libhook_register( #NAME, (uintptr_t*)&original_ ## NAME, (uintptr_t)hook_ ## NAME )

// DEFINEHOOKPP( returnType, functionNickName, argSpec ) { ... }
//
// DEFINEHOOKPP( int, functionName, (int arg1, int arg2) ) {
//     return ORIGINALPP( arg1, arg2 );
// }
#define DEFINEHOOKPP( RET_TYPE, NAME, ARGS ) \
    class NAME ## Proxy { \
    public: \
        typedef RET_TYPE(NAME ## Proxy::*NAME ## _t)ARGS; \
        static NAME ## _t original; \
        static NAME ## _t hook; \
        RET_TYPE NAME ARGS; \
    }; \
    NAME ## Proxy::NAME ## _t NAME ## Proxy::original = NULL; \
    NAME ## Proxy::NAME ## _t NAME ## Proxy::hook = &NAME ## Proxy::NAME; \
    RET_TYPE NAME ## Proxy::NAME ARGS

// ORIGINALPP( arg1, arg2, ... );
#define ORIGINALPP( ... ) \
    (this->*original)( __VA_ARGS__ )

// REGISTERHOOKPP( "functionMangledName", functionNickName )
#define REGISTERHOOKPP( MANGLED, NICK ) \
    _libhook_register( MANGLED, (uintptr_t*)&NICK ## Proxy::original, *(uintptr_t*)&NICK ## Proxy::hook);

#endif
