mkdir ffmpeg
mkdir ffmpeg/libavcodec
mkdir ffmpeg/libavutil
mkdir ffmpeg/libavformat
mkdir ffmpeg/Windows

cp libavcodec/avcodec.h ffmpeg/libavcodec/
cp libavcodec/dsputil.h ffmpeg/libavcodec/
cp libavcodec/mpegvideo.h ffmpeg/libavcodec/
cp libavcodec/avcodec-52.lib ffmpeg/Windows/
cp libavcodec/avcodec-52.dll ffmpeg/Windows/

cp libavutil/avutil.h ffmpeg/libavutil/
cp libavutil/common.h ffmpeg/libavutil/
cp libavutil/integer.h ffmpeg/libavutil/
cp libavutil/intfloat_readwrite.h ffmpeg/libavutil/
cp libavutil/intreadwrite.h ffmpeg/libavutil/
cp libavutil/log.h ffmpeg/libavutil/
cp libavutil/mathematics.h ffmpeg/libavutil/
cp libavutil/pixfmt.h ffmpeg/libavutil/
cp libavutil/bswap.h ffmpeg/libavutil/
cp libavutil/mem.h ffmpeg/libavutil/
cp libavutil/rational.h ffmpeg/libavutil/
cp libavutil/avutil-49.lib ffmpeg/Windows/
cp libavutil/avutil-49.dll ffmpeg/Windows/

cp libavformat/avformat.h ffmpeg/libavformat/
cp libavformat/avio.h ffmpeg/libavformat/
cp libavformat/rtp.h ffmpeg/libavformat/
cp libavformat/rtsp.h ffmpeg/libavformat/
cp libavformat/rtspcodes.h ffmpeg/libavformat/
cp libavformat/avformat-52.lib ffmpeg/Windows/
cp libavformat/avformat-52.dll ffmpeg/Windows/
