# libmp3lame.a libmp3lame.la libmp3lame.so

LOCAL_PATH := $(call my-dir)

MY_SOURCES_MP3LAME := \
	./libmp3lame/bitstream.c		\
	./libmp3lame/encoder.c			\
	./libmp3lame/fft.c			\
	./libmp3lame/gain_analysis.c		\
	./libmp3lame/id3tag.c			\
        ./libmp3lame/lame.c 			\
	./libmp3lame/mpglib_interface.c		\
	./libmp3lame/newmdct.c			\
	./libmp3lame/presets.c			\
	./libmp3lame/psymodel.c			\
	./libmp3lame/quantize.c			\
	./libmp3lame/quantize_pvt.c		\
	./libmp3lame/reservoir.c		\
	./libmp3lame/set_get.c			\
	./libmp3lame/tables.c			\
	./libmp3lame/takehiro.c			\
	./libmp3lame/util.c			\
	./libmp3lame/vbrquantize.c		\
	./libmp3lame/VbrTag.c			\
	./libmp3lame/version.c


MY_SOURCES_MPGLIB := \
	./mpglib/common.c \
	./mpglib/dct64_i386.c \
	./mpglib/decode_i386.c \
	./mpglib/interface.c \
	./mpglib/layer1.c \
	./mpglib/layer2.c \
	./mpglib/layer3.c \
	./mpglib/tabinit.c

MY_SOURCES_VECTOR := \
	./libmp3lame/vector/xmm_quantize_sub.c

#----------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO := lame

LOCAL_COPY_HEADERS := \
	include/lame.h

include $(BUILD_COPY_HEADERS)

#----------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE    := liblamevectorroutines
LOCAL_SRC_FILES := $(MY_SOURCES_VECTOR)

LOCAL_C_INCLUDES += 		\
	$(LOCAL_PATH)/include 	\
	$(LOCAL_PATH)/libmp3lame

#LOCAL_LDLIBS =

LOCAL_CFLAGS += -DHAVE_CONFIG_H -fomit-frame-pointer -ffast-math -Wall -pipe

include $(BUILD_STATIC_LIBRARY)

#----------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE    := libmpgdecoder
LOCAL_SRC_FILES := $(MY_SOURCES_MPGLIB)

LOCAL_C_INCLUDES += 		\
	$(LOCAL_PATH)/include 	\
	$(LOCAL_PATH)/mpglib	\
	$(LOCAL_PATH)/libmp3lame

#LOCAL_LDLIBS =

LOCAL_CFLAGS += -DHAVE_CONFIG_H -fomit-frame-pointer -ffast-math -Wall -pipe

include $(BUILD_STATIC_LIBRARY)

#----------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE    := libmp3lame
LOCAL_SRC_FILES := $(MY_SOURCES_MP3LAME)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/mpglib

LOCAL_STATIC_LIBRARIES := \
	libmpgdecoder \
	liblamevectorroutines

LOCAL_CFLAGS += -DHAVE_CONFIG_H -fomit-frame-pointer -ffast-math -Wall -pipe 

include $(BUILD_SHARED_LIBRARY)

#----------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE    := libmp3lame
LOCAL_SRC_FILES := $(MY_SOURCES_MP3LAME)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/mpglib

#LOCAL_LDLIBS =

LOCAL_STATIC_LIBRARIES := \
	libmpgdecoder \
	liblamevectorroutines

LOCAL_CFLAGS += -DHAVE_CONFIG_H -fomit-frame-pointer -ffast-math -Wall -pipe 

include $(BUILD_STATIC_LIBRARY)

#----------------------------------------------------------


