#!/bin/bash
DESTDIR=$1
[ -z "${DESTDIR}" ] && DESTDIR=../../../client-trunk/Libraries/ffmpeg-19751/canmore

echo DESTDIR=$DESTDIR
mkdir -p ${DESTDIR}
if [ -e ${DESTDIR}/avcodec.h ]; then
	rm ${DESTDIR}/*
fi

cp -R libavcodec/libavcodec.* $DESTDIR
cp -R libavutil/libavutil.* $DESTDIR
cp -R libavformat/libavformat.* $DESTDIR

