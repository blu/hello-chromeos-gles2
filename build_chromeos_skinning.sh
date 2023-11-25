#!/bin/bash

TARGET=test_egl_skinning
SOURCES_C=(
	linux-dmabuf-protocol.c
	egl_ext.c
	gles_ext.c
)
SOURCES_CXX=(
	main_chromeos.cpp
	app_skinning.cpp
	rendSkeleton.cpp
	util_tex.cpp
	util_file.cpp
	util_misc.cpp
)
CXXFLAGS=(
	-fstrict-aliasing
	-Wreturn-type
	-Wunused-variable
	-Wunused-value
	-DPLATFORM_EGL
	-DPLATFORM_GLES
	-DPLATFORM_GL_OES_vertex_array_object
	-DPLATFORM_GL_KHR_debug
	-I./khronos
	-I./libdrm
	-I./protocol
)
LFLAGS=(
#	-fuse-ld=lld
	/usr/lib/libwayland-client.so
	-lrt
	-ldl
	/usr/lib/libEGL.so
	/usr/lib/libGLESv2.so
)

source cxx_util.sh

if [[ ${HOSTTYPE:0:3} == "arm" || ${HOSTTYPE} == "aarch64" ]]; then

	cxx_uarch_arm

	if [[ ${HOSTTYPE:0:5} == "armv7" ]]; then
		CXXFLAGS+=(
			-mfpu=neon
		)
	fi

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

set -x
gcc -c ${CXXFLAGS[@]} ${SOURCES_C[@]}
g++ -c ${CXXFLAGS[@]} -fno-exceptions -fno-rtti ${SOURCES_CXX[@]}
g++ -o ${TARGET} ${SOURCES_CXX[@]//\.cpp/.o} ${SOURCES_C[@]//\.c/.o} ${LFLAGS[@]}
