cmake_minimum_required (VERSION 3.10)
project (injectHook)

#version number
set (injectHook_VERSION_MAJOR 1)
set (injectHook_VERSION_MINOR 0)

#include the subdirectory containing our hook library
add_subdirectory (hook)
include_directories(include)
add_library(hook::library ALIAS hook)
set_target_properties(hook PROPERTIES
        PREFIX "lib"
        LIBRARY_OUTPUT_DIRECTORY ..)

#include the subdirectory containing our inject library
add_subdirectory (inject)
include_directories(include)
add_library(inject::library ALIAS inject)

#indicate the entry point for the executable
IF (CMAKE_CROSSCOMPILING)
    message(STATUS "Cross-compiling building for arm64")
    add_executable (injectHook inject_arm64.c utils.c inject)
ELSE()
    message(STATUS "Not Cross-compiling building for x86-64")

    #indicate the entry point for the executable
    add_executable (injectHook utils.c ptrace.c inject-x86_64.c)

ENDIF()

# Indicate which libraries to include during the link process.
target_link_libraries(injectHook inject ${CMAKE_DL_LIBS})

install (TARGETS injectHook DESTINATION bin)
