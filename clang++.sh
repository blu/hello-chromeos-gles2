#!/bin/bash

gnu_spec_machine=`gcc -dumpmachine`
cxx_sys=/usr/local/include/c++/7.3.0
cxx_inc=/usr/local/include/c++/7.3.0/${gnu_spec_machine}
gnuc_lib=/usr/local/lib/gcc/${gnu_spec_machine}/7.3.0
clang++ $@ -cxx-isystem ${cxx_sys} -I ${cxx_inc} -B ${gnuc_lib} -L ${gnuc_lib}
