#!/bin/bash

gnu_spec_machine=`gcc -dumpmachine`
gnuc_lib=/usr/local/lib/gcc/${gnu_spec_machine}/7.3.0
clang $@ -B ${gnuc_lib} -L ${gnuc_lib}
