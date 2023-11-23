cmake_minimum_required(VERSION 3.10)

set(CMAKE_TOOLCHAIN_FILE $ENV{PS2SDK}/ps2dev.cmake)
set(CMAKE_CXX_STANDARD 20)


project(clearbench)

add_executable(clearbench.elf main.cpp)

target_link_libraries(clearbench.elf
	PRIVATE
	kernel
	graph
	dma
	draw)


set_target_properties(clearbench.elf
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

add_custom_target(run
	COMMAND ps2client execee host:../${CMAKE_PROJECT_NAME}.elf
	DEPENDS ${CMAKE_PROJECT_NAME}.elf
	USES_TERMINAL
	)

add_custom_target(reset
	COMMAND ps2client reset
	)

add_custom_target(emu
	COMMAND $ENV{PCSX2} -elf ${CMAKE_SOURCE_DIR}/${CMAKE_PROJECT_NAME}.elf
	DEPENDS ${CMAKE_PROJECT_NAME}.elf
	USES_TERMINAL
	)
