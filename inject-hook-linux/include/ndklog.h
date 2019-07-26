/* Excerpts:
   Copyright (c) 2008, The Android Open Source Project

   Modifications:
   Copyright (c) 2015, Jorrit 'Chainfire' Jongma
   Copyright (c) 2019, Max Compston, Embedded Software Solutions

   See LICENSE file for details */

// Slightly modified cutils/log.h for use in NDK
//
// define LOG_TAG prior to inclusion
//
// if you define DEBUG or LOG_DEBUG before inclusion, VDIWE are logged
// if LOG_SILENT is defined, nothing is logged (overrides (LOG_)DEBUG)
// if none of the above are defined, VD are ignored and IWE are logged
//
// if LOG_TO_FILE is defined, logging is to Log File

#pragma GCC diagnostic ignored "-Wwrite-strings"

#ifndef _NDK_LOG_H
#define _NDK_LOG_H

#define LOG_TO_FILE

#ifndef LOG_TO_FILE
#include <android/log.h>
#else
#include <stdarg.h>
#endif 

/**
 * Android log priority values, in increasing order of priority.
 */
typedef enum android_LogPriority 
{
  /** For internal use only.  */
  ANDROID_LOG_UNKNOWN = 0,
  /** The default priority, for internal use only.  */
  ANDROID_LOG_DEFAULT, /* only for SetMinPriority() */
  /** Verbose logging. Should typically be disabled for a release apk. */
  ANDROID_LOG_VERBOSE,
  /** Debug logging. Should typically be disabled for a release apk. */
  ANDROID_LOG_DEBUG,
  /** Informational logging. Should typically be disabled for a release apk. */
  ANDROID_LOG_INFO,
  /** Warning logging. For use with recoverable failures. */
  ANDROID_LOG_WARN,
  /** Error logging. For use with unrecoverable failures. */
  ANDROID_LOG_ERROR,
  /** Fatal logging. For use when aborting. */
  ANDROID_LOG_FATAL,
  /** For internal use only.  */
  ANDROID_LOG_SILENT, /* only for SetMinPriority(); must be last */
} android_LogPriority;

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>

#ifndef LOG_TAG
#define LOG_TAG "NDK_LOG"
#endif

#ifdef DEBUG
    #define LOG_DEBUG
#endif

#ifndef LOG_SILENT
    #ifndef LOG_TO_FILE
        #define LOG(...) ((void)__android_log_print(__VA_ARGS__))
    #else
#if 0    
        static void __stdout_log_print(int level, char* tag, char* fmt, ...) {
            char* lvl = "?";
            switch (level) {
            case ANDROID_LOG_VERBOSE: lvl = "V"; break;
            case ANDROID_LOG_DEBUG: lvl = "D"; break;
            case ANDROID_LOG_INFO: lvl = "I"; break;
            case ANDROID_LOG_WARN: lvl = "W"; break;
            case ANDROID_LOG_ERROR: lvl = "E"; break;
            }
            printf("%s/%s: ", lvl, tag);
            va_list args;
            va_start(args, fmt);
            vprintf(fmt, args);
            va_end(args);
            printf("\n");
            sync();
        }

        #define LOG(level, tag, ...) __stdout_log_print(level, tag, __VA_ARGS__)
#endif
        void log_log(int level, const char *tag, const char *file, int line, const char *fmt, ...);
#if 0
        static void __log_log(int level, char* tag, char* fmt, ...) 
        {
            char* lvl = "?";
            switch (level) {
            case ANDROID_LOG_VERBOSE: lvl = "V"; break;
            case ANDROID_LOG_DEBUG: lvl = "D"; break;
            case ANDROID_LOG_INFO: lvl = "I"; break;
            case ANDROID_LOG_WARN: lvl = "W"; break;
            case ANDROID_LOG_ERROR: lvl = "E"; break;
            }
            printf("%s/%s: ", lvl, tag);
            va_list args;
            va_start(args, fmt);
            vprintf(fmt, args);
            va_end(args);
            printf("\n");
            sync();
        }
#endif
        #define LOG(...) ((void)log_log(level, tag, _VA_ARGS__))
       
    #endif
#else
    #define LOG(...) ((void)0)
#endif

#ifdef LOG_DEBUG
    #define LOGV(...) ((void)LOG(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
    #define LOGD(...) ((void)LOG(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#else
    #define LOGV(...) ((void)0)
    #define LOGD(...) ((void)0)
#endif
#define LOGI(...) ((void)LOG(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)LOG(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)LOG(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

#define CONDITION(cond) (__builtin_expect((cond)!=0, 0))

#define LOGV_IF(cond, ...) ((CONDITION(cond)) ? ((void)LOGV(__VA_ARGS__)) : (void)0)
#define LOGD_IF(cond, ...) ((CONDITION(cond)) ? ((void)LOGD(__VA_ARGS__)) : (void)0)
#define LOGI_IF(cond, ...) ((CONDITION(cond)) ? ((void)LOGI(__VA_ARGS__)) : (void)0)
#define LOGW_IF(cond, ...) ((CONDITION(cond)) ? ((void)LOGW(__VA_ARGS__)) : (void)0)
#define LOGE_IF(cond, ...) ((CONDITION(cond)) ? ((void)LOGE(__VA_ARGS__)) : (void)0)

#ifdef __cplusplus
}
#endif

#endif // _NDK_LOG_H
