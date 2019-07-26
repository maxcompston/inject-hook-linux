
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>

#include "utils.h"

#if defined(__aarch64__)
#define NEXT_ADDRESS	8	// 64 bit Address
#define NEW_LINE		2	// New Line 2 Address
#else
#define NEXT_ADDRESS	4	// 32 bit Address
#define NEW_LINE		4	// New Line 4 Address
#endif

int dump_memory(pid_t pid, uintptr_t startAddress, uintptr_t endAddress)
{
	uintptr_t addr1, addr2;
	uintptr_t data;
	int i = 0;
	int nread = 0;
	char cbuf[16];

	addr1 = startAddress;
	addr2 = endAddress;

	for ( ; addr1 < addr2; addr1 += NEXT_ADDRESS)
	{
		errno = 0;
		if (((data = ptrace(PTRACE_PEEKDATA, pid, (void *)addr1)) == (uintptr_t)-1) && errno)
		{
			printf("PTRACE_PEEKDATA");

			if (ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1)
			{
				printf("PTRACE_DETACH\n");
				return(1);
			}
		}

		// New Line on 4th or 2nd Word
		if ((i++ % NEW_LINE) == 0)
		{
			nread = 0;
			printf("\n");
			printf("0x%08lX  ", (unsigned long)addr1);
		}

		unsigned char *buf = (unsigned char *)&data;

		printf("%02X %02X %02X %02X ", buf[0], buf[1], buf[2], buf[3]);

		cbuf[nread++] = buf[0];
		cbuf[nread++] = buf[1];
		cbuf[nread++] = buf[2];
		cbuf[nread++] = buf[3];

		// Check if 64 bit
		if (NEXT_ADDRESS == 8)
		{
			printf("%02X %02X %02X %02X ", buf[4], buf[5], buf[6], buf[7]);

			cbuf[nread++] = buf[4];
			cbuf[nread++] = buf[5];
			cbuf[nread++] = buf[6];
			cbuf[nread++] = buf[7];			
		}

		if ((i % NEW_LINE) == 0)
		{
	        // Show printable characters
	        printf("  ");
	        for (int j = 0; j < nread; j++)
	        {
	            if (cbuf[j] < 32)
	            	printf(".");
	            else
	            	printf("%c", cbuf[j]);
	        }
		}
	}
	printf("\n");
	return(0);
}

/*
 * findProcessByName()
 *
 * Given the name of a process, try to find its PID by searching through /proc
 * and reading /proc/[pid]/exe until we find a process whose name matches the
 * given process.
 *
 * args:
 * - char* processName: name of the process whose pid to find
 *
 * returns:
 * - a pid_t containing the pid of the process (or -1 if not found)
 *
 */

pid_t findProcessByName(char* processName)
{
	if(processName == NULL)
	{
		return -1;
	}

	struct dirent *procDirs;

	DIR *directory = opendir("/proc/");

	if (directory)
	{
		while ((procDirs = readdir(directory)) != NULL)
		{
			if (procDirs->d_type != DT_DIR)
				continue;

			pid_t pid = atoi(procDirs->d_name);

			int exePathLen = 10 + strlen(procDirs->d_name) + 1;
			char* exePath = malloc(exePathLen * sizeof(char));

			if(exePath == NULL)
			{
				continue;
			}

			sprintf(exePath, "/proc/%s/exe", procDirs->d_name);
			exePath[exePathLen-1] = '\0';

			char* exeBuf = malloc(PATH_MAX * sizeof(char));
			if(exeBuf == NULL)
			{
				free(exePath);
				continue;
			}
			ssize_t len = readlink(exePath, exeBuf, PATH_MAX - 1);

			if(len == -1)
			{
				free(exePath);
				free(exeBuf);
				continue;
			}

			exeBuf[len] = '\0';

			char* exeName = NULL;
			char* exeToken = strtok(exeBuf, "/");
			while(exeToken)
			{
				exeName = exeToken;
				exeToken = strtok(NULL, "/");
			}

			if(strcmp(exeName, processName) == 0)
			{
				free(exePath);
				free(exeBuf);
				closedir(directory);
				return pid;
			}

			free(exePath);
			free(exeBuf);
		}

		closedir(directory);
	}

	return -1;
}

/*
 * freespaceaddr()
 *
 * Search the target process' /proc/pid/maps entry and find an executable
 * region of memory that we can use to run code in.
 *
 * args:
 * - pid_t pid: pid of process to inspect
 *
 * returns:
 * - a long containing the address of an executable region of memory inside the
 *   specified process' address space.
 *
 */

long freespaceaddr(pid_t pid)
{
	FILE *fp;
	char filename[PATH_MAX];
	char line[1024];
	long addr1;
	long addr2;
	char perms[5];
	long num1;
	char mod[6];
	long num2;
	char module[PATH_MAX];

	sprintf(filename, "/proc/%d/maps", pid);
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		exit(1);
	}
	while (fgets(line, 850, fp) != NULL)
	{
		sscanf(line, "%lx-%lx %s %lx %s %ld %s", 
				&addr1, &addr2, perms, &num1, mod, &num2, module);

		// Check if Executable Region
		if (strstr(perms, "x") != NULL)
		{
//			printf("Found Executable Region\n");
			break;
		}
	}
	fclose(fp);
	return addr1;
}

