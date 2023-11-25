#!/bin/bash

mkdir -pv protocol
protocols_src=/usr/local/share/wayland-protocols
wayland-scanner public-code < ${protocols_src}/unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml > protocol/linux-dmabuf-protocol.c
wayland-scanner server-header < ${protocols_src}/unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml > protocol/linux-dmabuf-unstable-v1-server-protocol.h
wayland-scanner client-header < ${protocols_src}/unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml > protocol/linux-dmabuf-unstable-v1-client-protocol.h
ln -sf protocol/linux-dmabuf-protocol.c
