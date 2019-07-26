/* 
   Copyright (c) 2015, Simone Margaritelli <evilsocket at gmail dot com>
   Copyright (c) 2015, Jorrit 'Chainfire' Jongma
   Copyright (c) 2019, Max Compston, Embedded Software Solutions
   See LICENSE file for details */

#include <map>
#include <sstream>
#include <pthread.h>
#include "ndklog.h"
#include "hook.h"
#include "report.h"

using namespace std;
typedef std::map< int, std::string > fds_map_t;

static pthread_mutex_t __lock = PTHREAD_MUTEX_INITIALIZER;
static fds_map_t __descriptors;

#define LOCK() pthread_mutex_lock(&__lock)
#define UNLOCK() pthread_mutex_unlock(&__lock)

void dump_address(uintptr_t startAddress, int totalWords);

extern uintptr_t find_original( const char *name );

void io_add_descriptor( int fd, const char *name ) 
{
    LOCK();

    __descriptors[fd] = name;

    UNLOCK();
}

void io_del_descriptor( int fd ) 
{
    LOCK();

    fds_map_t::iterator i = __descriptors.find(fd);
    if( i != __descriptors.end() ){
        __descriptors.erase(i);
    }

    UNLOCK();
}

std::string io_resolve_descriptor( int fd ) 
{
    std::string name;

    LOCK();

    fds_map_t::iterator i = __descriptors.find(fd);
    if( i == __descriptors.end() ){
        // attempt to read descriptor from /proc/self/fd
        char descpath[0xFF] = {0},
             descbuff[0xFF] = {0};

        sprintf( descpath, "/proc/self/fd/%d", fd );
        if( readlink( descpath, descbuff, 0xFF ) != -1 ){
            name = descbuff;
        }
        else {
            std::ostringstream s;
            s << "(" << fd << ")";
            name = s.str();
        }
    }
    else {
        name = i->second;
    }

    UNLOCK();

    return name;
}

extern "C" {

DEFINEHOOK( int, open, (const char *pathname, int flags) ) 
{
    int fd = ORIGINAL( open, pathname, flags );

    if( fd != -1 ){
        io_add_descriptor( fd, pathname );
    }

    report_add( "open", "si.i",
        "pathname", pathname,
        "flags", flags,
        fd );

    return fd;
}

DEFINEHOOK( ssize_t, read, (int fd, void *buf, size_t count) ) 
{
    ssize_t r = ORIGINAL( read, fd, buf, count );
/*
    report_add( "read", "spu.i",
        "fd", io_resolve_descriptor(fd).c_str(),
        "buf", buf,
        "count", count,
        r );        
*/
    HOOKLOG("libc, read, Dump");

    // Dump the Contents of the Buffer to the Console
    dump_address((uintptr_t)buf, r / 8);

    return r;
}

DEFINEHOOK( ssize_t, write, (int fd, const void *buf, size_t len, int flags) ) 
{
    ssize_t wrote = ORIGINAL( write, fd, buf, len, flags );

    report_add( "write", "spui.i",
        "fd", io_resolve_descriptor(fd).c_str(),
        "buf", buf,
        "len", len,
        "flags", flags,
        wrote );

    // Dump the Contents of the Buffer to the Console
    dump_address((uintptr_t)buf, len / 8);

    return wrote;
}

DEFINEHOOK( int, close, (int fd) ) 
{
    int c = ORIGINAL( close, fd );

    report_add( "close", "s.i",
        "fd", io_resolve_descriptor(fd).c_str(),
        c );

    io_del_descriptor( fd );

    return c;
}

DEFINEHOOK( int, getc, (FILE *stream) ) 
{
    int c = ORIGINAL( getc, stream );
/*
    report_add( "getc", "s.i",
        "stream", stream,
        c );
*/
    // Check if UnPrintable Character
    if (c < 32 || c > 126)
    {
        HOOKLOG("getc, 0x%02X, %02d, .", c, c);
    }
    else
    {
        HOOKLOG("getc, 0x%02X, %02d, %c", c, c, c);       
    }
    return c;
}

DEFINEHOOK( int, putc, (int c, FILE *stream) ) 
{
    int ret = ORIGINAL( putc, c, stream );
/*
    report_add( "putc", "s.i",
        "stream", stream,
        c );
*/
    // Check if UnPrintable Character
    if (c < 32 || c > 126)
    {
        HOOKLOG("putc, 0x%02X, %02d, .", c, c);
    }
    else
    {
        HOOKLOG("putc, 0x%02X, %02d, %c", c, c, c);       
    }
    return ret;
}

} // extern "C"

DEFINEHOOKPP(void*, read, (char *data, long size)) 
{
    
    void *ptr = ORIGINALPP(data, size);

    HOOKLOG("libstdc++, _ZNSi4readEPcl (read), Dumping");

    // Dump the Contents of the Buffer to the Console
    dump_address((uintptr_t)data, size / 8);
 
    return ptr;
}

extern "C" {

void hook() 
{
    LOGD("LOADED INTO %d", getpid());

    REGISTERHOOK(open);
    REGISTERHOOK(read);
    REGISTERHOOK(write);
    REGISTERHOOK(close);
    REGISTERHOOK(getc);
    REGISTERHOOK(putc);

    REGISTERHOOKPP("_ZNSi4readEPcl", read);

    libhook_hook(1, 1);
}

}