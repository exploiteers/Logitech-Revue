#  GPL LICENSE SUMMARY
#
#  Copyright(c) <2009> Logitech. All rights reserved.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of version 2 of the GNU General Public License as
#  published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
#  The full GNU General Public License is included in this distribution
#  in the file called LICENSE.GPL.
#
#  Contact Information:
#  <Ken Erbes, Ken_Erbes@logitech.com>
# 
#  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/user1/wolverine_prj/Canmore-1.1073/i686-linux-elf/lib:
#

include ../../make-include/Makefile.inc
include ./objectfile.inc

FILE=SDL-1.2.13.tar.gz
BUILD_DIR=SDL-1.2.13
DES="SDL 1.2.13 ..."

PKG_CONFIG_PATH=${BUILD_DEST}/usr/local/lib/pkgconfig:${BUILD_DEST}/lib/pkgconfig:
export PKG_CONFIG_PATH

SDL_VIDEODRIVER=directfb
export SDL_VIDEODRIVER

#PREFIX=sdl_installed_stuff
PREFIX=

<default>: all

.PHONY: all
all:
	@echo ">>>Building ${DES}"
	@rm -rf ${BUILD_DIR}
	@tar xfz ${FILE}
	@cd ${BUILD_DIR} && \
		./configure --build=${BUILD} --host=${HOST} --prefix=${BUILD_DEST}/${PREFIX} \
		-disable-video-x11 -disable-video-x11-vm -disable-video-x11-dgamouse -disable-video-x11-xv \
		-disable-video-x11-xinerama -disable-video-x11-xme -disable-video-x11-xrandr -disable-video-x11-dpms \
		-disable-x11-shared -disable-video-opengl \
		-disable-video-directfb \
		&& make all install

install:

install_lean:
	@echo ">>>install_lean ${DES}"
	@for o in ${LIB_FILE}; do \
		echo cp -d ${BUILD_DEST}/${PREFIX}/lib/$${o} ${FSROOT}/lib; \
		cp -d ${BUILD_DEST}/${PREFIX}/lib/$${o} ${FSROOT}/lib; \
	done
	@for o in ${BIN_FILE}; do \
		echo cp -d ${BUILD_DEST}/${PREFIX}/bin/$${o} ${FSROOT}/bin; \
		cp -d ${BUILD_DEST}/${PREFIX}/bin/$${o} ${FSROOT}/bin; \
	done

uninstall:
	@for o in ${LIB_FILE}; do \
		echo rm ${BUILD_DEST}/${PREFIX}/lib/$${o}; \
		echo rm ${FSROOT}/lib/$${o}; \
		rm ${BUILD_DEST}/${PREFIX}/lib/$${o}; \
		rm ${FSROOT}/lib/$${o}; \
	done
	@for o in ${BIN_FILE}; do \
		echo rm ${BUILD_DEST}/${PREFIX}/bin/$${o}; \
		echo rm ${FSROOT}/bin/$${o}; \
		rm ${BUILD_DEST}/${PREFIX}/bin/$${o}; \
		rm ${FSROOT}/bin/$${o}; \
	done

.PHONY: clean
clean:
	@echo ">>>Cleaning ${DES}"
	@rm -rf ${BUILD_DIR}


