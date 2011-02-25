#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>		/* Used for struct cdev */
#include <linux/dma-mapping.h>	/* For class_simple_create */
#include <linux/device.h>
#include <asm/arch/vpss.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <asm/arch/dm365_facedetect.h>
#include "dm365_facedetect_hw.h"

#define DRIVERNAME  "DM365FACEDETECT"

struct completion fd_complete;

static struct facedetect_inputdata_t facedetect_default_input_params = {

	(char)0,		/* Enable */
	(char)1,		/* Interrupt Enable */
	(unsigned char *)0x0,	/* Input Addr */
	(unsigned char *)0x0,	/* WorkArea Addr */
	/* Direction : UP */
	(unsigned char)FACE_DETECT_DIRECTION_UP,
	/* Minimum face size 20Pixels */
	(unsigned char)MINIMUM_FACE_SIZE_20_PIXEL,
	(unsigned short)0,	/* Image start X */
	(unsigned short)0,	/* Image start Y */
	(unsigned short)320,	/* Image Width */
	(unsigned short)240,	/* Image Height */
	(unsigned char)5	/* Face detect Threshold value */
};

static struct facedetect_inputdata_t facedetect_current_input_params;

static irqreturn_t facedetect_isr(int irq, void *device_id,
				  struct pt_regs *regs)
{
	complete(&fd_complete);
	regw_if(1 << FINISH_CONTROL, FD_CTRL);

	return IRQ_HANDLED;
}

int facedetect_init_interrupt()
{
	init_completion(&fd_complete);

	return 0;
}

int facedetect_init_hw_setup()
{

	/* Software reset */

	/* Initialize the other control registers */
	regw_if(0x000, FD_DNUM);
	/* Detect direction Up and Minimum face size 20 pixel */
	regw_if(((facedetect_default_input_params.
		  direction) << 2) | ((facedetect_default_input_params.
				       minFaceSize) << 0), FD_DCOND);
	regw_if(facedetect_default_input_params.inputImageStartX, FD_STARTX);
	regw_if(facedetect_default_input_params.inputImageStartY, FD_STARTY);
	regw_if(facedetect_default_input_params.inputImageWidth, FD_SIZEX);
	regw_if(facedetect_default_input_params.inputImageHeight, FD_SIZEY);
	regw_if(facedetect_default_input_params.ThresholdValue, FD_LHIT);

	regw_if((int)facedetect_default_input_params.inputAddr, FDIF_PICADDR);
	regw_if((int)facedetect_default_input_params.workAreaAddr, FDIF_WKADDR);

	facedetect_current_input_params.direction =
	    facedetect_default_input_params.direction;
	facedetect_current_input_params.minFaceSize =
	    facedetect_default_input_params.minFaceSize;
	facedetect_current_input_params.inputImageStartX =
	    facedetect_default_input_params.inputImageStartX;
	facedetect_current_input_params.inputImageStartY =
	    facedetect_default_input_params.inputImageStartY;
	facedetect_current_input_params.inputImageWidth =
	    facedetect_default_input_params.inputImageWidth;
	facedetect_current_input_params.inputImageHeight =
	    facedetect_default_input_params.inputImageHeight;
	facedetect_current_input_params.ThresholdValue =
	    facedetect_default_input_params.ThresholdValue;
	facedetect_current_input_params.inputAddr =
	    facedetect_default_input_params.inputAddr;
	facedetect_current_input_params.workAreaAddr =
	    facedetect_default_input_params.workAreaAddr;
	facedetect_current_input_params.intEnable =
	    facedetect_default_input_params.intEnable;
	facedetect_current_input_params.enable =
	    facedetect_default_input_params.enable;

	/* clear FD interrupt if already enabled */
	vpss_free_irq(VPSS_FDIF_INT, (void *)NULL);

	return 0;
}

