# libSDL.a libSDL.la libSDL.so libSDL-1.2.so.0 libSDL-1.2.so.0.11.2 libSDLmain.a

LOCAL_PATH := $(call my-dir)

define all-h-files-under
$(patsubst ./%,%, \
  $(shell cd $(LOCAL_PATH) ; \
          find $(1) -name "*.h" -and -not -name ".*") \
 )
endef

MY_SOURCES := \
	./src/SDL.c				\
	./src/SDL_error.c			\
	./src/SDL_fatal.c			\
	./src/audio/SDL_audio.c			\
	./src/audio/SDL_audiocvt.c		\
	./src/audio/SDL_audiodev.c		\
	./src/audio/SDL_mixer.c			\
	./src/audio/SDL_mixer_MMX.c		\
	./src/audio/SDL_mixer_MMX_VC.c		\
	./src/audio/SDL_mixer_m68k.c		\
	./src/audio/SDL_wave.c			\
	./src/cdrom/SDL_cdrom.c			\
	./src/cpuinfo/SDL_cpuinfo.c		\
	./src/events/SDL_active.c		\
	./src/events/SDL_events.c		\
	./src/events/SDL_expose.c		\
	./src/events/SDL_keyboard.c		\
	./src/events/SDL_mouse.c		\
	./src/events/SDL_quit.c			\
	./src/events/SDL_resize.c		\
	./src/file/SDL_rwops.c			\
	./src/stdlib/SDL_getenv.c		\
	./src/stdlib/SDL_iconv.c		\
	./src/stdlib/SDL_malloc.c		\
	./src/stdlib/SDL_qsort.c		\
	./src/stdlib/SDL_stdlib.c		\
	./src/stdlib/SDL_string.c		\
	./src/thread/SDL_thread.c		\
	./src/timer/SDL_timer.c			\
	./src/video/SDL_RLEaccel.c		\
	./src/video/SDL_blit.c			\
	./src/video/SDL_blit_0.c		\
	./src/video/SDL_blit_1.c		\
	./src/video/SDL_blit_A.c		\
	./src/video/SDL_blit_N.c		\
	./src/video/SDL_bmp.c			\
	./src/video/SDL_cursor.c		\
	./src/video/SDL_gamma.c			\
	./src/video/SDL_pixels.c		\
	./src/video/SDL_stretch.c		\
	./src/video/SDL_surface.c		\
	./src/video/SDL_video.c			\
	./src/video/SDL_yuv.c			\
	./src/video/SDL_yuv_mmx.c		\
	./src/video/SDL_yuv_sw.c		\
	./src/joystick/SDL_joystick.c		\
	./src/video/dummy/SDL_nullevents.c	\
	./src/video/dummy/SDL_nullmouse.c	\
	./src/video/dummy/SDL_nullvideo.c	\
	./src/audio/disk/SDL_diskaudio.c	\
	./src/audio/dummy/SDL_dummyaudio.c	\
	./src/loadso/dlopen/SDL_sysloadso.c	\
	./src/audio/dsp/SDL_dspaudio.c		\
	./src/audio/dma/SDL_dmaaudio.c		\
	./src/video/fbcon/SDL_fb3dfx.c		\
	./src/video/fbcon/SDL_fbelo.c		\
	./src/video/fbcon/SDL_fbevents.c	\
	./src/video/fbcon/SDL_fbmatrox.c	\
	./src/video/fbcon/SDL_fbmouse.c		\
	./src/video/fbcon/SDL_fbriva.c		\
	./src/video/fbcon/SDL_fbvideo.c		\
	./src/thread/pthread/SDL_systhread.c	\
	./src/thread/pthread/SDL_syssem.c	\
	./src/thread/pthread/SDL_sysmutex.c	\
	./src/thread/pthread/SDL_syscond.c	\
	./src/joystick/linux/SDL_sysjoystick.c	\
	./src/cdrom/linux/SDL_syscdrom.c	\
	./src/timer/unix/SDL_systimer.c

MY_SOURCES_2 := \
	./src/main/dummy/SDL_dummy_main.c

#----------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO := SDL

LOCAL_COPY_HEADERS := $(call all-h-files-under, include)

include $(BUILD_COPY_HEADERS)

#----------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE    := libSDL
LOCAL_SRC_FILES := $(MY_SOURCES)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include

LOCAL_LDLIBS =  -lm -ldl -lpthread

LOCAL_CFLAGS += -D_REENTRANT -DHAVE_LINUX_VERSION_H -DNO_MALLINFO -DSDL_THREAD_PTHREAD_RECURSIVE_MUTEX -DSDL_TIMER_UNIX -DSDL_CDROM_LINUX -DSDL_JOYSTICK_LINUX -DSDL_AUDIO_DRIVER_OSS -DSDL_THREAD_PTHREAD -D__CANMORE__
#LOCAL_CFLAGS += -D_REENTRANT -DHAVE_LINUX_VERSION_H -DNO_MALLINFO -DSDL_THREAD_PTHREAD_RECURSIVE_MUTEX -DSDL_TIMER_UNIX -DSDL_CDROM_LINUX -DSDL_JOYSTICK_LINUX -DSDL_AUDIO_DRIVER_OSS -DSDL_THREAD_PTHREAD

include $(BUILD_SHARED_LIBRARY)

#----------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE    := libSDL
LOCAL_SRC_FILES := $(MY_SOURCES)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include

LOCAL_LDLIBS =  -lm -ldl -lpthread

LOCAL_CFLAGS += -D_REENTRANT -DHAVE_LINUX_VERSION_H -DNO_MALLINFO -DSDL_THREAD_PTHREAD_RECURSIVE_MUTEX -DSDL_TIMER_UNIX -DSDL_CDROM_LINUX -DSDL_JOYSTICK_LINUX -DSDL_AUDIO_DRIVER_OSS -DSDL_THREAD_PTHREAD -D__CANMORE__
#LOCAL_CFLAGS += -D_REENTRANT -DHAVE_LINUX_VERSION_H -DNO_MALLINFO -DSDL_THREAD_PTHREAD_RECURSIVE_MUTEX -DSDL_TIMER_UNIX -DSDL_CDROM_LINUX -DSDL_JOYSTICK_LINUX -DSDL_AUDIO_DRIVER_OSS -DSDL_THREAD_PTHREAD

include $(BUILD_STATIC_LIBRARY)

#---------------------------------------------------------- 
include $(CLEAR_VARS)

LOCAL_MODULE    := libSDLmain
LOCAL_SRC_FILES := $(MY_SOURCES_2)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include

LOCAL_CFLAGS += -D_GNU_SOURCE=1 -fvisibility=hidden -D_REENTRANT -DSDL_THREAD_PTHREAD_RECURSIVE_MUTEX -DHAVE_LINUX_VERSION_H -DNO_MALLINFO -DSDL_THREAD_PTHREAD -D__CANMORE__
#LOCAL_CFLAGS += -D_GNU_SOURCE=1 -fvisibility=hidden -D_REENTRANT -DSDL_THREAD_PTHREAD_RECURSIVE_MUTEX -DHAVE_LINUX_VERSION_H -DNO_MALLINFO -DSDL_THREAD_PTHREAD

include $(BUILD_STATIC_LIBRARY)

#----------------------------------------------------------



