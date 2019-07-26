#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>

#include "ndklog.h"
#include "hook.h"

// 64 bit Architecture
#if defined(__aarch64__) || defined(__x86_64__)
#define NEXT_ADDRESS	8	// 64 bit Address
#else
#define NEXT_ADDRESS	4	// 32 bit Address
#endif

char *getAscii(uintptr_t startAddress)
{
    char szChar[5];
    static char szAscii[256];
    char pmem;
    uintptr_t mem;
    uintptr_t address;

    szAscii[0] = '\0';
    szChar[0] = '\0';

    for (address = startAddress; address < startAddress + NEXT_ADDRESS * 8; address++)
    {
        mem = *(uintptr_t *)address;
        pmem = (char)mem;

        if (pmem < 32 || pmem > 126)
        {
            strcpy(szChar, ".");
        }
        else
        {
            sprintf(szChar, "%c", pmem);
        }
        strcat(szAscii, szChar);
    }
    return(szAscii);
}

void dump_address(uintptr_t startAddress, int totalWords)
{
    int nread = 0;
    uintptr_t address;

    HOOKLOG("Start Address: 0x%016lX, total Words: %d, NEXT_ADDRESS: %d", startAddress, totalWords, NEXT_ADDRESS);

    // Dump the Buffer
    for (address = startAddress; address < startAddress + totalWords * NEXT_ADDRESS; address += NEXT_ADDRESS * 8)
    {
        if (NEXT_ADDRESS == 4)
        {
           HOOKLOG("0x%08lX - 0x%08lX 0x%08lX 0x%08lX 0x%08lX 0x%08lX 0x%08lX 0x%08lX 0x%08lX %s",
                (uintptr_t)address, *(uintptr_t *)address, *(uintptr_t *)(address + 4), *(uintptr_t *)(address + 8), *(uintptr_t *)(address + 12),
                *(uintptr_t *)(address + 16), *(uintptr_t *)(address + 20), *(uintptr_t *)(address + 24), *(uintptr_t *)(address + 28), getAscii(address));
        }
        else
        {
           HOOKLOG("0x%016lX - 0x%016lX 0x%016lX 0x%016lX 0x%016lX 0x%016lX 0x%016lX 0x%016lX 0x%016lX %s",
                (uintptr_t)address, *(uintptr_t *)address, *(uintptr_t *)(address + 8), *(uintptr_t *)(address + 16), *(uintptr_t *)(address + 24),
                *(uintptr_t *)(address + 32), *(uintptr_t *)(address + 40), *(uintptr_t *)(address + 48), *(uintptr_t *)(address + 56), getAscii(address));
         }
    }
}
