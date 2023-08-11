#!/bin/sh
./configure --prefix=${PWD}/install --disable-x86asm --disable-everything --enable-decoder=h264,hevc,mjpeg --enable-protocol=concat,file --enable-demuxer=h264,hevc,mjpeg --enable-parser=h264,hevc,mjpeg --enable-gpl