/*
 * getlibcaddr()
 *
 * Gets the base address of libc.so inside a process by reading /proc/pid/maps.
 *
 * args:
 * - pid_t pid: the pid of the process whose libc.so base address we should
 *   find
 * 
 * returns:
 * - a long containing the base address of libc.so inside that process
 *
 */

long getlibcaddr(pid_t pid)
{
	FILE *fp;
	char filename[PATH_MAX];
	char line[1024];
	long addr1;
	long addr2;
	char perms[5];
	long num1;
	char mod[6];
	long num2;
	char module[PATH_MAX];

	sprintf(filename, "/proc/%d/maps", pid);
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		exit(1);
	}
	while (fgets(line, 850, fp) != NULL)
	{
		sscanf(line, "%lx-%lx %s %lx %s %ld %s", 
				&addr1, &addr2, perms, &num1, mod, &num2, module);

		if (strstr(line, "libc-") != NULL)
		{
//			printf("FOUND Libc\n");
			break;
		}
	}
	fclose(fp);
	return addr1;
}

/*
 * checkloaded()
 *
 * Given a process ID and the name of a shared library, check whether that
 * process has loaded the shared library by reading entries in its
 * /proc/[pid]/maps file.
 *
 * args:
 * - pid_t pid: the pid of the process to check
 * - char* libname: the library to search /proc/[pid]/maps for
 *
 * returns:
 * - an int indicating whether or not the library has been loaded into the
 *   process (1 = yes, 0 = no)
 *
 */

int checkloaded(pid_t pid, char* libname)
{
	FILE *fp;
	char filename[PATH_MAX];
	char line[1024];
	long addr1;
	long addr2;
	char perms[5];
	long num1;
	char mod[6];
	long num2;
	char module[PATH_MAX];

	sprintf(filename, "/proc/%d/maps", pid);
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		exit(1);
	}

	// Get the Base File Name
	char *szlibBase = basename(libname);

	while (fgets(line, 850, fp) != NULL)
	{
		sscanf(line, "%lx-%lx %s %lx %s %ld %s", 
				&addr1, &addr2, perms, &num1, mod, &num2, module);
//		printf("addr1: %lx, addr2: %lx, perms: %s\n", addr1, addr2, perms);
//		printf("num1: %lx, mod: %s, num2: %ld, module: %s\n", num1, mod, num2, module);

		// Get the Base Name of the Module
		char *moduleBase = basename(module);

		// Check if Module Found
		if (strcmp(moduleBase, szlibBase) == 0)
		{
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}

/*
 * getFunctionAddress()
 *
 * Find the address of a function within our own loaded copy of libc.so.
 *
 * args:
 * - char* funcName: name of the function whose address we want to find
 *
 * returns:
 * - a long containing the address of that function
 *
 */

long getFunctionAddress(char* funcName)
{
	void* self = dlopen("libc.so.6", RTLD_LAZY);
	void* funcAddr = dlsym(self, funcName);
	return (long)funcAddr;
}

/*
 * findRet()
 *
 * Starting at an address somewhere after the end of a function, search for the
 * "ret" instruction that ends it. We do this by searching for a 0xc3 byte, and
 * assuming that it represents that function's "ret" instruction. This should
 * be a safe assumption. Function addresses are word-aligned, and so there's
 * usually extra space at the end of a function. This space is always padded
 * with "nop"s, so we'll end up just searching through a series of "nop"s
 * before finding our "ret". In other words, it's unlikely that we'll run into
 * a 0xc3 byte that corresponds to anything other than an actual "ret"
 * instruction.
 *
 * Note that this function only applies to x86 and x86_64, and not ARM.
 *
 * args:
 * - void* endAddr: the ending address of the function whose final "ret"
 *   instruction we want to find
 *
 * returns:
 * - an unsigned char* pointing to the address of the final "ret" instruction
 *   of the specified function
 *
 */

unsigned char* findRet(void* endAddr)
{
	unsigned char* retInstAddr = endAddr;
	while(*retInstAddr != INTEL_RET_INSTRUCTION)
	{
		retInstAddr--;
	}
	return retInstAddr;
}

/*
 * usage()
 *
 * Print program usage and exit.
 *
 * args:
 * - char* name: the name of the executable we're running out of
 *
 */

void usage(char* name)
{
	printf("usage: %s [-n process-name] [-p pid] [library-to-inject]\n", name);
}

void dump_buffer(unsigned char buf[], int size)
{
    char cbuf[8];
    int nread = 0;

    // Dump the Buffer
    for (int i = 0; i < size; i += 8)
    {
        printf("0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X ",
            buf[i], buf[i+1], buf[i+2], buf[i+3], buf[i+4], buf[i+5], buf[i+6], buf[i+7]);
 
        cbuf[nread++] = buf[i];
        cbuf[nread++] = buf[i+1];
        cbuf[nread++] = buf[i+2];
        cbuf[nread++] = buf[i+3];
        cbuf[nread++] = buf[i+4];
        cbuf[nread++] = buf[i+5];
        cbuf[nread++] = buf[i+6];
        cbuf[nread++] = buf[i+7];

        if ((i % 8) == 0)  
        {
            // Show printable characters
            printf("  ");
            for (int j = 0; j < nread; j++)
            {
                if (cbuf[j] < 32 || cbuf[j] > 126)
                    printf(". ");
                else
                    printf("%c ", cbuf[j]);
            }
        }
        nread = 0;       
        printf("\n");        
    }
}
