# Find OpenWRT directory locations.
if (NOT DEFINED ENV{STAGING_DIR})
  message(FATAL_ERROR "The STAGING_DIR environment variable has not been set")
endif()
set(owrt_staging_dir $ENV{STAGING_DIR})
# message(-- " OpenWRT staging dir: ${owrt_staging_dir}")
file(GLOB owrt_staging_target_dir "${owrt_staging_dir}/../target-*")
# message(-- " OpenWRT target dir: ${owrt_staging_target_dir}")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER ${owrt_staging_dir}/bin/mips-openwrt-linux-gcc)
set(CMAKE_STAGING_PREFIX ${owrt_staging_target_dir})
set(CMAKE_FIND_ROOT_PATH ${owrt_staging_target_dir} ${owrt_staging_dir})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
