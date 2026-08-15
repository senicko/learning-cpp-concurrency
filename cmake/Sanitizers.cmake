option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer [cannot combine with ASan]" OFF)

function(cpp_concurrency_sanitizers target)
    set(sanitizer_flags)

    if(ENABLE_TSAN)
        if(ENABLE_ASAN)
            message(FATAL_ERROR "ENABLE_TSAN and ENABLE_ASAN cannot be used together")
        endif()

        list(APPEND sanitizer_flags -fsanitize=thread -fno-omit-frame-pointer)
    else()
        if(ENABLE_ASAN)
            list(APPEND sanitizer_flags -fsanitize=address -fno-omit-frame-pointer)
        endif()

        if(ENABLE_UBSAN)
            list(APPEND sanitizer_flags -fsanitize=undefined -fno-omit-frame-pointer)
        endif()
    endif()

    if(sanitizer_flags)
        target_compile_options(${target} PRIVATE ${sanitizer_flags})
        target_link_options(${target} PRIVATE ${sanitizer_flags})
    endif()
endfunction()
