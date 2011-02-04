#!/bin/bash
[ -z "${DESTDIR}" ] && DESTDIR=../../../client-trunk/Libraries/ffmpeg-19751/mac

FFARCH=ppc
[ "`uname -m`" = "i386" ] && FFARCH="i386"

for name in libavutil libavcodec libavformat; do
	echo "Updating ${name}.a and ${name}.dylib"
	if [ -f ${name}.dylib ]; then
		lipo -replace ${FFARCH} ${name}/${name}.dylib ${DESTDIR}/${name}.dylib -o ${DESTDIR}/${name}.dylib
	else
		cp ${name}/${name}.dylib ${DESTDIR}
	fi
done
