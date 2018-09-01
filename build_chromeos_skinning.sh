#!/bin/bash

TARGET=test_egl_skinning
SOURCE=(
	main_chromeos.cpp
	app_skinning.cpp
	rendSkeleton.cpp
	util_tex.cpp
	util_file.cpp
	util_misc.cpp
	gles_ext.cpp
)
CFLAGS=(
	-std=c++11
	-pipe
	-fno-exceptions
	-fno-rtti
	-fstrict-aliasing
	-Wreturn-type
	-Wunused-variable
	-Wunused-value
	-DPLATFORM_GLES
	-DPLATFORM_GL_OES_vertex_array_object
	-I./khronos
)
LFLAGS=(
	-fuse-ld=lld
	-lwayland-client
	-lrt
	-ldl
	-lEGL
	-lGLESv2
	-lpng16
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
		-DPLATFORM_GL_KHR_debug
	)
else
	CFLAGS+=(
		-ffast-math
		-funroll-loops
		-O3
		-DNDEBUG
	)
fi

BUILD_CMD="../clang++.sh -o "${TARGET}" "${CFLAGS[@]}" "${SOURCE[@]}" "${LFLAGS[@]}
echo $BUILD_CMD
$BUILD_CMD
