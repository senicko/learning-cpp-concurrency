# add_subprogram([<name>])
#
# Call from packages/<chapter>/<name>/CMakeLists.txt. Expected layout:
#
#   packages/<chapter>/<name>/
#     CMakeLists.txt          # add_subprogram()  (name defaults to directory)
#     include/<name>/...      # public headers
#     src/main.cpp            # required entry point
#     src/*.cpp               # other sources
#
# Target names must be unique across chapters.
#
# Builds:
#   <name>         executable  (cmake --build build --target <name>)
#   run-<name>     build+run   (cmake --build build --target run-<name>)

function(add_subprogram)
    if(ARGC GREATER 0)
        set(name "${ARGV0}")
    else()
        get_filename_component(name "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
    endif()

    set(prog_dir "${CMAKE_CURRENT_SOURCE_DIR}")
    set(include_dir "${prog_dir}/include")
    set(src_dir "${prog_dir}/src")
    set(main_src "${src_dir}/main.cpp")

    if(NOT EXISTS "${main_src}")
        message(FATAL_ERROR "Subprogram '${name}' requires ${main_src}")
    endif()

    file(GLOB_RECURSE sources CONFIGURE_DEPENDS
         "${src_dir}/*.cpp" "${src_dir}/*.cc" "${src_dir}/*.cxx")

    add_executable(${name} ${sources})

    target_include_directories(${name} PRIVATE "${src_dir}")
    if(EXISTS "${include_dir}")
        target_include_directories(${name} PRIVATE "${include_dir}")
    endif()

    target_link_libraries(${name} PRIVATE Threads::Threads)
    cpp_concurrency_warnings(${name})
    cpp_concurrency_sanitizers(${name})

    add_custom_target(run-${name}
        COMMAND "$<TARGET_FILE:${name}>"
        DEPENDS ${name}
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:${name}>"
        USES_TERMINAL
        COMMENT "Running ${name}")
endfunction()
