/* Copyright (c) 2015, Simone 'evilsocket' Margaritelli
   Copyright (c) 2015, Jorrit 'Chainfire' Jongma
   Copyright (c) 2019, Max Compston, Embedded Software Solutions
   See LICENSE file for details */

#ifndef INJECT_H
#define INJECT_H

// INJECTLOG( "some message %d %d %d", 1, 2, 3 );
#ifndef LOG_TO_STDOUT
#define INJECTLOG(F,...) \
    __android_log_print( ANDROID_LOG_DEBUG, "LIBINJECT", F, ##__VA_ARGS__ )
#else
#define INJECTLOG(F,...) \
    __stdout_log_print( ANDROID_LOG_DEBUG, "LIBINJECT", F, ##__VA_ARGS__ )
#endif

// Find pid for process
pid_t libinject_find_pid_of(const char* process);

// Load library in process pid, returns 0 on success
int libinject_inject(pid_t pid, char* library);

#endif
