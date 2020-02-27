cmake_minimum_required(VERSION 3.10)

project(WRInfo
	VERSION 1.0
	DESCRIPTION "Utility to get Wi-Fi Radio Info"
)

# Add source to module path so the Findns3.cmake module will be found
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(libnl REQUIRED)

# Define our executable
add_executable(wrinfo
	main.c
	nlcctx.c		nlcctx.h
	jdattr.c		jdattr.h
	jdump.c			jdump.h
	wifi_interface.c	wifi_interface.h
	wifi_scan.c		wifi_scan.h
	wifi_survey.c		wifi_survey.h
)
target_link_libraries(wrinfo libnl)

# Jdump tests
add_executable(jdump_test
	jdump.c			jdump.h
	jdump_test.c
)
