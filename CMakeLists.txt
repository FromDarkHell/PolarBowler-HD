cmake_minimum_required(VERSION 4.0)
project(PolarBowler-HD VERSION 0.1.0 LANGUAGES C CXX)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "PolarBowler-HD must be built as 32-bit. Configure with `cmake --preset x86` or `-A Win32`.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Zydis requests CMake 3.9, force the runtime library policy on for it too
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(vendor/HookLib EXCLUDE_FROM_ALL)

# inih
set(INIH_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vendor/inih")

add_library(inih STATIC
    "${INIH_DIRECTORY}/ini.c"
    "${INIH_DIRECTORY}/ini.h"
    "${INIH_DIRECTORY}/cpp/INIReader.cpp"
    "${INIH_DIRECTORY}/cpp/INIReader.h"
)
target_include_directories(inih PUBLIC "${INIH_DIRECTORY}" "${INIH_DIRECTORY}/cpp")
set_target_properties(inih PROPERTIES FOLDER "vendor")

# PolarBowler-HD
file(GLOB_RECURSE POLARBOWLER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
file(GLOB_RECURSE POLARBOWLER_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

add_library(PolarBowler-HD SHARED ${POLARBOWLER_SOURCES} ${POLARBOWLER_HEADERS})

target_include_directories(PolarBowler-HD PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_precompile_headers(PolarBowler-HD PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/pch.h")
target_link_libraries(PolarBowler-HD PRIVATE HookLib inih)

target_compile_definitions(PolarBowler-HD PRIVATE UNICODE _UNICODE)

if(MSVC)
    target_compile_options(PolarBowler-HD PRIVATE /W4 /permissive- /utf-8 /MP)
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${POLARBOWLER_SOURCES} ${POLARBOWLER_HEADERS})
