# build_optimizations.cmake

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    # Release
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
        set(_rel_flags "-O3 -flto=thin -march=x86-64-v2 -DNDEBUG")
    else()
        set(_rel_flags "-O3 -flto=thin -DNDEBUG")
    endif()
    set(CMAKE_C_FLAGS_RELEASE "${_rel_flags}")
    set(CMAKE_CXX_FLAGS_RELEASE "${_rel_flags}")
    unset(_rel_flags)

    # RelWithDebInfo
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")

    # Debug
    set(CMAKE_C_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
endif()
