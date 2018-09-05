#!/bin/bash

gnu_spec_machine=`gcc -dumpmachine`
gnu_version=`gcc -dumpversion`
gnu_libc_path=`gcc -print-file-name=libc.so`
gnu_libc_real=`realpath ${gnu_libc_path}`
gnu_libc_dir=`dirname ${gnu_libc_real}`
gnu_lib=${gnu_libc_dir}/gcc/${gnu_spec_machine}/${gnu_version}
cxx_sys=/usr/local/include/c++/${gnu_version}
cxx_inc=/usr/local/include/c++/${gnu_version}/${gnu_spec_machine}
clang++ $@ -cxx-isystem ${cxx_sys} -I ${cxx_inc} -B ${gnu_lib} -L ${gnu_lib}
