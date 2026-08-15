cmake_minimum_required(VERSION 4.4)

project(cpp_concurrency LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(AddSubprogram)
include(CompilerWarnings)
include(Sanitizers)

find_package(Threads REQUIRED)

file(GLOB subprogram_dirs CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/packages/*")

foreach(dir IN LISTS subprogram_dirs)
    if(IS_DIRECTORY "${dir}" AND EXISTS "${dir}/CMakeLists.txt")
        add_subdirectory("${dir}")
    endif()
endforeach()

file(REMOVE "${CMAKE_SOURCE_DIR}/compile_commands.json")
file(CREATE_LINK
     "${CMAKE_BINARY_DIR}/compile_commands.json"
     "${CMAKE_SOURCE_DIR}/compile_commands.json"
     SYMBOLIC)
