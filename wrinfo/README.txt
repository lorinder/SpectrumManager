Compile on Linux
================

1) Install libnl version 3 and corresponding developer packages.  For
   example, on Debian:

   # apt install libnl-3-dev libnl-genl-3-dev

2) The configure and build using cmake the usual way:
   
   $ mkdir build
   $ cd build
   $ cmake ..
   $ cmake --build .


Crosscompile for OpenWRT
========================

1) Install OpenWRT SDK

2) Install libnl-tiny on SDK; for that purpose, read
   https://openwrt.org/docs/guide-developer/using_the_sdk
   (after updating feeds and everything, do:
	make package/feeds/base/libnl-tiny/compile
   This should install libnl-tiny.so into your staging environment.)

3) Setup the PATH and STAGING_DIR environment variables.  Both these are
   required by the OpenWRT SDK in order to be able to compile software
   with it.  If <ROOT> is the root folder of the unpacked OpenWRT, the
   STAGING_FIR environment variable should point to a directory of the
   form

       <ROOT>/staging_dir/toolchain-*/

   where * is dependent on the exact SDK version.  (There's presumably
   only one such toolchain-* subdirectory.)

   PATH should then be updated to include the ${STAGING_DIR}/bin, so as
   to include the toolchain utilities in the search path:

       PATH="${STAGING_DIR}/bin:${PATH}"

   I typically write a small script called setenv.sh for that purpose,
   which I can then source to get the environment variables imported,
   like so:
   
	$ . ./setenv.sh

4) Prepare the build directory with cmake using crosscompilation
   settings.  The cmake/openwrt_toolchain.cmake file is a suitable cmake
   toolchain file that can be used for enabling the cross compilation.
   Also set the USE_NL_TINY flag since the OpenWRT environment uses
   libnl-tiny instead of libnl.

  	$ mkdir build
	$ cd build
	$ cmake \
		-DCMAKE_TOOLCHAIN_FILE=../cmake/openwrt_toolchain.cmake \
		-DUSE_NL_TINY=TRUE \
		..

5) Then build as usual.

	$ cmake --build .
