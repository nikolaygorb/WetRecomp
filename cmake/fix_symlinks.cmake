# fix_symlinks.cmake
#
# Fixes git symlinks that were checked out as regular files on Windows.
# When core.symlinks=false (default on Windows), git creates text files
# containing the symlink target path instead of actual symlinks.
#
# This script detects such files and replaces them with actual file copies
# from the target location.

# Only needed on Windows where symlinks are disabled by default
if(NOT WIN32)
    return()
endif()

set(MSPACK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rexglue-sdk/thirdparty/libmspack")
set(MSPACK_SRC "${MSPACK_DIR}/libmspack/mspack")
set(MSPACK_DST "${MSPACK_DIR}/cabextract/mspack")

if(NOT EXISTS "${MSPACK_SRC}" OR NOT EXISTS "${MSPACK_DST}")
    return()
endif()

# List of files that are symlinks in the libmspack repository
set(MSPACK_SYMLINK_FILES
    cab.h
    cabd.c
    ChangeLog
    lzx.h
    lzxd.c
    macros.h
    mspack.h
    mszip.h
    mszipd.c
    qtm.h
    qtmd.c
    readbits.h
    readhuff.h
    system.c
    system.h
)

foreach(file ${MSPACK_SYMLINK_FILES})
    set(dst_file "${MSPACK_DST}/${file}")
    set(src_file "${MSPACK_SRC}/${file}")

    if(EXISTS "${dst_file}")
        # Read first line to check if it's a symlink (starts with ../)
        file(READ "${dst_file}" file_content LIMIT 100)
        if(file_content MATCHES "^\\.\\./\\.\\./libmspack/mspack/")
            # It's a symlink placeholder - replace with actual file
            if(EXISTS "${src_file}")
                # file(COPY) skips files it thinks are already up to date (by
                # timestamp), so use COPY_FILE which always overwrites.
                file(COPY_FILE "${src_file}" "${dst_file}")
                message(STATUS "Fixed symlink: ${file}")
            else()
                message(WARNING "Cannot fix symlink ${file}: source ${src_file} not found")
            endif()
        endif()
    endif()
endforeach()
