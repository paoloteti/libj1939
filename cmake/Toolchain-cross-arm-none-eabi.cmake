SET(CMAKE_C_COMPILER_WORKS 1)
SET(CMAKE_CXX_COMPILER_WORKS 1)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_CROSSCOMPILING 1)

set(TOOLCHAIN_TRIPLE "arm-none-eabi-" CACHE STRING "Triple prefix for arm crosscompiling tools")
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(TOOLCHAIN_SUFFIX ".exe" CACHE STRING "Toolchain executable file extension")
else()
    set(TOOLCHAIN_SUFFIX "" CACHE STRING "Toolchain executable file extension")
endif()

# Search default paths for the GNU ARM Embedded Toolchain
# This will need to be changed if you have it in a different directory.
find_path(TOOLCHAIN_BIN_PATH "${TOOLCHAIN_TRIPLE}gcc${TOOLCHAIN_SUFFIX}"
    PATHS "/usr/bin"                                             # Linux, Mac
    PATHS "C:/Program Files (x86)/GNU Tools ARM Embedded/*/bin"  # Default on Windows
    DOC "Toolchain binaries directory")

if(NOT TOOLCHAIN_BIN_PATH)
    message(FATAL_ERROR "GNU ARM Embedded Toolchain not found. \
                         Please install it and provide its directory as a \
                         TOOLCHAIN_BIN_PATH variable.")
endif()

set(CMAKE_C_COMPILER   "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}gcc${TOOLCHAIN_SUFFIX}")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}g++${TOOLCHAIN_SUFFIX}")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}gcc${TOOLCHAIN_SUFFIX}")
#
# cmake issue: CMAKE_AR seems to require CACHE on some cmake versions
#
set(CMAKE_AR           "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}ar${TOOLCHAIN_SUFFIX}" CACHE FILEPATH "")
set(CMAKE_LINKER       "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}ld${TOOLCHAIN_SUFFIX}")
set(CMAKE_NM           "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}nm${TOOLCHAIN_SUFFIX}")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}objcopy${TOOLCHAIN_SUFFIX}")
set(CMAKE_OBJDUMP      "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}objdump${TOOLCHAIN_SUFFIX}")
set(CMAKE_STRIP        "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}strip${TOOLCHAIN_SUFFIX}")
set(CMAKE_PRINT_SIZE   "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}size${TOOLCHAIN_SUFFIX}")
set(CMAKE_RANLIB       "${TOOLCHAIN_BIN_PATH}/${TOOLCHAIN_TRIPLE}ranlib${TOOLCHAIN_SUFFIX}")

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)    # remove -Wl,-Bdynamic
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS) # remove -rdynamic
set(BUILD_SHARED_LIBRARIES OFF)

set(CMAKE_EXE_LINKER_FLAGS "-static -nostartfiles -Wl,--gc-sections --specs=nosys.specs")
set(CMAKE_C_FLAGS "-fno-strict-aliasing -ffunction-sections -fdata-sections")
set(CMAKE_ASM_FLAGS "-x assembler-with-cpp")
