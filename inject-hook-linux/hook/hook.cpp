/* Copyright (c) 2015, Simone 'evilsocket' Margaritelli
   Copyright (c) 2015, Jorrit 'Chainfire' Jongma
   Copyright (c) 2019, Max Compston, Embedded Software Solutions
   See LICENSE file for details 
    
    Modifications:
	-- Adapted this code from Android to Linux
    -- Linking and loading for Linux differs from Android
    -- Hybrid adaptation to Android Interface   
*/

#include <sys/mman.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <dlfcn.h>              // Adds dladdr and dlvsym

#include "plthook.h"
#include "ndklog.h"
#include "hook.h"

typedef struct ld_module {
    uintptr_t address_exec_start;
    uintptr_t address_exec_end;
    uintptr_t address_ro_start = 0;
    uintptr_t address_ro_end = 0;
    uintptr_t address_rw_start = 0;
    uintptr_t address_rw_end = 0;
    std::string name;

    ld_module(uintptr_t start, uintptr_t end, const std::string& name) :
        address_exec_start(start), address_exec_end(end), name(name) {
    }

    void set_data_ro(uintptr_t start, uintptr_t end) 
    {
        this->address_ro_start = start;
        this->address_ro_end = end;
    }

    void set_data_rw(uintptr_t start, uintptr_t end) 
    {
        this->address_rw_start = start;
        this->address_rw_end = end;
    }
} ld_module_t;

typedef std::vector<ld_module_t> ld_modules_t;

typedef struct hook_t {
    const char *name;
    uintptr_t *original;
    uintptr_t hook;

    hook_t(const char* n, uintptr_t* o, uintptr_t h) :
            name(n), original(o), hook(h) {
    }
} hook_t;

typedef std::vector<hook_t> hooks_t;
hooks_t hooks;

static ld_modules_t get_modules() 
{
    ld_modules_t modules;
    char szPath[PATH_MAX];
    char buffer[1024] = { 0 };
    uintptr_t address;
    uintptr_t address_end;
    std::string name;

    FILE *fp = fopen("/proc/self/maps", "rt");
    if (fp == NULL) 
    {
        perror("fopen");
        return modules;
    }

    // get the Executable Path
    readlink("/proc/self/exe", szPath, PATH_MAX);

    std::string strPath = std::string(szPath);

//    HOOKLOG("strPath: %s", strPath.c_str());

    while (fgets(buffer, sizeof(buffer), fp)) 
    {
        address = (uintptr_t) strtoul(buffer, NULL, 16);
        address_end = (uintptr_t) strtoul(strchr(buffer, '-') + 1, NULL, 16);
        if (strchr(buffer, '/') != NULL) 
        {
            name = strchr(buffer, '/');
        } 
        else 
        {
            name = strrchr(buffer, ' ') + 1;
        }
        name.resize(name.size() - 1);

        // Skip Non Module Name
        if (strchr(name.c_str(), '[') != NULL)
        {
//            HOOKLOG("module name: %s", name.c_str());
            continue;
        }
       
        // Check if Base Name Exists
        if (strrchr(buffer, '/') != NULL)
        {
            std::string base;
            base = strrchr(buffer, '/');
            base.resize(base.size());
//            HOOKLOG("base name: %s", base.c_str());
            std::size_t found = base.find("/ld-");

            // Check if Base Name is the Link / Loader, Skip It
            if (found != std::string::npos)
            {
//                HOOKLOG("skipping, base name: %s", base.c_str());
                continue;
            }
        }

        // If Module is the executable, skip this module
        if (!name.compare(strPath))
        {           
            continue;
        }

        if (strstr(buffer, "r-xp")) 
        {
//            HOOKLOG("module[%s] exec [0x%lx]-[0x%lx]", name.c_str(), (long unsigned int)address, (long unsigned int)address_end);
            modules.push_back(ld_module_t(address, address_end, name));
        } 
        else if (strstr(buffer, "r--p")) 
        {
            for (ld_modules_t::iterator i = modules.begin(), ie = modules.end(); i != ie; ++i) 
            {
                if (i->name == name) 
                {
//                    HOOKLOG("module[%s] r/o [0x%lx]-[0x%lx]", name.c_str(), (long unsigned int)address, (long unsigned int)address_end);
                    i->set_data_ro(address, address_end);
                }
            }
        } 
        else if (strstr(buffer, "rw-p")) 
        {
            for (ld_modules_t::iterator i = modules.begin(), ie = modules.end(); i != ie; ++i) 
            {
                if (i->name == name) 
                {
//                    HOOKLOG("module[%s] r/w [0x%lx]-[0x%lx]", name.c_str(), (long unsigned int)address, (long unsigned int)address_end);
                    i->set_data_rw(address, address_end);
                }
            }
        }
    }

    if (fp) 
    {
        fclose(fp);
    }

    return modules;
}

