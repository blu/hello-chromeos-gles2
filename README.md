Description
-----------

This is an experiment in self-hosted ChromeOS development focused on OpenGL ES. It relies on the ChromeOS-native GLESv2/EGL/Wayland stacks and as such should run on any ChromeOS device meeting the prerequisites, irrespective of [Crouton](https://github.com/dnschneid/crouton) conditions.

![](asset/screenshot1.png)

Prerequisites
-------------

* A ChromeOS device in developer mode.
* ChromeOS revision with support for DRM render nodes, and a functioning Wayland server -- both found on ChromeOS revisions with support for Android apps, e.g. R67.
* [Chromebrew](https://github.com/skycocker/chromebrew)

The following Chromebrew packages are required:

* buildessential (a meta package that may or may not include some or all of the follwing packages at the time of reading)
* gcc7
* llvm (optional up to R67, required from R68 onwards due to a limitation in the GNU linker from binutils package; see [Chromebrew ticket #2563](https://github.com/skycocker/chromebrew/issues/2563))
* libwayland
* wayland_protocols
* libpng


How to Build
------------

Before you can build any of the applications you have to run `build_wayland_protocols.sh` in the source directory in order to generate the code for the required auxiliary wayland protocols. It might be useful to repeat that step after every update of Chromebrew package `wayland_protocols`.

Running one of the `build_chromeos_xxx.sh` scripts in the source directiory builds the respective `test_egl_xxx` executable. Currently the compiler is hard-coded to clang++ and the linker to lld, so that building is R68-ready out of the box. To use the GNU linker and g++, partially comment-out the linker line like this:

```
LFLAGS=(
#	-fuse-ld=lld
	-lwayland-client
	-lrt
	-ldl
	-lEGL
	-lGLESv2
	-lpng16
)

```
And change the compiler found in the BUILD_CMD variable to g++.


Warnings & Tips
---------------

Please, take a moment to check the CLI options -- you will need them!
```
usage: ./test_egl_sphere [<option> ...]
options:
        -frames <unsigned_integer>              : set number of frames to run; default is max unsigned int
        -screen <width> <height> <Hz>           : set fullscreen output of specified geometry and refresh
        -bitness <r> <g> <b> <a>                : set EGL config of specified RGBA bitness; default is screen's bitness
        -config_id <positive_integer>           : set EGL config of specified ID; overrides any other config options
        -fsaa <positive_integer>                : set fullscreen antialiasing; default is none
        -grab_frame <unsigned_integer> <file>   : grab the Nth frame to file; index is zero-based
        -drawcalls <positive_integer>           : set number of drawcalls per frame; may be ignored by apps
        -print_egl_configs                      : print available EGL configs
        -app <option> [<arguments>]             : app-specific option
```

Example of CLI usage:
```
$ EGL_LOG_LEVEL=warning taskset 0xc ./test_egl_xxx -frames 2000 -bitness 8 8 8 8 -screen 640 640 60 -app albedo_map asset/texture/unperturbed_normal.raw 16 16 -app alt_anim
```

The above:

* Sets the EGL diagnostics envvar to 'warnings-only'.
* Pins the app to the 3rd and 4th cores.
* Specifies 2000 frames worth of runtime.
* Specifies framebuffer pixel format of RGBA8888 (mandatory for now).
* Specifies framebuffer geometry of 640x640x60Hz (refresh is required yet conveniently ignored).
* Passes two app-specific options via the `-app` arguments.

Please, note that:

* Resizing at runtime, including switching to fullscreen, is not implemented yet.
* Only drawables of 32-bit pixel formats and `EGL_SURFACE_TYPE` of `EGL_PBUFFER_BIT` work currently; any other configs may result in exceptionally slow output or EGL failing to initialize.
* Due to a deficiency in the current EGL/Wayland bridging, the frame loop is quite CPU-intensive. On bigLITTLE ARM machines one might want to pin the app to the big cores.
* If you need to see libEGL diagnostics/debug messages, set the `EGL_LOG_LEVEL` envvar to `debug`.

References
----------

* Jan Newmarch [Programming Wayland Clients](https://jan.newmarch.name/Wayland/)
* Henrique Dante de Almeida [The Hello Wayland Tutorial](https://hdante.wordpress.com/2014/07/08/the-hello-wayland-tutorial/)

![](asset/screenshot2.png)

Cherish your pixels!
