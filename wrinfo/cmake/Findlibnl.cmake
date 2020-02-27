#
# Create a target for libnl or libnl-tiny
#
set(USE_NL_TINY FALSE CACHE BOOL "Whether to use libnl-tiny instead of libnl")
set(nl_found FALSE)
if (USE_NL_TINY)
	find_library(libnl_lib nl-tiny)
	find_path(libnl_include netlink/genl/genl.h
		PATHS /usr/include/libnl-tiny
	)
	if (libnl_lib AND libnl_include)
		set(nl_found TRUE)
	endif()
else()
	find_library(libnl_lib nl-3)
	find_library(libnl_genl_lib nl-genl-3)
	find_path(libnl_include netlink/genl/genl.h
		PATHS /usr/include/libnl3
	)
	if (libnl_lib AND libnl_genl_lib AND libnl_include)
		set(nl_found TRUE)
	endif()
endif()

if (${nl_found})
	add_library(libnl INTERFACE)
	set_target_properties(libnl PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES ${libnl_include}
		INTERFACE_COMPILE_DEFINITIONS _GNU_SOURCE
	)

	target_link_libraries(libnl INTERFACE ${libnl_lib})
	if (NOT USE_NL_TINY)
		target_link_libraries(libnl INTERFACE ${libnl_genl_lib})
	endif()
else()
	message(-- " WARNING: libnl not found!")
endif()
