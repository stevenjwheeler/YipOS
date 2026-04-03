# Runs on every build to update git_hash.h if the hash changed.
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY "${SRC_DIR}"
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT GIT_HASH)
    set(GIT_HASH "unknown")
endif()

set(NEW_CONTENT "#pragma once\n#define YIP_GIT_HASH \"${GIT_HASH}\"\n")

# Only write if changed (avoids unnecessary rebuilds)
if(EXISTS "${OUT_FILE}")
    file(READ "${OUT_FILE}" OLD_CONTENT)
    if("${OLD_CONTENT}" STREQUAL "${NEW_CONTENT}")
        return()
    endif()
endif()

file(WRITE "${OUT_FILE}" "${NEW_CONTENT}")
