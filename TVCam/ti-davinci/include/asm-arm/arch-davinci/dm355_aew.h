/* *
 * Copyright (C) 2006 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* dm355_aew.h file */
#ifndef DM355_AEW_DRIVER_H
#define DM355_AEW_DRIVER_H

/* Include Files */
#include <linux/ioctl.h>

#ifdef __KERNEL__
#include <asm/semaphore.h>	/* For sempaphores */
#include <asm/fcntl.h>
#endif				/* end of #ifdef __KERNEL__ */

/* Driver Range Constants*/
#define AEW_WINDOW_VERTICAL_COUNT_MIN       1
#define AEW_WINDOW_VERTICAL_COUNT_MAX       128
#define AEW_WINDOW_HORIZONTAL_COUNT_MIN     2
#define AEW_WINDOW_HORIZONTAL_COUNT_MAX     36
#define AEW_WINDOW_SIZE                     18

#define AEW_WIDTH_MIN                       8
#define AEW_WIDTH_MAX                      256

#define AEW_AVELMT_MAX                      1023

#define AEW_HZ_LINEINCR_MIN                 2
#define AEW_HZ_LINEINCR_MAX                 32


#define AEW_VT_LINEINCR_MIN                 2
#define AEW_VT_LINEINCR_MAX                 32

#define AEW_HEIGHT_MIN                      2
#define AEW_HEIGHT_MAX                      256

#define AEW_HZSTART_MIN                      0
#define AEW_HZSTART_MAX                      4095

#define AEW_VTSTART_MIN                      0
#define AEW_VTSTART_MAX                      4095

#define AEW_BLKWINHEIGHT_MIN                2
#define AEW_BLKWINHEIGHT_MAX                256

#define AEW_BLKWINVTSTART_MIN               0
#define AEW_BLKWINVTSTART_MAX               4095


#ifdef __KERNEL__

/* Device Constants*/
#define AEW_NR_DEVS                         1
#define DEVICE_NAME                         "dm355_aew"
#define AEW_MAJOR_NUMBER                    0
#define AEW_IOC_MAXNR                       4
#define AEW_TIMEOUT                         (300 * HZ/1000)
#endif


/* List of ioctls */
#pragma pack(1)
#define AEW_MAGIC_NO    'e'
#define AEW_S_PARAM     _IOWR(AEW_MAGIC_NO,1,struct aew_configuration *)
#define AEW_G_PARAM     _IOWR(AEW_MAGIC_NO,2,struct aew_configuration *)
#define AEW_ENABLE      _IO(AEW_MAGIC_NO,3)
#define AEW_DISABLE     _IO(AEW_MAGIC_NO,4)
#pragma  pack()

/*Enum for device usage*/
typedef enum {
	AEW_NOT_IN_USE = 0,	/* Device is not in use */
	AEW_IN_USE = 1		/* Device in use */
} aew_In_use;

/*Enum for Enable/Disable specific feature*/
typedef enum {
	H3A_AEW_ENABLE = 1,
	H3A_AEW_DISABLE = 0
} aew_alaw_enable;

typedef enum {
	H3A_AEW_CONFIG_NOT_DONE,
	H3A_AEW_CONFIG
} aew_config_flag;


/* Contains the information regarding Window Structure in AEW Engine*/
struct aew_window {
	unsigned int width;	/* Width of the window */
	unsigned int height;	/* Height of the window */
	unsigned int hz_start;	/* Horizontal Start of the window */
	unsigned int vt_start;	/* Vertical Start of the window */
	unsigned int hz_cnt;	/* Horizontal Count */
	unsigned int vt_cnt;	/* Vertical Count */
	unsigned int hz_line_incr;	/* Horizontal Line Increment */
	unsigned int vt_line_incr;	/* Vertical Line Increment */
};

/* Contains the information regarding the AEW Black Window Structure*/
struct aew_black_window {
	unsigned int height;	/* Height of the Black Window */
	unsigned int vt_start;	/* Vertical Start of the black Window */
};

typedef enum _aew_input_src {
	AEW_CCDC = 0,
	AEW_SDRAM = 1
} aew_input_src_t;
/* Contains configuration required for setup of AEW engine*/
struct aew_configuration {
	aew_alaw_enable alaw_enable;	/* A-law status */
	int saturation_limit;	/* Saturation Limit */
	struct aew_window window_config;	/* Window for AEW Engine */
	struct aew_black_window blackwindow_config;	/* Black Window */
};
#ifdef __KERNEL__
/* Contains information about device structure of AEW*/
struct aew_device {
	aew_In_use in_use;	/* Driver usage flag */
	struct aew_configuration *config;	/* Device configuration */
	void *buff_old;		/* Contains latest statistics */
	void *buff_curr;	/* Buffer in which HW will */
	/*fill the statistics */
	/*or HW is already filling */
	/*statistics */
	void *buff_app;		/* Buffer which will be passed */
	/*to user on read call */
	int buffer_filled;	/* Flag indicates statistics */
	/*are available */
	unsigned int size_window;	/* Window size in bytes */
	wait_queue_head_t aew_wait_queue;	/*Wait queue for the driver */
	struct semaphore read_blocked;	/* Semaphore for driver */
	aew_config_flag aew_config;	/*Flag indicates Engine is configured */
};

int aew_hardware_setup(void);
int aew_validate_parameters(void);
#endif				/* End of #ifdef __KERNEL__ */

#endif				/*End of DM355_AEW_H */
