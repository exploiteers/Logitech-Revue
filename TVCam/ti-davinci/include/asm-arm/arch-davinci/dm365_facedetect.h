#ifndef DM365_FACEDETECT
#define DM365_FACEDETECT

#include <linux/ioctl.h>

/* Brief Face detect direction settings */
#define FACE_DETECT_DIRECTION_UP	0
#define FACE_DETECT_DIRECTION_RIGHT	1
#define FACE_DETECT_DIRECTION_LEFT	2

/* Brief Face detect minimum face size settings */
#define MINIMUM_FACE_SIZE_20_PIXEL	0
#define MINIMUM_FACE_SIZE_25_PIXEL	1
#define MINIMUM_FACE_SIZE_32_PIXEL	2
#define MINIMUM_FACE_SIZE_40_PIXEL	3

/*
 * Brief Minimum Work Area Size that need to be allocated by application
 * as a contiguous buffer
 */
#define MIN_WORKAREA_SIZE	(13200 * 4)

/* Brief Face detect status */
typedef enum fd_status {
	FD_STATUS_FREE = 0,
	FD_STATUS_BUSY = 1
} fd_status_t;

/* Brief Face detection sensor input parameters */
struct facedetect_inputdata_t {
	char enable;		/* Facedetect: TRUE: Enable, FALSE: Disable */
	char intEnable;		/* Interrupt: TRUE: Enable, FALSE: Disable */
	unsigned char *inputAddr;	/* Picture data address in SDRAM */
	unsigned char *workAreaAddr;	/* Work area address in SDRAM */
	unsigned char direction;	/* Direction : UP, RIGHT, LEFT */
	unsigned char minFaceSize;	/* Min face size 20,25,32,30 Pixels */
	unsigned short inputImageStartX;	/* Image start X */
	unsigned short inputImageStartY;	/* Image start Y */
	unsigned short inputImageWidth;		/* Image Width */
	unsigned short inputImageHeight;	/* Image Height */
	unsigned char ThresholdValue;	/* Face detect Threshold value */
};

/* Brief Face detection sensor face detect position */
struct facedetect_position_t {
	unsigned short resultX;
	unsigned short resultY;
	unsigned short resultConfidenceLevel;
	unsigned short resultSize;
	unsigned short resultAngle;
};

/* Brief Face detection sensor output parameters */
struct facedetect_outputdata_t {
	struct facedetect_position_t face_position[35];
	unsigned char faceCount;
};

/* Brief Face detection sensor input and output parameters */
struct facedetect_params_t {
	struct facedetect_inputdata_t inputdata;
	struct facedetect_outputdata_t outputdata;
};

/* Brief Device structure keeps track of global information */
struct facedetect_device {
	struct facedetect_params_t facedetect_params;
	unsigned char opened;	/* state of the device */
};

/* ioctls definition */
#define FACE_DETECT_IOC_BASE		'F'
#define FACE_DETECT_SET_HW_PARAM	_IOW(FACE_DETECT_IOC_BASE, 0, \
						struct facedetect_params*)
#define FACE_DETECT_EXECUTE		_IOW(FACE_DETECT_IOC_BASE, 1, \
						struct facedetect_params*)
#define FACE_DETECT_SET_BUFFER		_IOW(FACE_DETECT_IOC_BASE, 2, \
						struct facedetect_params*)
#define FACE_DETECT_GET_HW_PARAM	_IOWR(FACE_DETECT_IOC_BASE, 3, \
						struct facedetect_params*)

#define FACE_DETECT_IOC_MAXNR		4

#ifdef __KERNEL__

struct inode;
struct file;

/* function definition for character driver interface functions */
int facedetect_init(void);
void facedetect_cleanup(void);
int facedetect_open(struct inode *inode, struct file *);
int facedetect_release(struct inode *inode, struct file *);
int facedetect_ioctl(struct inode *inode, struct file *, unsigned int,
		     unsigned long);

#endif	/* __KERNEL__ */
#endif	/* DM365_FACEDETECT */
