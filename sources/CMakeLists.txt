CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

PROJECT(kiv_os_rpios CXX C ASM)

SET(CMAKE_EXECUTABLE_SUFFIX ".elf" CACHE STRING "Executable suffix" FORCE)

IF (NOT STANDALONE_BUILD)
    SET(COMPLETE_BUILD ON CACHE INTERNAL "Complete build with standard library and userspace")
ENDIF()

ADD_DEFINITIONS(-g)

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/kernel/include/")

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/stdlib/include/")

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/stdutils/include/")

# budeme vyvijet jen pro RPi0, takze tady proste natvrdo napiseme cesty k hlavickam a pridame define
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/kernel/include/board/rpi0/")
ADD_DEFINITIONS(-DRPI0)

# rekurzivne pridame vsechny zdrojaky z kernel podslozky
FILE(GLOB_RECURSE kernel_src "kernel/*.cpp" "kernel/*.c" "kernel/*.h" "kernel/*.hpp" "kernel/*.s")

# pridame knihovnu utilit (ovladani zarizeni z userspace, apod)
FILE(GLOB_RECURSE stdutils_src "stdutils/*.cpp" "stdutils/*.c" "stdutils/*.h" "stdutils/*.hpp" "stdutils/*.s")

# pridame cil - sestavujeme jen kernel
ADD_EXECUTABLE(kernel ${kernel_src})

# specifikujeme linker skript - potrebujeme vlastni definici pametoveho layoutu
SET(KERNEL_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/kernel/link.ld")
TARGET_LINK_OPTIONS(kernel PRIVATE -T${KERNEL_LINKER_SCRIPT})

# pridame podslozku - standardni knihovnu
ADD_SUBDIRECTORY(stdlib)

# pridame userspace
ADD_SUBDIRECTORY(userspace)

# prilinkujeme standardni knihovnu do kernelu
TARGET_LINK_LIBRARIES(kernel kivrtos_stdlib gcc)

# po dokonceni sestaveni musime jeste zkonvertovat vystupy
# 1) zkompilovany ELF soubor na surovou binarni mapu pro RPi bootloader (kernel.img)
# 2) pro pruzkumne ucely i disassembly vysledne binarky
# 3) SREC dump pro UART bootloader
ADD_CUSTOM_COMMAND(
    TARGET kernel POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ./kernel${CMAKE_EXECUTABLE_SUFFIX} -O binary ./kernel.img
    COMMAND ${CMAKE_OBJDUMP} -l -S -D ./kernel${CMAKE_EXECUTABLE_SUFFIX} > ./kernel.asm
	COMMAND ${CMAKE_OBJCOPY} --srec-forceS3 ./kernel${CMAKE_EXECUTABLE_SUFFIX} -O srec kernel.srec
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
