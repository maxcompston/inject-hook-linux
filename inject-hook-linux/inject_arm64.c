/* -*- indent-tabs-mode: nil -*-
 *
 * injector - Library for injecting a shared library into a Linux process
 *
 * URL: https://github.com/kubo/injector
 *
 * ------------------------------------------------------
 *
 * Copyright (C) 2018 Kubo Takehiro <kubo@jiubao.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 
    Copyright (c) 2015, Simone 'evilsocket' Margaritelli
    Copyright (c) 2015, Jorrit 'Chainfire' Jongma
    Copyright (C) 2015, Tyler Colgan
    Copyright (C) 2018, Kubo Takehiro <kubo@jiubao.org>
    Copyright (c) 2019, Max Compston, Embedded Software Solutions.

    Modifications:
    -- Removed Windows Specific Code
    -- Adapted to use CMake
    -- Added LOGGING
    -- Fixed Buffer Overflow Issue for path variable

 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>

#include "utils.h"
#include "injector.h"

#define INVALID_PID -1

int main(int argc, char **argv)
{
    injector_pid_t pid = INVALID_PID;
    injector_t *injector;
    int opt;
    int i;
    char *endptr;

   printf("injectHook - Copyright (C) 2015 - 2019, Max Compston, Kubo Takehiro, Tyler Colgan, Jorrit 'Chainfire' Jongma, Simone 'evilsocket' Margaritelli\n");

    while ((opt = getopt(argc, argv, "n:p:")) != -1) 
    {
        switch (opt) 
        {
        case 'n':
            pid = findProcessByName(optarg);
            if (pid == INVALID_PID) 
            {
                fprintf(stderr, "counld not find the process: %s\n", optarg);
                return 1;
            }
            printf("targeting process \"%s\" with pid %d\n", optarg, pid);
            break;
        case 'p':
            pid = strtol(optarg, &endptr, 10);
            if (pid <= 0 || *endptr != '\0') 
            {
                fprintf(stderr, "invalid process id number: %s\n", optarg);
                return 1;
            }
            printf("targeting process with pid %d\n", pid);
            break;
        }
    }
    
    if (pid == INVALID_PID) 
    {
        fprintf(stderr, "Usage: %s [-n process-name] [-p pid] library-to-inject ...\n", argv[0]);
        return 1;
    }

    if (injector_attach(&injector, pid) != 0) 
    {
        printf("%s\n", injector_error());
        return 1;
    }
    for (i = optind; i < argc; i++) 
    {
        char *libname = argv[i];
        if (injector_inject(injector, libname) == 0) 
        {
            printf("\"%s\" successfully injected\n", libname);
        } 
        else 
        {
            fprintf(stderr, "could not inject \"%s\"\n", libname);
            fprintf(stderr, "  %s\n", injector_error());
        }
    }
    injector_detach(injector);
    return 0;
}
