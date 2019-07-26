# inject-hook-linux
Linux Library Inject and Function Hooking

Source code for injection and hooking on x86-64 and arm64 based linux platforms.  This code was tested on x86-64 and arm64 plaforms.  The arm64 code was tested on a Raspberry Pi running Ubuntu Core Server.

Library injection and function hooking with support for:
- arm64
- x86-64
- Tested on Ubuntu 18.04.2

## License

Please see the (LICENSE) file for the exact details.

In summary:

The code injection and library loading are based upon two Linux projects listed below.  For Ubuntu 18.04.2 the following source code was itegrated from the following projects listed below respectively for the the x86-64 and arm64 hardware platforms.  The linux-inject-master source implemented the x84-64 bit injection.  And the injector-master source implemented the arm64 injection.

The function hooking library are based upon three project listed below.  The first is a linux project that accounts for the differences in elf and linking in linux.  The other two projects where meant for Android to perform function hooking.  They were adapted for useage in linux.

## injectHook and libhook.so 

## About

The program injectHook is a Linux (root) app that can inject a shared library into another process or application.  Additionally, if the user injects the complementary libhook.so, this library, when injected will attempt to hook functions that are registered within the library into the process.  The hook library will attempt to attach hooks to the PLT / GOT addresses in its library.  If a function successfully hooks the addresses will be modified to point to the registered hook functions in libhook.  After this point when a legitimate function is called the function will trampoline to the hook library function.  When the trampoline function is called it can extract / change the payload of the function. After the information is extracted the trampoline function will call the orginal function to satisfy the linkage and return control to the legitimate path.

This program needs to run as the root user.

user@pc $ sudo -s

Also, ptrace restrictions must be disabled to allow the program it inject the library using ptrace.

You can temporarily disable this restriction (and revert to the old behaviour allowing your user to ptrace (gdb) any of their other processes) by doing:
$ echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
To permanently allow it edit /etc/sysctl.d/10-ptrace.conf and change the line:
kernel.yama.ptrace_scope = 1
To read
kernel.yama.ptrace_scope = 0

Below are the command line options for running the injection hook program.

[-n process-name] - option to inject library in process named

[-p pid] - alternate option to inject lbrary into process id

[ library-to-inject ] - shared library to inject into the process

root@ubuntu # ./injectHook [-n process-name] [-p pid] [ library-to-inject ] 

Example below injects libhook library in the example BankSystem program.

root@ubuntu # ./injectHook -n BankSystem ./libhook.so

## Sources: Injecting/hooking

Tyler Colgan - linux-inject-master

https://github.com/gaffe23/linux-inject

Copyright (C) 2018 Kubo Takehiro <kubo@jiubao.org>

injector-master

https://github.com/kubo/injector

Intial hooking is based upon the plthook for Linux.  This code accounts for the differences in hooking linux vs android.  Next, the function hooking and library patching are based upon the Android projects listed below with modfications from Android adapted for Linux.  

Copyright (C) 2018 Kubo Takehiro <kubo@jiubao.org>

plthook

https://github.com/kubo/plthook

Based on Simone *evilsocket* Margaritelli's [ARM Inject](https://github.com/evilsocket/arminject) (&copy; 2015, BSD 3-clause).

Modifications and additions by Jorrit *Chainfire* Jongma (&copy; 2015, BSD 3-clause).

This release is part of the [Spaghetti Sauce Project](https://github.com/Chainfire/spaghetti_sauce_project).

Excerpts from The Android Open Source Project (&copy; 2008, APLv2).

Credit to the above developers for the use of their source code.

Max Compston, Embedded Software Solutions.

## Building this Release

The source code for this program is built using CMake.  

To build the x86-64 program navigate to the build directory run cmake as follows:

$ cmake .. && make

The source code for the arm64 target are cross compiled using gnu arm64 compiler, below.  First in install the cross-compiler on the host.

$ sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

Next, set up the host to cross-compile arm64 creating a toolchain-aarch64-linux-gnu.cmake file in your home directory.  Add the following:

#- this one is important
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR arm64)

#- specify the cross compiler
SET(CMAKE_C_COMPILER   /usr/bin/aarch64-linux-gnu-gcc)
SET(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)

#- where is the target environment
SET(CMAKE_FIND_ROOT_PATH  /user/aarch64-linux-gnu)

#- search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

#- for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

To build the cross-compiled arm64 program navigate to the build directory and run cmake as follows:

$ cmake -DCMAKE_TOOLCHAIN_FILE=~/toolchain-aarch64-linux-gnu.cmake .. && make

