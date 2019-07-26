
#add_definitions(${GCC_COMPILER_FLAGS})
include_directories(../include)

add_library (inject STATIC
  elf.c
  injector.c
  ptrace.c
  remote_call.c
  util.c)