int facedetect_set_hw_param(struct facedetect_params_t *config)
{
	if (!config)
		return -EINVAL;

	/* Detect direction Up and Minimum face size 20 pixel */
	facedetect_set_detect_direction(config);
	facedetect_set_minimum_face_size(config);
	regw_if(config->inputdata.inputImageStartX, FD_STARTX);
	regw_if(config->inputdata.inputImageStartY, FD_STARTY);
	regw_if(config->inputdata.inputImageWidth, FD_SIZEX);
	regw_if(config->inputdata.inputImageHeight, FD_SIZEY);
	regw_if(config->inputdata.ThresholdValue, FD_LHIT);

	/* Set the address for the Picture address and Work area */
	facedetect_set_buffer(config);

	facedetect_current_input_params.direction = config->inputdata.direction;
	facedetect_current_input_params.minFaceSize =
	    config->inputdata.minFaceSize;
	facedetect_current_input_params.inputImageStartX =
	    config->inputdata.inputImageStartX;
	facedetect_current_input_params.inputImageStartY =
	    config->inputdata.inputImageStartY;
	facedetect_current_input_params.inputImageWidth =
	    config->inputdata.inputImageWidth;
	facedetect_current_input_params.inputImageHeight =
	    config->inputdata.inputImageHeight;
	facedetect_current_input_params.ThresholdValue =
	    config->inputdata.ThresholdValue;
	facedetect_current_input_params.inputAddr = config->inputdata.inputAddr;
	facedetect_current_input_params.workAreaAddr =
	    config->inputdata.workAreaAddr;
	facedetect_current_input_params.intEnable =
	    facedetect_default_input_params.intEnable;
	facedetect_current_input_params.enable =
	    facedetect_default_input_params.enable;

	return 0;
}

int facedetect_get_hw_param(struct facedetect_params_t *config)
{
	if (!config)
		return -EINVAL;

	config->inputdata.direction = facedetect_current_input_params.direction;
	config->inputdata.minFaceSize =
	    facedetect_current_input_params.minFaceSize;
	config->inputdata.inputImageStartX =
	    facedetect_current_input_params.inputImageStartX;
	config->inputdata.inputImageStartY =
	    facedetect_current_input_params.inputImageStartY;
	config->inputdata.inputImageWidth =
	    facedetect_current_input_params.inputImageWidth;
	config->inputdata.inputImageHeight =
	    facedetect_current_input_params.inputImageHeight;
	config->inputdata.ThresholdValue =
	    facedetect_current_input_params.ThresholdValue;
	config->inputdata.inputAddr = facedetect_current_input_params.inputAddr;
	config->inputdata.workAreaAddr =
	    facedetect_current_input_params.workAreaAddr;
	config->inputdata.intEnable = facedetect_current_input_params.intEnable;
	config->inputdata.enable = facedetect_current_input_params.enable;

	return 0;
}

int facedetect_set_detect_direction(struct facedetect_params_t *config)
{
	unsigned int read_val = 0x000;
	unsigned int write_val = 0x000;

	read_val = regr_if(FD_DCOND);
	read_val = read_val & 0x003;
	switch (config->inputdata.direction) {
	case FACE_DETECT_DIRECTION_UP:
		write_val = read_val | 0x000;
		break;

	case FACE_DETECT_DIRECTION_RIGHT:
		write_val = read_val | 0x004;
		break;

	case FACE_DETECT_DIRECTION_LEFT:
		write_val = read_val | 0x008;
		break;
	}
	regw_if(write_val, FD_DCOND);

	return 0;
}

int facedetect_set_minimum_face_size(struct facedetect_params_t *config)
{
	unsigned int read_val = 0x000;
	unsigned int write_val = 0x000;

	read_val = regr_if(FD_DCOND);
	read_val = read_val & 0xC;

	switch (config->inputdata.minFaceSize) {
	case MINIMUM_FACE_SIZE_20_PIXEL:
		write_val = read_val | 0x0;
		break;

	case MINIMUM_FACE_SIZE_25_PIXEL:
		write_val = read_val | 0x1;
		break;

	case MINIMUM_FACE_SIZE_32_PIXEL:
		write_val = read_val | 0x2;
		break;

	case MINIMUM_FACE_SIZE_40_PIXEL:
		write_val = read_val | 0x3;
		break;
	}

	regw_if(write_val, FD_DCOND);

	return 0;
}

int facedetect_get_num_faces(struct facedetect_params_t *config)
{
	config->outputdata.faceCount = regr_if(FD_DNUM);

	return 0;
}

int facedetect_get_all_face_detect_positions(struct facedetect_params_t *config)
{
	volatile unsigned char loop = 0;
	volatile unsigned short confidence_size = 0x000;

	for (loop = 0; loop < config->outputdata.faceCount; loop++) {
		config->outputdata.face_position[loop].resultX =
		    regr_if(FD_CENTERX1 + (loop * OUTPUT_RESULT_OFFSET));
		config->outputdata.face_position[loop].resultY =
		    regr_if(FD_CENTERY1 + (loop * OUTPUT_RESULT_OFFSET));
		confidence_size =
		    regr_if(FD_CONFSIZE1 + (loop * OUTPUT_RESULT_OFFSET));
		config->outputdata.face_position[loop].resultConfidenceLevel =
		    (FACE_DETECT_CONFIDENCE & confidence_size) >> 8;
		config->outputdata.face_position[loop].resultSize =
		    (FACE_DETECT_SIZE & confidence_size);
		config->outputdata.face_position[loop].resultAngle =
		    (regr_if(FD_ANGLE1 + (loop * OUTPUT_RESULT_OFFSET))) &
		    0x1FF;
	}

	return 0;

}

