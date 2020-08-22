#!/bin/bash

TARGET=test_egl_sphere
SOURCES_C=(
	protocol/linux-dmabuf-protocol.c
	egl_ext.c
	gles_ext.c
)
SOURCES=(
	main_chromeos.cpp
	app_sphere.cpp
	util_tex.cpp
	util_file.cpp
	util_misc.cpp
)
CXXFLAGS=(
	-fno-exceptions
	-fno-rtti
	-fstrict-aliasing
	-Wreturn-type
	-Wunused-variable
	-Wunused-value
	-Wno-incompatible-function-pointer-types
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

source cxx_util.sh

if [[ ${HOSTTYPE:0:3} == "arm" ]]; then

	# some distro vendors insist on targeting Thumb2 on ARM - go proper ARM
	CXXFLAGS+=(
		-marm
	)

	ASIMD=`cat /proc/cpuinfo | grep "^Features" | grep -m1 -o -e neon -e asimd`

	if [[ -n $ASIMD ]]; then
		CXXFLAGS+=(
			-mfpu=neon
		)
	fi

	# check for RK3399 based on bigLITTLE config
	if [ `cat /proc/cpuinfo | grep "^CPU part[[:space:]]*: 0xd08" | wc -l` -eq 2 -a \
	     `cat /proc/cpuinfo | grep "^CPU part[[:space:]]*: 0xd03" | wc -l` -eq 4 ]; then
		CXXFLAGS+=(
			-DQUIRK0001_SYSTEM_CRASH_AT_EXIT
		)
	fi

	cxx_uarch_arm

elif [[ ${HOSTTYPE:0:3} == "x86" ]]; then

	CXXFLAGS+=(
		-march=native
		-mtune=native
	)

fi

if [[ $1 == "debug" ]]; then
	CXXFLAGS+=(
		-Wall
		-O0
		-g
		-DDEBUG
	)
else
	CXXFLAGS+=(
		-ffast-math
		-funroll-loops
		-O3
		-DNDEBUG
	)
fi

BUILD_CMD="./clang++.sh -o "${TARGET}" -pipe "${CXXFLAGS[@]}" -x c++ "${SOURCES[@]}" -x c "${SOURCES_C[@]}" "${LFLAGS[@]}
echo $BUILD_CMD
$BUILD_CMD
