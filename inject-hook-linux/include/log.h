/**
 * Copyright (c) 2017 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `log.c` for details.
 *  Modifications:
 *  Copyright (c) 2019, Max Compston, Embedded Software Solutions 
 * -- Modified to align with Android Log Levels
 * -- Added Android Tag
 */

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>

#define LOG_VERSION "0.1.0"

typedef void (*log_LockFn)(void *udata, int lock);

#ifdef __cplusplus
extern "C" {
#endif

void log_set_udata(void *udata);
void log_set_lock(log_LockFn fn);
void log_set_fp(FILE *fp);
void log_set_level(int level);
void log_set_quiet(int enable);
void log_log(int level, const char *tag, const char *file, int line, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