void _libhook_register(const char* name, uintptr_t* original, uintptr_t hook) 
{
    hooks.push_back(hook_t(name, original, hook));
}

void libhook_hook(int patch_module_ro, int patch_module_rw) 
{
    HOOKLOG("LIBRARY LOADED FROM PID %d", getpid());

    plthook_t *plthook;

    const char* libself = NULL;
    Dl_info info;
    if (dladdr((void*) &_libhook_register, &info) != 0) 
    {
        libself = info.dli_fname;
    }
    HOOKLOG("LIBRARY NAME %s", libself == NULL ? "UNKNOWN" : libself);

    // get a list of all loaded modules inside this process.
    ld_modules_t modules = get_modules();

    HOOKLOG("Found %u modules", (unsigned int)modules.size());
    HOOKLOG("Installing %u hooks", (unsigned int)hooks.size());

    uint32_t flags_min = FLAG_NEW_SOINFO | FLAG_LINKED;
    uint32_t flags_max = (FLAG_NEW_SOINFO | FLAG_GNU_HASH | FLAG_LINKER | FLAG_EXE | FLAG_LINKED) + 0x1000 /* future */;

    for (ld_modules_t::iterator i = modules.begin(), ie = modules.end(); i != ie; ++i) 
    {
        // since we know the module is already loaded and mostly
        // we DO NOT want its constructors to be called again,
        // use RTLD_NOLOAD to just get its soinfo address.
//        void* soinfo_base = dlopen(i->name.c_str(), 4, /*RTLD_NOLOAD*/); 

        HOOKLOG("i->name.c_str(): %s", i->name.c_str());

        // Open the Shared Library, Check if Error Returned
        if (plthook_open(&plthook, i->name.c_str()) != 0)
        {
            HOOKLOG("error: %s\n", plthook_error());
        }

        unsigned int pos = 0;
        const char *symbol;
        void **addr;
        int rv;

        while ((rv = plthook_enum(plthook, &pos, &symbol, &addr)) == 0) 
        {
//            HOOKLOG("symbol = %s, address = %p at %p", symbol, *addr, addr);
                
            for (hooks_t::iterator j = hooks.begin(), je = hooks.end(); j != je; ++j) 
            {
                size_t funcnamelen = strlen(j->name);
        
                if (strncmp(symbol, j->name, funcnamelen) == 0) 
                {
                    HOOKLOG("Found symbol = %s, address = %p at %p", symbol, *addr, addr);

                    // Check if symbol Valid
                    if (symbol[funcnamelen] == '\0' || symbol[funcnamelen] == '@')
                    {
                        int prot = get_memory_permission(addr);
                        if (prot == 0) 
                        {                        
                            HOOKLOG("Plt Hook Internal Error");
                        }

                        // Set Memory Protection to Read / Write
                        if (!(prot & PROT_WRITE)) 
                        {
                            if (mprotect(ALIGN_ADDR(addr), page_size, PROT_READ | PROT_WRITE) != 0) 
                            {
                                HOOKLOG("Could not change the process memory permission at %p: %s",
                                        ALIGN_ADDR(addr), strerror(errno));
                            }
                        }

                        // Get the Original Function Pointer
                        *(j->original) = *(uintptr_t *)addr;

                        // Set the Address to the Hooked Function
                        *addr = (void *)j->hook;

                        HOOKLOG("Library: %s, Symbol: %s - Original: %p -> Hook: %p", i->name.c_str(), j->name, *(j->original), j->hook);

                        // If Protction was Write Only Set Protection Back                    
                        if (!(prot & PROT_WRITE)) 
                        {
                            mprotect(ALIGN_ADDR(addr), page_size, prot);
                        }
                    }
                }
            }
        }
        plthook_close(plthook);  
    }
//    HOOKLOG("Done");
}
