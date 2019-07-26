
SET(GCC_COMPILER_FLAGS "-fpermissive")

#add_definitions(${GCC_COMPILER_FLAGS})
include_directories(../include ./include)

add_library (hook SHARED
  hook_main.cpp
  hook.cpp
  plthook_elf.c
  log.c
  dump.cpp
  io/io.cpp
  io/report.cpp)

# Indicate which libraries to include during the link process.
target_link_libraries (hook ${CMAKE_DL_LIBS})
