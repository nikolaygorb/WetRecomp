# build_optimizations.cmake
#
# The ISA baseline matches the official SDK packages (rexglue-sdk CI builds
# with -march=x86-64-v2): SSE4.2, POPCNT, CMPXCHG16B, LAHF/SAHF on top of the
# SSE4.1 the SDK requires. Not -march=native, so the binary runs on any
# x86-64-v2 CPU, not only the one it was built on.

option(RDRRECOMP_ENABLE_LTO "Enable ThinLTO for Release builds" ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
        add_compile_options(-march=x86-64-v2)
    endif()

    if(RDRRECOMP_ENABLE_LTO AND UNIX AND NOT APPLE)
        include(CheckLinkerFlag)
        check_linker_flag(CXX "-fuse-ld=lld" RDRRECOMP_HAVE_LLD)
        if(RDRRECOMP_HAVE_LLD)
            add_link_options(-fuse-ld=lld)
        else()
            message(WARNING "ld.lld not found: disabling ThinLTO (GNU ld can't link the LTO build). Install lld to enable it.")
            set(RDRRECOMP_ENABLE_LTO OFF)
        endif()
    endif()

    # Release
    if(RDRRECOMP_ENABLE_LTO)
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto=thin -DNDEBUG")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    endif()
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

    # RelWithDebInfo
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")

    # Debug
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_C_FLAGS_DEBUG "-g")
endif()
