#!/bin/bash
# ./ffmpeg.canmore.configure.sh -I${LOCAL_OBJ_INCLUDE} -L${LOCAL_SYSTEM_LIB} ${BUILD_DEST} $(CC)
# --enable-cross-compile --arch=i686 --cross-prefix=/usr/local/google/home/jsimmons/git_pc101/canmore-sdk/i686-linux-elf/bin/i686-cm-linux-

if [ -z $1 ] || [ -z $2 ] || [ -z $3 ] || [ -z $4 ]; then

	echo "Error: Please pass the value of the following arguments:"
	echo "--extra-cflags --extra-ldflags --prefix -cc"
	exit 1;
fi

./configure \
--arch=i686 \
--cpu=i686 \
--cc=$4 \
--enable-cross-compile \
--enable-memalign-hack \
--enable-shared \
--disable-ffserver \
--disable-ffplay \
--disable-ipv6 \
--disable-devices \
--disable-swscale-alpha \
--disable-network \
--disable-debug \
--disable-encoders \
--disable-decoders \
--disable-demuxers \
--disable-muxers \
--disable-parsers  \
--disable-static \
--enable-libmp3lame \
--enable-encoder=h263 \
--enable-encoder=flv \
--enable-encoder=h263p \
--enable-encoder=h264 \
--enable-encoder=snow \
--enable-encoder=mpeg1video \
--enable-encoder=mp2 \
--enable-encoder=vc1 \
--enable-encoder=png \
--enable-encoder=rawvideo \
--enable-encoder=mjpeg \
--enable-encoder=pcm_s16le \
--enable-encoder=pcm_s16be \
--enable-encoder=pcm_s8 \
--enable-encoder=pcm_u16le \
--enable-encoder=pcm_u16be \
--enable-encoder=pcm_u8 \
--enable-encoder=flac \
--enable-encoder=libmp3lame \
--enable-encoder=png \
--enable-decoder=h263 \
--enable-decoder=flv \
--enable-decoder=h264 \
--enable-decoder=snow \
--enable-decoder=mpegvideo \
--enable-decoder=mpeg1video \
--enable-decoder=vc1 \
--enable-decoder=png \
--enable-decoder=rawvideo \
--enable-decoder=mjpeg \
--enable-decoder=mp2 \
--enable-decoder=pcm_s16le \
--enable-decoder=pcm_s16be \
--enable-decoder=pcm_s8 \
--enable-decoder=pcm_u16le \
--enable-decoder=pcm_u16be \
--enable-decoder=pcm_u8 \
--enable-decoder=flac \
--enable-decoder=mp3 \
--enable-decoder=png \
--enable-muxer=mpeg1system \
--enable-muxer=mpeg1vcd \
--enable-muxer=mpeg1video \
--enable-muxer=h263 \
--enable-muxer=h264 \
--enable-muxer=mp4 \
--enable-muxer=mov \
--enable-muxer=mp2 \
--enable-muxer=avi \
--enable-muxer=flv \
--enable-muxer=mjpeg \
--enable-muxer=flac \
--enable-muxer=wav \
--enable-muxer=mpegps \
--enable-muxer=pcm_s16le \
--enable-muxer=pcm_s16be \
--enable-muxer=pcm_s8 \
--enable-muxer=pcm_u16le \
--enable-muxer=pcm_u16be \
--enable-muxer=pcm_u8 \
--enable-muxer=mp3 \
--enable-muxer=asf \
--enable-demuxer=h263 \
--enable-demuxer=h264 \
--enable-demuxer=mpegvideo \
--enable-demuxer=mpegps \
--enable-demuxer=avi \
--enable-demuxer=mp4 \
--enable-demuxer=mov \
--enable-demuxer=flac \
--enable-demuxer=wav \
--enable-demuxer=flv \
--enable-demuxer=mjpeg \
--enable-demuxer=vc1 \
--enable-demuxer=pcm_s16le \
--enable-demuxer=pcm_s16be \
--enable-demuxer=pcm_s8 \
--enable-demuxer=pcm_u16le \
--enable-demuxer=pcm_u16be \
--enable-demuxer=pcm_u8 \
--enable-demuxer=mp3 \
--enable-demuxer=asf \
--enable-demuxer=rawvideo \
--enable-parser=h263 \
--enable-parser=vc1 \
--enable-parser=h264 \
--enable-parser=mpegaudio \
--enable-parser=mpegvideo \
--enable-parser=mpeg4video \
--enable-parser=mjpeg \
--extra-cflags=$1 \
--extra-ldflags=$2 \
--prefix=$3


