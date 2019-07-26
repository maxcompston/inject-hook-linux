/* Copyright (c) 2015, Simone 'evilsocket' Margaritelli
   Copyright (c) 2015, Jorrit 'Chainfire' Jongma
   Copyright (c) 2019, Max Compston, Embedded Software Solutions
   See LICENSE file for details */

#include <stdlib.h>
#include <dirent.h>
#include "ndklog.h"
#include "hook.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void hook();

#ifdef __cplusplus
}
#endif

int hooked = 0;
void __attribute__ ((constructor)) hook_main() 
{
//    printf("hook_main, Library loaded\n");

    #ifdef LOG_TO_FILE
        FILE *fp = NULL;
        char szFileName[PATH_MAX];
        char szPath[PATH_MAX];

        // Set up Logging to a File
        if (getcwd(szPath, sizeof(szPath)) == NULL) 
        {
            perror("getcwd() error");
            return;
        }

        // Logging to a File, Set Logging to Quiet
        log_set_quiet(true);

        sprintf(szFileName, "%s/hooked_proc_%d.txt", szPath, getpid());
//        printf("Set up logging to file, szFileName: %s\n", szFileName);
        fp = fopen(szFileName, "w+");

        log_set_fp(fp);
    #endif

     HOOKLOG("******* Library Load in pid[%d] *************", getpid());

    if (hooked) 
        return;

    hooked = 1;
    hook();
}
