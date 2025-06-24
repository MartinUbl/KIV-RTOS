CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

PROJECT(kiv_os_rpios_userspace CXX C ASM)

SET(CMAKE_EXECUTABLE_SUFFIX ".elf" CACHE STRING "Executable suffix" FORCE)

SET(USERSPACE_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/link.ld")

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../kernel/include/")

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../stdlib/include/")

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../stdutils/include/")

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../kernel/include/board/rpi0/")
ADD_DEFINITIONS(-DRPI0)

SET(ABI_FILES cxxabi.cpp)
SET(CRT_FILES crt0.s crt0.c)

IF(NOT COMPLETE_BUILD)
	SET(STDLIB_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../build/stdlib/libkivrtos_stdlib.a)
ELSE()
	SET(STDLIB_FILES kivrtos_stdlib)
ENDIF()

FUNCTION(ADD_USERSPACE_EXECUTABLE target_name file_list)
	SET(file_list ${ARGV})
	LIST(REMOVE_AT file_list 0)

	ADD_EXECUTABLE(${target_name} ${CRT_FILES} ${ABI_FILES} ${file_list})
	TARGET_LINK_LIBRARIES(${target_name} ${STDLIB_FILES} gcc)
	TARGET_LINK_OPTIONS(${target_name} PRIVATE -T${USERSPACE_LINKER_SCRIPT})

	SET_TARGET_PROPERTIES(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)

	IF (COMPLETE_BUILD)
		ADD_CUSTOM_COMMAND(
			TARGET ${target_name} POST_BUILD
			COMMAND ${CMAKE_OBJCOPY} ${CMAKE_CURRENT_SOURCE_DIR}/build/${target_name}${CMAKE_EXECUTABLE_SUFFIX} -O binary ${CMAKE_CURRENT_SOURCE_DIR}/build/${target_name}.bin
			COMMAND ${CMAKE_OBJDUMP} -l -S -D ${CMAKE_CURRENT_SOURCE_DIR}/build/${target_name}${CMAKE_EXECUTABLE_SUFFIX} > ${CMAKE_CURRENT_SOURCE_DIR}/build/${target_name}.asm
			COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/build && xxd -i ./${target_name}${CMAKE_EXECUTABLE_SUFFIX} > ${CMAKE_CURRENT_SOURCE_DIR}/build/src_${target_name}.h
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
	ELSE()
		ADD_CUSTOM_COMMAND(
			TARGET ${target_name} POST_BUILD
			COMMAND ${CMAKE_OBJCOPY} ${CMAKE_CURRENT_SOURCE_DIR}/build/${target_name}${CMAKE_EXECUTABLE_SUFFIX} -O binary ${CMAKE_CURRENT_SOURCE_DIR}/build/${target_name}.bin
			COMMAND ${CMAKE_OBJDUMP} -l -S -D ${CMAKE_CURRENT_SOURCE_DIR}/build/${target_name}${CMAKE_EXECUTABLE_SUFFIX} > ${CMAKE_CURRENT_SOURCE_DIR}/build/${target_name}.asm
			COMMAND xxd -i ${CMAKE_CURRENT_SOURCE_DIR}/build/${target_name}${CMAKE_EXECUTABLE_SUFFIX} > ${CMAKE_CURRENT_SOURCE_DIR}/build/src_${target_name}.h
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
	ENDIF()

	IF(COMPLETE_BUILD)
	    ADD_DEPENDENCIES(kernel ${target_name})
	ENDIF()
ENDFUNCTION(ADD_USERSPACE_EXECUTABLE)

# init task - systemovy task, musi byt pritomen
ADD_USERSPACE_EXECUTABLE(init_task init_task/main.cpp)

# volitelne tasky pro ruzne ulohy
ADD_USERSPACE_EXECUTABLE(sos_task sos_task/main.cpp)
ADD_USERSPACE_EXECUTABLE(oled_task oled_task/main.cpp)
ADD_USERSPACE_EXECUTABLE(logger_task logger_task/main.cpp)
ADD_USERSPACE_EXECUTABLE(counter_task counter_task/main.cpp)
ADD_USERSPACE_EXECUTABLE(tilt_task tilt_task/main.cpp)
