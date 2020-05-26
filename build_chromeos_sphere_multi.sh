#!/bin/bash

TARGET=test_egl_sphere_multi
SOURCES_C=(
	protocol/linux-dmabuf-protocol.c
	egl_ext.c
	gles_ext.c
)
SOURCES=(
	main_chromeos.cpp
	app_sphere_multi.cpp
	util_tex.cpp
	util_file.cpp
	util_misc.cpp
)
CFLAGS=(
	-fno-exceptions
	-fno-rtti
	-fstrict-aliasing
	-Wreturn-type
	-Wunused-variable
	-Wunused-value
	-Wno-incompatible-function-pointer-types
	-DGL_ES_CONTEXT_VERSION=3
	-DPLATFORM_EGL
	-DPLATFORM_GLES
	-DPLATFORM_GL_OES_vertex_array_object
	-DPLATFORM_GL_KHR_debug
	-I./khronos
	-I./libdrm
	-I./protocol
)
LFLAGS=(
	-fuse-ld=lld
	-lwayland-client
	-lrt
	-ldl
	-lEGL
	-lGLESv2
)

if [[ ${HOSTTYPE:0:3} == "arm" ]]; then

	# some distro vendors insist on targeting Thumb2 on ARM - go proper ARM
	CFLAGS+=(
		-marm
	)

	ASIMD=`cat /proc/cpuinfo | grep "^Features" | grep -m1 -o -e neon -e asimd`

	if [[ -n $ASIMD ]]; then
		CFLAGS+=(
			-mfpu=neon
		)
	fi

	# check for RK3399 based on bigLITTLE config
	if [ `cat /proc/cpuinfo | grep "^CPU part[[:space:]]*: 0xd08" | wc -l` -eq 2 -a \
	     `cat /proc/cpuinfo | grep "^CPU part[[:space:]]*: 0xd03" | wc -l` -eq 4 ]; then
		CFLAGS+=(
			-DQUIRK0001_SYSTEM_CRASH_AT_EXIT
		)
	fi

	# clang may fail auto-detecting the host armv7/armv8 cpu on some setups; collect all part numbers
	UARCH=`cat /proc/cpuinfo | grep "^CPU part" | sed s/^[^[:digit:]]*//`

	# in order of preference, in case of big.LITTLE (armv7 and armv8 lumped together)
	if   [ `echo $UARCH | grep -c 0xd09` -ne 0 ]; then
		CFLAGS+=(
			-march=armv8-a
			-mtune=cortex-a73
		)
	elif [ `echo $UARCH | grep -c 0xd08` -ne 0 ]; then
		CFLAGS+=(
			-march=armv8-a
			-mtune=cortex-a72
		)
	elif [ `echo $UARCH | grep -c 0xd07` -ne 0 ]; then
		CFLAGS+=(
			-march=armv8-a
			-mtune=cortex-a57
		)
	elif [ `echo $UARCH | grep -c 0xd03` -ne 0 ]; then
		CFLAGS+=(
			-march=armv8-a
			-mtune=cortex-a53
		)
	elif [ `echo $UARCH | grep -c 0xc0f` -ne 0 ]; then
		CFLAGS+=(
			-march=armv7-a
			-mtune=cortex-a15
		)
	elif [ `echo $UARCH | grep -c 0xc0e` -ne 0 ]; then
		CFLAGS+=(
			-march=armv7-a
			-mtune=cortex-a17
		)
	elif [ `echo $UARCH | grep -c 0xc09` -ne 0 ]; then
		CFLAGS+=(
			-march=armv7-a
			-mtune=cortex-a9
		)
	elif [ `echo $UARCH | grep -c 0xc08` -ne 0 ]; then
		CFLAGS+=(
			-march=armv7-a
			-mtune=cortex-a8
		)
	elif [ `echo $UARCH | grep -c 0xc07` -ne 0 ]; then
		CFLAGS+=(
			-march=armv7-a
			-mtune=cortex-a7
		)
	fi

elif [[ ${HOSTTYPE:0:3} == "x86" ]]; then

	CFLAGS+=(
		-march=native
		-mtune=native
	)

fi

if [[ $1 == "debug" ]]; then
	CFLAGS+=(
		-Wall
		-O0
		-g
		-DDEBUG
	)
else
	CFLAGS+=(
		-ffast-math
		-funroll-loops
		-O3
		-DNDEBUG
	)
fi

BUILD_CMD="./clang++.sh -o "${TARGET}" -pipe "${CFLAGS[@]}" -x c++ "${SOURCES[@]}" -x c "${SOURCES_C[@]}" "${LFLAGS[@]}
echo $BUILD_CMD
$BUILD_CMD