int facedetect_set_buffer(struct facedetect_params_t *config)
{
	if ((((int)config->inputdata.inputAddr % 32) != 0)
	    || (((int)config->inputdata.workAreaAddr % 32) != 0))
		return -EINVAL;

	/* Set the address for the Picture address */
	regw_if((int)config->inputdata.inputAddr, FDIF_PICADDR);

	/* Set the address for the Work area */
	regw_if((int)config->inputdata.workAreaAddr, FDIF_WKADDR);

	facedetect_current_input_params.inputAddr = config->inputdata.inputAddr;
	facedetect_current_input_params.workAreaAddr =
	    config->inputdata.workAreaAddr;

	return 0;
}

int facedetect_execute(struct facedetect_params_t *config)
{
	volatile int cnt = 0;

	/* Enable Face Detection */
	facedetect_int_enable(1);

	regw_if(1 << SRST_CONTROL, FD_CTRL);

	/* Delay between SW Reset and Run. Need Tuning */
	for (cnt = 0; cnt < 10000; cnt++)
		;

	regw_if(1 << RUN_CONTROL, FD_CTRL);

	facedetect_int_wait(1);
	facedetect_int_clear(1);

	/* Disable interrupt again */
	facedetect_int_enable(0);

	facedetect_get_num_faces(config);
	facedetect_get_all_face_detect_positions(config);

	return 0;
}

int facedetect_int_enable(char enable)
{
	regw_if(enable, FDIF_INTEN);

	if (enable == 1) {
		if (vpss_request_irq
		    (VPSS_FDIF_INT, facedetect_isr, SA_INTERRUPT, DRIVERNAME,
		     (void *)NULL)) {
			return -1;
		}
	} else if (enable == 0)
		vpss_free_irq(VPSS_FDIF_INT, (void *)NULL);

	facedetect_current_input_params.intEnable = enable;
	return 0;
}

int facedetect_int_clear(char stat_clear)
{
	regw_if(0x000, FDIF_INTEN);
	INIT_COMPLETION(fd_complete);
	vpss_free_irq(VPSS_FDIF_INT, (void *)NULL);
	return 0;
}

int facedetect_int_wait(char stat_wait)
{
	/* Waiting for LDC to complete */
	wait_for_completion_interruptible(&fd_complete);
	INIT_COMPLETION(fd_complete);
	return 0;
}

int facedetect_isbusy(char *status)
{
	volatile unsigned int read_val = 0x000;

	read_val = regr_if(FD_CTRL);

	/* Check the Finish control bit */
	if (0x001 << FINISH_CONTROL & read_val) {
		*status = FD_STATUS_FREE;
		regw_if(1 << FINISH_CONTROL, FD_CTRL);
	} else
		*status = FD_STATUS_BUSY;

	return 0;
}

int facedetect_set_start_x(struct facedetect_params_t *config)
{
	/* Value should be 0<=STARTX<=160 */
	regw_if(config->inputdata.inputImageStartX, FD_STARTX);

	return 0;
}

int facedetect_set_start_y(struct facedetect_params_t *config)
{
	/* Value should be 0<=STARTY<=120 */
	regw_if(config->inputdata.inputImageStartY, FD_STARTY);

	return 0;
}

int facedetect_set_detection_width(struct facedetect_params_t *config)
{
	/*
	 * Operation cannot be guaranteed when neither 160<=SIZEX<=320
	 * nor STARTX+SIZEX<=320 is met
	 */
	regw_if(config->inputdata.inputImageWidth, FD_SIZEX);

	return 0;
}

int facedetect_set_detection_height(struct facedetect_params_t *config)
{
	/*
	 * Operation cannot be guaranteed when neither 120<=SIZEY<=240
	 * nor STARTY+SIZEY<=240 is met
	 */
	regw_if(config->inputdata.inputImageWidth, FD_SIZEY);

	return 0;
}

int facedetect_set_threshold(struct facedetect_params_t *config)
{
	/* Threshold value should be between 0 and 9 */
	regw_if(config->inputdata.ThresholdValue, FD_LHIT);

	return 0;
}
