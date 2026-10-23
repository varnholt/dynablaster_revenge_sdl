# Lua 5.4 as a static library, shared by the client (embedded server) and the standalone server.
# Lua is plain C and builds on every platform we target; upstream has no CMake project, so its
# sources are compiled here directly.
include_guard(GLOBAL)

include(FetchContent)

FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG v5.4.7
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(lua)

set(DYNABLASTER_LUA_SOURCES
    lapi.c lauxlib.c lbaselib.c lcode.c lcorolib.c lctype.c ldblib.c ldebug.c ldo.c ldump.c lfunc.c
    lgc.c linit.c liolib.c llex.c lmathlib.c lmem.c loadlib.c lobject.c lopcodes.c loslib.c lparser.c
    lstate.c lstring.c lstrlib.c ltable.c ltablib.c ltm.c lundump.c lutf8lib.c lvm.c lzio.c
)
list(TRANSFORM DYNABLASTER_LUA_SOURCES PREPEND ${lua_SOURCE_DIR}/)

add_library(lua STATIC ${DYNABLASTER_LUA_SOURCES})
set_target_properties(lua PROPERTIES LINKER_LANGUAGE C)
target_include_directories(lua PUBLIC ${lua_SOURCE_DIR})

# newlib doesn't give C++ the long long limits luaconf.h checks for; Lua 5.4 then wants C89 numbers, which
# it only takes via LUA_USE_C89 (long is 64 bits on the Switch's aarch64)
if(NINTENDO_SWITCH)
    target_compile_definitions(lua PUBLIC LUA_USE_C89)
endif()

if(MSVC)
    target_compile_definitions(lua PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
