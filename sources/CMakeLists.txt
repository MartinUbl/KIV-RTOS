CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

PROJECT(kiv_os_rpios CXX C ASM)

SET(CMAKE_EXECUTABLE_SUFFIX ".elf" CACHE STRING "Executable suffix" FORCE)

IF (NOT STANDALONE_BUILD)
    SET(COMPLETE_BUILD ON CACHE INTERNAL "Complete build with standard library and userspace")
ENDIF()

ADD_DEFINITIONS(-g)


# ---------- telemetry build option ----------
option(ENABLE_TELEMETRY "Enable system telemetry collection" ON)
if(ENABLE_TELEMETRY)
    add_definitions(-DENABLE_TELEMETRY)
    # očekávaný počet timer IRQ na 1 sekundu; přizpůsob dle tvého timeru
    # např. 1000 pro 1kHz tick, 100 pro 100Hz tick
    set(TELEMETRY_TICKS_PER_SEC  1000 CACHE STRING "Number of timer ticks per second used by telemetry")
    add_definitions(-DTELEMETRY_TICKS_PER_SEC=${TELEMETRY_TICKS_PER_SEC})
endif()
# ---------- end telemetry build option ----------


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
ADD_EXECUTABLE(kernel ${kernel_src}
        #userspace/telemetry_task/main.cpp
        #kernel/include/fs/drivers/telemetry_fs.h
        #kernel/include/telemetry.h
)

# specifikujeme linker skript - potrebujeme vlastni definici pametoveho layoutu
SET(KERNEL_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/kernel/link.ld")
TARGET_LINK_OPTIONS(kernel PRIVATE -T${KERNEL_LINKER_SCRIPT})

# pridame podslozku - standardni knihovnu
ADD_SUBDIRECTORY(stdlib)

# Přidáme cestu k build adresáři userspace, kde jsou generované hlavičkové soubory src_*.h
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/userspace/build")

# pridame userspace
ADD_SUBDIRECTORY(userspace)


# Explicitní seznam vygenerovaných userspace hlaviček
set(USERSPACE_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/userspace/build/src_init_task.h
        ${CMAKE_CURRENT_SOURCE_DIR}/userspace/build/src_sos_task.h
        ${CMAKE_CURRENT_SOURCE_DIR}/userspace/build/src_oled_task.h
        ${CMAKE_CURRENT_SOURCE_DIR}/userspace/build/src_logger_task.h
        ${CMAKE_CURRENT_SOURCE_DIR}/userspace/build/src_counter_task.h
        ${CMAKE_CURRENT_SOURCE_DIR}/userspace/build/src_tilt_task.h
        ${CMAKE_CURRENT_SOURCE_DIR}/userspace/build/src_telemetry_task.h #nový
)

# Označit jako GENERATED, aby CMake věděl, že vzniknou až během buildu
foreach(h ${USERSPACE_HEADERS})
    set_source_files_properties(${h} PROPERTIES GENERATED TRUE)
endforeach()

# Přidat do kernelu
target_sources(kernel PRIVATE ${USERSPACE_HEADERS})

# Explicitně říct kernelu, aby čekal na userspace targety
add_dependencies(kernel init_task sos_task oled_task logger_task counter_task tilt_task telemetry_task mutex_task)


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


add_custom_target(full_build
        COMMAND ${CMAKE_COMMAND} -E echo "=== [1/3] Building stdlib ==="
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/../build_stdlib.cmake
        COMMAND ${CMAKE_COMMAND} -E echo "=== [2/3] Building userspace ==="
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_SOURCE_DIR}/userspace/build --target init_task sos_task oled_task logger_task counter_task tilt_task telemetry_task mutex_task
        COMMAND ${CMAKE_COMMAND} -E echo "=== [3/3] Rebuilding kernel with userspace symbols ==="
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target kernel --clean-first
)