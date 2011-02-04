#!/bin/bash
# for disabling codecs/techniques

./configure \
--extra-cflags=-I/usr/local/include \
--extra-cflags="-fno-common" \
--extra-ldflags=-L/usr/local/lib \
--enable-memalign-hack \
--target-os=mingw32 \
--enable-shared \
--enable-runtime-cpudetect \
--disable-static \
--enable-libmp3lame
