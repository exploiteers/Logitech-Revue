# output files...
# static libraries: libavcodec.a libavformat.a libavutil.a
# shared libraries: libavcodec/libavcodec.so libavcodec/libavcodec.so.52 libavformat/libavformat.so libavformat/libavformat.so.52 libavutil/libavutil.so libavutil/libavutil.so.49
# binaries: ffmpeg
#----------------------------------------------------------

ifeq ($(BUILD_GOOGLETV),true)
ifeq ($(TARGET_ARCH),x86)

LOCAL_PATH := $(call my-dir)

define all-h-files-under
$(shell cd $(LOCAL_PATH) ; ls $(1)/*.h)
endef

FFMPEG_CFLAGS := \
    --sysroot $(CANMORE_BUILD_DEST) -gstabs \
    -march=i686 -m32 -std=c99 -O3 -fomit-frame-pointer -DBUILD_GOOGLETV=1 \
    -DHAVE_AV_CONFIG_H -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE \
    -D_ISOC99_SOURCE -D_POSIX_C_SOURCE=200112

#----------------------------------------------------------

include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO := ffmpeg
LOCAL_COPY_HEADERS := config.h
include $(BUILD_COPY_HEADERS)

#----------------------------------------------------------
# libavutil

include $(CLEAR_VARS)

LOCAL_MODULE := libavutil_logi

LOCAL_NO_DEFAULT_COMPILER_FLAGS := true
LOCAL_CFLAGS := $(FFMPEG_CFLAGS)

LOCAL_C_INCLUDES += \
	./

LOCAL_SRC_FILES := \
    libavutil/adler32.c \
    libavutil/aes.c \
    libavutil/avstring.c \
    libavutil/base64.c \
    libavutil/crc.c \
    libavutil/des.c \
    libavutil/fifo.c \
    libavutil/intfloat_readwrite.c \
    libavutil/lfg.c \
    libavutil/lls.c \
    libavutil/log.c \
    libavutil/lzo.c \
    libavutil/mathematics.c \
    libavutil/md5.c \
    libavutil/mem.c \
    libavutil/rational.c \
    libavutil/rc4.c \
    libavutil/sha.c \
    libavutil/tree.c \
    libavutil/utils.c \
    libavutil/random_seed.c

#    libavutil/random.c \

include $(BUILD_SHARED_LIBRARY)

LOCAL_COPY_HEADERS_TO := ffmpeg/libavutil
LOCAL_COPY_HEADERS := $(call all-h-files-under, libavutil)
include $(BUILD_COPY_HEADERS)

LOCAL_COPY_HEADERS_TO := ffmpeg/libavutil/x86
LOCAL_COPY_HEADERS := $(call all-h-files-under, libavutil/x86)
include $(BUILD_COPY_HEADERS)

#----------------------------------------------------------
# libavcodec

include $(CLEAR_VARS)

LOCAL_MODULE := libavcodec_logi

# Do not use GLOBAL_CFLAGS because it builds position independent code.
# FFmpeg's inline assembly needs access to as many registers as possible
# so disable -fPIC and add -fomit-frame-pointer

LOCAL_NO_DEFAULT_COMPILER_FLAGS := true
LOCAL_CFLAGS := $(FFMPEG_CFLAGS)

LOCAL_C_INCLUDES += \
	external/zlib \
	$(TARGET_OUT_HEADERS)

LOCAL_SHARED_LIBRARIES := \
	libz

LOCAL_STATIC_LIBRARIES := \
	libmp3lame \
	libmpgdecoder


#	libcrypto \
#	libcurl_droid \
#	libssl \

# /home/user/workspace/WolverineG/external/lame/398.2/include

# ~/workspace/WolverineG/external/ffmpeg/19751$ find libavcodec -name "*.c"
LOCAL_SRC_FILES := \
	libavcodec/resample.c \
	libavcodec/avs.c \
	libavcodec/nellymoserdec.c \
	libavcodec/qtrleenc.c \
	libavcodec/cga_data.c \
	libavcodec/utils.c \
	libavcodec/ffv1.c \
	libavcodec/audioconvert.c \
	libavcodec/h264dspenc.c \
	libavcodec/mpc.c \
	libavcodec/h264enc.c \
	libavcodec/dnxhddec.c \
	libavcodec/mace.c \
	libavcodec/lcldec.c \
	libavcodec/vp56.c \
	libavcodec/bitstream.c \
	libavcodec/svq1.c \
	libavcodec/rawdec.c \
	libavcodec/pnm_parser.c \
	libavcodec/mjpegenc.c \
	libavcodec/asv1.c \
	libavcodec/dvdsubenc.c \
	libavcodec/elbg.c \
	libavcodec/qtrle.c \
	libavcodec/atrac3.c \
	libavcodec/huffman.c \
	libavcodec/pcx.c \
	libavcodec/dirac_parser.c \
	libavcodec/svq1dec.c \
	libavcodec/fft.c \
	libavcodec/adpcm.c \
	libavcodec/raw.c \
	libavcodec/mpeg4video_parser.c \
	libavcodec/ac3dec_data.c \
	libavcodec/eac3dec_data.c \
	libavcodec/rv30.c \
	libavcodec/dsputil.c \
	libavcodec/intrax8.c \
	libavcodec/mimic.c \
	libavcodec/rv40.c \
	libavcodec/aandcttab.c \
	libavcodec/nellymoserenc.c \
	libavcodec/jpeglsenc.c \
	libavcodec/mpegaudiodec.c \
	libavcodec/vb.c \
	libavcodec/mjpeg.c \
	libavcodec/cavs.c \
	libavcodec/msvideo1.c \
	libavcodec/rv10.c \
	libavcodec/png.c \
	libavcodec/qpeg.c \
	libavcodec/jrevdct.c \
	libavcodec/msrle.c \
	libavcodec/gifdec.c \
	libavcodec/acelp_pitch_delay.c \
	libavcodec/pgssubdec.c \
	libavcodec/roqvideoenc.c \
	libavcodec/v210dec.c \
	libavcodec/svq1enc.c \
	libavcodec/ratecontrol.c \
	libavcodec/ac3dec.c \
	libavcodec/bfi.c \
	libavcodec/truemotion2.c \
	libavcodec/libmp3lame.c \
	libavcodec/h261enc.c \
	libavcodec/h263.c \
	libavcodec/jfdctfst.c \
	libavcodec/tiertexseqv.c \
	libavcodec/ljpegenc.c \
	libavcodec/ac3.c \
	libavcodec/qdm2.c \
	libavcodec/qcelpdec.c \
	libavcodec/vorbis.c \
	libavcodec/faanidct.c \
	libavcodec/h261dec.c \
	libavcodec/dnxhddata.c \
	libavcodec/imgconvert.c \
	libavcodec/vp6.c \
	libavcodec/dpx.c \
	libavcodec/mlp.c \
	libavcodec/eatqi.c \
	libavcodec/shorten.c \
	libavcodec/mpc7.c \
	libavcodec/mpegvideo.c \
	libavcodec/sp5xdec.c \
	libavcodec/roqvideodec.c \
	libavcodec/rv34.c \
	libavcodec/mjpega_dump_header_bsf.c \
	libavcodec/qdrw.c \
	libavcodec/mpegvideo_enc.c \
	libavcodec/cinepak.c \
	libavcodec/alacenc.c \
	libavcodec/cavsdec.c \
	libavcodec/pnmenc.c \
	libavcodec/xsubenc.c \
	libavcodec/ptx.c \
	libavcodec/ra288.c \
	libavcodec/mdec.c \
	libavcodec/vorbis_enc.c \
	libavcodec/parser.c \
	libavcodec/dsicinav.c \
	libavcodec/smc.c \
	libavcodec/v210x.c \
	libavcodec/idcinvideo.c \
	libavcodec/mjpegbdec.c \
	libavcodec/mdct.c \
	libavcodec/wavpack.c \
	libavcodec/libdirac_libschro.c \
	libavcodec/msmpeg4.c \
	libavcodec/huffyuv.c \
	libavcodec/rpza.c \
	libavcodec/xiph.c \
	libavcodec/mpegaudiodata.c \
	libavcodec/jpeglsdec.c \
	libavcodec/remove_extradata_bsf.c \
	libavcodec/rv10enc.c \
	libavcodec/ac3enc.c \
	libavcodec/error_resilience.c \
	libavcodec/rtjpeg.c \
	libavcodec/mpeg12enc.c \
	libavcodec/sonic.c \
	libavcodec/sgienc.c \
	libavcodec/flac.c \
	libavcodec/vorbis_data.c \
	libavcodec/tmv.c \
	libavcodec/cook.c \
	libavcodec/ws-snd1.c \
	libavcodec/flacenc.c \
	libavcodec/targaenc.c \
	libavcodec/wmv2.c \
	libavcodec/allcodecs.c \
	libavcodec/dnxhd_parser.c \
	libavcodec/h264pred.c \
	libavcodec/txd.c \
	libavcodec/dvbsubdec.c \
	libavcodec/movsub_bsf.c \
	libavcodec/sunrast.c \
	libavcodec/aaccoder.c \
	libavcodec/dvdsubdec.c \
	libavcodec/flashsv.c \
	libavcodec/twinvq.c \
	libavcodec/dca_parser.c \
	libavcodec/rawenc.c \
	libavcodec/dca.c \
	libavcodec/imc.c \
	libavcodec/tta.c \
	libavcodec/dctref.c \
	libavcodec/bethsoftvideo.c \
	libavcodec/celp_math.c \
	libavcodec/vp3_parser.c \
	libavcodec/mp3_header_decompress_bsf.c \
	libavcodec/rdft.c \
	libavcodec/cavs_parser.c \
	libavcodec/aacpsy.c \
	libavcodec/dpcm.c \
	libavcodec/s3tc.c \
	libavcodec/vorbis_dec.c \
	libavcodec/noise_bsf.c \
	libavcodec/cabac.c \
	libavcodec/bmp.c \
	libavcodec/simple_idct.c \
	libavcodec/aac_adtstoasc_bsf.c \
	libavcodec/smacker.c \
	libavcodec/gif.c \
	libavcodec/snow.c \
	libavcodec/motion_est.c \
	libavcodec/vqavideo.c \
	libavcodec/mlp_parser.c \
	libavcodec/eaidct.c \
	libavcodec/ac3_parser.c \
	libavcodec/interplayvideo.c \
	libavcodec/rv40dsp.c \
	libavcodec/escape124.c \
	libavcodec/faandct.c \
	libavcodec/g726.c \
	libavcodec/pcxenc.c \
	libavcodec/cscd.c \
	libavcodec/lclenc.c \
	libavcodec/ulti.c \
	libavcodec/golomb.c \
	libavcodec/indeo3.c \
	libavcodec/avpacket.c \
	libavcodec/rv20enc.c \
	libavcodec/mpeg12data.c \
	libavcodec/flacdec.c \
	libavcodec/h264_mp4toannexb_bsf.c \
	libavcodec/zmbvenc.c \
	libavcodec/mpeg12.c \
	libavcodec/wmv2dec.c \
	libavcodec/imgresample.c \
	libavcodec/jpegls.c \
	libavcodec/rv30dsp.c \
	libavcodec/mpegaudio.c \
	libavcodec/aactab.c \
	libavcodec/c93.c \
	libavcodec/dv.c \
	libavcodec/tiff.c \
	libavcodec/aasc.c \
	libavcodec/vmdav.c \
	libavcodec/roqvideo.c \
	libavcodec/4xm.c \
	libavcodec/lsp.c \
	libavcodec/eatgq.c \
	libavcodec/cljr.c \
	libavcodec/mp3_header_compress_bsf.c \
	libavcodec/vp3dsp.c \
	libavcodec/wmv2enc.c \
	libavcodec/pngenc.c \
	libavcodec/aac_ac3_parser.c \
	libavcodec/api-example.c \
	libavcodec/h263_parser.c \
	libavcodec/vp3.c \
	libavcodec/loco.c \
	libavcodec/mmvideo.c \
	libavcodec/dvdsub_parser.c \
	libavcodec/ac3tab.c \
	libavcodec/pngdec.c \
	libavcodec/motionpixels.c \
	libavcodec/aac.c \
	libavcodec/adxdec.c \
	libavcodec/xan.c \
	libavcodec/mpegvideo_parser.c \
	libavcodec/h261_parser.c \
	libavcodec/xsubdec.c \
	libavcodec/wma.c \
	libavcodec/h264.c \
	libavcodec/vcr1.c \
	libavcodec/apedec.c \
	libavcodec/imx_dump_header_bsf.c \
	libavcodec/vc1_parser.c \
	libavcodec/vc1.c \
	libavcodec/dvbsub_parser.c \
	libavcodec/acelp_vectors.c \
	libavcodec/h261.c \
	libavcodec/mlpdec.c \
	libavcodec/mjpegdec.c \
	libavcodec/mpeg4audio.c \
	libavcodec/vc1dsp.c \
	libavcodec/fraps.c \
	libavcodec/wmaenc.c \
	libavcodec/x86/cavsdsp_mmx.c \
	libavcodec/x86/fdct_mmx.c \
	libavcodec/x86/motion_est_mmx.c \
	libavcodec/x86/dsputil_mmx.c \
	libavcodec/x86/dnxhd_mmx.c \
	libavcodec/x86/idct_mmx_xvid.c \
	libavcodec/x86/flacdsp_mmx.c \
	libavcodec/x86/dsputilenc_mmx.c \
	libavcodec/x86/mpegvideo_mmx.c \
	libavcodec/x86/snowdsp_mmx.c \
	libavcodec/x86/cpuid.c \
	libavcodec/x86/vp6dsp_sse2.c \
	libavcodec/x86/idct_sse2_xvid.c \
	libavcodec/x86/idct_mmx.c \
	libavcodec/x86/vp3dsp_mmx.c \
	libavcodec/x86/simple_idct_mmx.c \
	libavcodec/x86/mlpdsp.c \
	libavcodec/x86/vc1dsp_mmx.c \
	libavcodec/x86/vp3dsp_sse2.c \
	libavcodec/x86/vp6dsp_mmx.c \
	libavcodec/bmpenc.c \
	libavcodec/eatgv.c \
	libavcodec/roqaudioenc.c \
	libavcodec/cyuv.c \
	libavcodec/rangecoder.c \
	libavcodec/dvbsub.c \
	libavcodec/flacdata.c \
	libavcodec/nuv.c \
	libavcodec/vp6dsp.c \
	libavcodec/alac.c \
	libavcodec/pcm-mpeg.c \
	libavcodec/vp5.c \
	libavcodec/8svx.c \
	libavcodec/eac3dec.c \
	libavcodec/lzwenc.c \
	libavcodec/h264cavlc.c \
	libavcodec/kmvc.c \
	libavcodec/celp_filters.c \
	libavcodec/opt.c \
	libavcodec/zmbv.c \
	libavcodec/adxenc.c \
	libavcodec/lzw.c \
	libavcodec/pnm.c \
	libavcodec/eamad.c \
	libavcodec/h263dec.c \
	libavcodec/dnxhdenc.c \
	libavcodec/truespeech.c \
	libavcodec/msrledec.c \
	libavcodec/libopencore-amr.c \
	libavcodec/psymodel.c \
	libavcodec/flicvideo.c \
	libavcodec/acelp_filters.c \
	libavcodec/wnv1.c \
	libavcodec/h264idct.c \
	libavcodec/mpegaudiodecheader.c \
	libavcodec/rl2.c \
	libavcodec/targa.c \
	libavcodec/vmnc.c \
	libavcodec/bitstream_filter.c \
	libavcodec/aacenc.c \
	libavcodec/mjpeg_parser.c \
	libavcodec/dump_extradata_bsf.c \
	libavcodec/msmpeg4data.c \
	libavcodec/resample2.c \
	libavcodec/mlpdsp.c \
	libavcodec/sgidec.c \
	libavcodec/h264_parser.c \
	libavcodec/indeo2.c \
	libavcodec/mpegaudioenc.c \
	libavcodec/eval.c \
	libavcodec/aac_parser.c \
	libavcodec/mpegaudio_parser.c \
	libavcodec/faxcompr.c \
	libavcodec/nellymoser.c \
	libavcodec/rle.c \
	libavcodec/lpc.c \
	libavcodec/pcm.c \
	libavcodec/vp56data.c \
	libavcodec/eacmv.c \
	libavcodec/ra144.c \
	libavcodec/pixdesc.c \
	libavcodec/truemotion1.c \
	libavcodec/intrax8dsp.c \
	libavcodec/options.c \
	libavcodec/jfdctint.c \
	libavcodec/wmadec.c \
	libavcodec/vc1dec.c \
	libavcodec/8bps.c \
	libavcodec/iirfilter.c \
	libavcodec/mpc8.c \
	libavcodec/xl.c \
	libavcodec/v210enc.c \
	libavcodec/tiffenc.c \
	libavcodec/vc1data.c

#	libavcodec/motion_est_template.c \
#	libavcodec/tscc.c \
#	libavcodec/libschroedinger.c \
#	libavcodec/libdiracdec.c \
#	libavcodec/libschroedingerenc.c \
#	libavcodec/vaapi_mpeg2.c \
#	libavcodec/libxvidff.c \
#	libavcodec/libx264.c \
#	libavcodec/libspeexdec.c \
#	libavcodec/libfaad.c \
#	libavcodec/flashsvenc.c \
#	libavcodec/w32thread.c \
#	libavcodec/libtheoraenc.c \
#	libavcodec/libfaac.c \
#	libavcodec/vdpau.c \
#	libavcodec/dxa.c \
#	libavcodec/svq3.c \
#	libavcodec/imgconvert_template.c \
#	libavcodec/libopenjpeg.c \
#	libavcodec/mlib/dsputil_mlib.c \
#	libavcodec/ps2/idct_mmi.c \
#	libavcodec/ps2/dsputil_mmi.c \
#	libavcodec/ps2/mpegvideo_mmi.c \
#	libavcodec/libvorbis.c \
#	libavcodec/libdiracenc.c \
#	libavcodec/libschroedingerdec.c \
#	libavcodec/wmaprodec.c \
#	libavcodec/vaapi_mpeg4.c \
#	libavcodec/x86/dsputil_mmx_avg_template.c \
#	libavcodec/x86/mpegvideo_mmx_template.c \
#	libavcodec/x86/dsputil_mmx_rnd_template.c \
#	libavcodec/x86/dsputil_h264_template_mmx.c \
#	libavcodec/x86/dsputil_h264_template_ssse3.c \
#	libavcodec/x86/h264dsp_mmx.c \
#	libavcodec/x86/rv40dsp_mmx.c \
#	libavcodec/x86/dsputil_mmx_qns_template.c \
#	libavcodec/os2thread.c \
#	libavcodec/alpha/motion_est_alpha.c \
#	libavcodec/arm/dsputil_iwmmxt_rnd_template.c \
#	libavcodec/arm/dsputil_iwmmxt.c \
#	libavcodec/arm/mpegvideo_iwmmxt.c \
#	libavcodec/arm/float_arm_vfp.c \
#	libavcodec/arm/dsputil_arm.c \
#	libavcodec/arm/dsputil_neon.c \
#	libavcodec/arm/mpegvideo_arm.c \
#	libavcodec/arm/mpegvideo_armv5te.c \
#	libavcodec/mpegvideo_xvmc.c \
#	libavcodec/beosthread.c \
#	libavcodec/sparc/simple_idct_vis.c \
#	libavcodec/sparc/dsputil_vis.c \
#	libavcodec/sh4/dsputil_align.c \
#	libavcodec/sh4/dsputil_sh4.c \
#	libavcodec/sh4/idct_sh4.c \
#	libavcodec/sh4/qpel.c \
#	libavcodec/g729dec.c \
#	libavcodec/ppc/int_altivec.c \
#	libavcodec/ppc/dsputil_ppc.c \
#	libavcodec/ppc/imgresample_altivec.c \
#	libavcodec/ppc/check_altivec.c \
#	libavcodec/ppc/idct_altivec.c \
#	libavcodec/ppc/float_altivec.c \
#	libavcodec/ppc/mpegvideo_altivec.c \
#	libavcodec/ppc/h264_template_altivec.c \
#	libavcodec/ppc/vc1dsp_altivec.c \
#	libavcodec/ppc/vp3dsp_altivec.c \
#	libavcodec/ppc/h264_altivec.c \
#	libavcodec/ppc/fft_altivec.c \
#	libavcodec/ppc/fdct_altivec.c \
#	libavcodec/ppc/dsputil_altivec.c \
#	libavcodec/ppc/gmc_altivec.c \
#	libavcodec/vaapi_vc1.c \
#	libavcodec/libxvid_rc.c \
#	libavcodec/libgsm.c \
#	libavcodec/vaapi.c \
#	libavcodec/dct-test.c \
#	libavcodec/fft-test.c \
#	libavcodec/motion-test.c \
#	libavcodec/pthread.c \
#	libavcodec/alpha/dsputil_alpha.c \
#	libavcodec/alpha/simple_idct_alpha.c \
#	libavcodec/alpha/mpegvideo_alpha.c \
#	libavcodec/cavsdsp.c \
#	libavcodec/bfin/vp3_bfin.c \
#	libavcodec/bfin/mpegvideo_bfin.c \
#	libavcodec/bfin/dsputil_bfin.c
#	libavcodec/x86/fft_sse.c \
#	libavcodec/x86/fft_3dn2.c \
#	libavcodec/x86/fft_3dn.c

include $(BUILD_SHARED_LIBRARY)

LOCAL_COPY_HEADERS_TO := ffmpeg/libavcodec
LOCAL_COPY_HEADERS := $(call all-h-files-under, libavcodec)
include $(BUILD_COPY_HEADERS)

#----------------------------------------------------------
# libavformat

include $(CLEAR_VARS)

LOCAL_MODULE := libavformat_logi

LOCAL_NO_DEFAULT_COMPILER_FLAGS := true
LOCAL_CFLAGS := $(FFMPEG_CFLAGS)

LOCAL_SHARED_LIBRARIES := libavutil_logi libavcodec_logi

LOCAL_C_INCLUDES += \
	./ \

LOCAL_SRC_FILES := \
    libavformat/4xm.c \
    libavformat/adtsenc.c \
    libavformat/aiff.c \
    libavformat/allformats.c \
    libavformat/amr.c \
    libavformat/apc.c \
    libavformat/ape.c \
    libavformat/asf.c \
    libavformat/asfcrypt.c \
    libavformat/asfdec.c \
    libavformat/asfenc.c \
    libavformat/assdec.c \
    libavformat/assenc.c \
    libavformat/au.c \
    libavformat/audiointerleave.c \
    libavformat/avc.c \
    libavformat/avidec.c \
    libavformat/avienc.c \
    libavformat/avio.c \
    libavformat/aviobuf.c \
    libavformat/avs.c \
    libavformat/bethsoftvid.c \
    libavformat/bfi.c \
    libavformat/c93.c \
    libavformat/crcenc.c \
    libavformat/cutils.c \
    libavformat/daud.c \
    libavformat/dsicin.c \
    libavformat/dv.c \
    libavformat/dvenc.c \
    libavformat/dxa.c \
    libavformat/eacdata.c \
    libavformat/electronicarts.c \
    libavformat/ffmdec.c \
    libavformat/ffmenc.c \
    libavformat/file.c \
    libavformat/flacdec.c \
    libavformat/flacenc.c \
    libavformat/flic.c \
    libavformat/flvdec.c \
    libavformat/flvenc.c \
    libavformat/framecrcenc.c \
    libavformat/gif.c \
    libavformat/gopher.c \
    libavformat/gxf.c \
    libavformat/gxfenc.c \
    libavformat/http.c \
    libavformat/id3v2.c \
    libavformat/idcin.c \
    libavformat/idroq.c \
    libavformat/iff.c \
    libavformat/img2.c \
    libavformat/ipmovie.c \
    libavformat/isom.c \
    libavformat/iss.c \
    libavformat/lmlm4.c \
    libavformat/matroska.c \
    libavformat/matroskadec.c \
    libavformat/matroskaenc.c \
    libavformat/metadata.c \
    libavformat/metadata_compat.c \
    libavformat/mm.c \
    libavformat/mmf.c \
    libavformat/mov.c \
    libavformat/movenc.c \
    libavformat/mp3.c \
    libavformat/mpc.c \
    libavformat/mpc8.c \
    libavformat/mpeg.c \
    libavformat/mpegenc.c \
    libavformat/mpegts.c \
    libavformat/mpegtsenc.c \
    libavformat/mpjpeg.c \
    libavformat/msnwc_tcp.c \
    libavformat/mtv.c \
    libavformat/mvi.c \
    libavformat/mxf.c \
    libavformat/mxfdec.c \
    libavformat/mxfenc.c \
    libavformat/ncdec.c \
    libavformat/nsvdec.c \
    libavformat/nut.c \
    libavformat/nutdec.c \
    libavformat/nutenc.c \
    libavformat/nuv.c \
    libavformat/oggdec.c \
    libavformat/oggenc.c \
    libavformat/oggparseflac.c \
    libavformat/oggparseogm.c \
    libavformat/oggparsespeex.c \
    libavformat/oggparsetheora.c \
    libavformat/oggparsevorbis.c \
    libavformat/oma.c \
    libavformat/options.c \
    libavformat/psxstr.c \
    libavformat/pva.c \
    libavformat/r3d.c \
    libavformat/raw.c \
    libavformat/riff.c \
    libavformat/rl2.c \
    libavformat/rm.c \
    libavformat/rmdec.c \
    libavformat/rmenc.c \
    libavformat/rpl.c \
    libavformat/sdp.c \
    libavformat/segafilm.c \
    libavformat/sierravmd.c \
    libavformat/siff.c \
    libavformat/smacker.c \
    libavformat/sol.c \
    libavformat/swfdec.c \
    libavformat/swfenc.c \
    libavformat/thp.c \
    libavformat/tiertexseq.c \
    libavformat/tta.c \
    libavformat/txd.c \
    libavformat/utils.c \
    libavformat/vc1test.c \
    libavformat/vc1testenc.c \
    libavformat/voc.c \
    libavformat/vocdec.c \
    libavformat/vocenc.c \
    libavformat/wav.c \
    libavformat/wc3movie.c \
    libavformat/westwood.c \
    libavformat/wv.c \
    libavformat/xa.c \
    libavformat/yuv4mpeg.c \
    libavformat/seek.c \
    libavformat/apetag.c \
    libavformat/id3v1.c \
    libavformat/avlanguage.c

#    libavformat/rdt.c \
#    libavformat/rtp.c \
#    libavformat/rtp_aac.c \
#   libavformat/rtp_h264.c \
#    libavformat/rtp_mpv.c \
#    libavformat/rtpdec.c \
#    libavformat/rtpenc.c \
#    libavformat/rtpenc_h264.c \
#    libavformat/rtpproto.c \
#    libavformat/rtsp.c \
#    libavformat/udp.c \
#    libavformat/tcp.c \
#    libavformat/os_support.c \

include $(BUILD_SHARED_LIBRARY)

LOCAL_COPY_HEADERS_TO := ffmpeg/libavformat
LOCAL_COPY_HEADERS := $(call all-h-files-under, libavformat)
include $(BUILD_COPY_HEADERS)

#----------------------------------------------------------

endif  # TARGET_ARCH == x86
endif  # BUILD_GOOGLETV == true



