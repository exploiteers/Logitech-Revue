#ifndef __IWOLVERINE_H__
#define __IWOLVERINE_H__

/*
 * This section is types used between various portions of the application internally
 */

/*define type for u-boot only */
#include <common.h>
#include <linux/stddef.h>

typedef unsigned char 	UInt8;
typedef unsigned short 	UInt16;
typedef unsigned long 	UInt32;
typedef char 		  	Int8;
typedef short 		  	Int16;
typedef long 		  	Int32;
typedef int				Int;
typedef void			Void;
/* end porting to u-boot */

#if 0
/* These are message IDs for messages being passed to a thread on it's hMsgFifo */
typedef enum {
    Fifo_Msg_START,     // (Re)Start the thread
    Fifo_Msg_CONFIG,    // Configuration change
    Fifo_Msg_RESIZE,    // Resize as dimensions have changed
    Fifo_Msg_RESIZE2,   // Resize as dimensions have changed
    Fifo_Msg_MODE,      // Resize as mode has changed
    Fifo_Msg_MODE2,     // Resize as mode has changed
    Fifo_Msg_CSPACE,    // Colorspace
    Fifo_Msg_CSPACE2,   // Colorspace
    Fifo_Msg_CPROCESS,  // Color Process Disable/Enable
    Fifo_Msg_PLAY,      // Play
    Fifo_Msg_PAUSE,     // Pause
    Fifo_Msg_STOP,      // Stop
    Fifo_Msg_BYPASS,    // Bypass
    Fifo_Msg_QUIT,      // Message to tell the thread to quit

    /* Any message types must be defined above this comment */
    Fifo_Msg_MAX
} Fifo_Msg_ID;

typedef struct Fifo_Msg {
    Fifo_Msg_ID                 msgID;      // Generic message ID for function decode
    // Union of all the possible meanings except the BASIC and GRAPHICS Buffers
    union {                         
        BufferGfx_Dimensions    dimension;      // Fifo_Msg_RESIZE(2): Message to change dimensions based on x & y
        vpbe_mode_enum_t        mode;           // Fifo_Msg_MODE(2):   Message to change dimensions based on enumerated mode for display thread
        ColorSpace_Type         colorSpace;     // Fifo_Msg_CSPACE(2): Message to change color space
        Uint8                   uint8;          // Multiple:           Generic 8 bit byte parameter
    };
} Fifo_Msg, *hFifo_Msg; 
#endif

/* The following enum is used to type the data mode */
typedef enum Video_Format {
    VC_VIDEO_FORMAT_OFF = 0,    /* No Video */
    VC_VIDEO_FORMAT_YUY2,       /* YUYV data 2 Bytes per pixel */
    VC_VIDEO_FORMAT_MJPEG,      /* Compressed data with MJPEG encapsulation on USB */
    VC_VIDEO_FORMAT_H264,       /* H264 */
} Video_Format;

/*
 * This section is types used between the application and the 2211
 */

/* The following structures need to be aligned 1 and packed */
#define __PACKED_ALIGNED    __attribute__ ((packed, aligned (1)))

/* This is the maximum size of the write and should be the same size as the buffer in the 2211 */
#define MAX_I2C_LENGTH      (192+5)

/* T_I2C_WRITE This is the format of the buffer coming from the I2C driver */
typedef struct __PACKED_ALIGNED ST_I2C_WRITE {
    UInt32                  addr;
    UInt8                   readLength;
    UInt8                   data[MAX_I2C_LENGTH - 5];
} T_I2C_WRITE;

/* T_DM365_STATUS */
typedef struct __PACKED_ALIGNED ST_DM365_STATUS {
    UInt8                   mode;
    UInt8                   index;
} T_DM365_STATUS;

/* CT_SCANNING_MODE_CONTROL 0002 */
typedef struct __PACKED_ALIGNED ST_CT_SCANNING_MODE_CONTROL {
    UInt8               bScanningMode;
} T_CT_SCANNING_MODE_CONTROL;

/* CT_AE_MODE_CONTROL 0003*/
typedef struct __PACKED_ALIGNED ST_CT_AE_MODE_CONTROL {
    UInt8               bAutoExposureMode;
/*
    union {
        UInt8               bAutoExposureMode;
        struct {
            UInt8           bManualMode:1;
            UInt8           bAutoMode:1;
            UInt8           bShutterPriorityMode:1;
            UInt8           bAperturePriorityMode:1;
        };
    };
*/
} T_CT_AE_MODE_CONTROL;

/* CT_AE_PRIORITY_CONTROL 0004*/
typedef struct __PACKED_ALIGNED ST_CT_AE_PRIORITY_CONTROL {
    UInt8                   bAutoExposurePriority;
} T_CT_AE_PRIORITY_CONTROL;

/* CT_EXPOSURE TIME_ABSOLUTE_CONTROL 0005:4 */
typedef struct __PACKED_ALIGNED ST_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL {
    UInt32                  dwExposureTimeAbsolute;
} T_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL;

/* CT_EXPOSURE_TIME_RELATIVE_CONTROL 0009 */
typedef struct __PACKED_ALIGNED ST_CT_EXPOSURE_TIME_RELATIVE_CONTROL {
    Int8                    bExposureTimeRelative;
} T_CT_EXPOSURE_TIME_RELATIVE_CONTROL;

/* CT_FOCUS_ABSOLUTE_CONTROL */
typedef struct __PACKED_ALIGNED ST_CT_FOCUS_ABSOLUTE_CONTROL {
    UInt16                  wFocusAbsolute;
} T_CT_FOCUS_ABSOLUTE_CONTROL;

/* CT_FOCUS_RELATIVE_CONTROL */
typedef struct __PACKED_ALIGNED ST_CT_FOCUS_RELATIVE_CONTROL {
    Int8                    bFocusRelative;
    UInt8                   bSpeed;
} T_CT_FOCUS_RELATIVE_CONTROL;

/* CT_FOCUS_AUTO_CONTROL */
typedef struct __PACKED_ALIGNED ST_CT_FOCUS_AUTO_CONTROL {
    UInt8                   bFocusAuto;
} T_CT_FOCUS_AUTO_CONTROL;

/* CT_IRIS_ABSOLUTE_CONTROL */
typedef struct __PACKED_ALIGNED ST_CT_IRIS_ABSOLUTE_CONTROL {
    UInt16                  wIrisAbsolute;
} T_CT_IRIS_ABSOLUTE_CONTROL;

/* CT_IRIS_RELATIVE_CONTROL */
typedef struct __PACKED_ALIGNED ST_CT_IRIS_RELATIVE_CONTROL {
    Int8                    bIrisRelative;
} T_CT_IRIS_RELATIVE_CONTROL;

/* CT_ZOOM_ABSOLUTE_CONTROL */
typedef struct __PACKED_ALIGNED ST_CT_ZOOM_ABSOLUTE_CONTROL {
    UInt16                  wObjectiveFocalLength;
} T_CT_ZOOM_ABSOLUTE_CONTROL;

/* CT_ZOOM_RELATIVE_CONTROL */
typedef struct __PACKED_ALIGNED ST_CT_ZOOM_RELATIVE_CONTROL {
    Int8                    bZoom;
    UInt8                   bDigitalZoom;
    UInt8                   bSpeed;
} T_CT_ZOOM_RELATIVE_CONTROL;

/* CT_PANTILT_ABSOLUTE_CONTROL */
typedef struct __PACKED_ALIGNED ST_CT_PANTILT_ABSOLUTE_CONTROL {
    UInt32                  dwPanAbsolute;
    UInt32                  dwTiltAbsolute;
} T_CT_PANTILT_ABSOLUTE_CONTROL;

/* CT_PANTILT_RELATIVE_CONTROL */
typedef struct __PACKED_ALIGNED ST_CT_PANTILT_RELATIVE_CONTROL {
    Int8                    bPanRelative;
    UInt8                   bPanSpeed;
    Int8                    bTiltRelative;
    UInt8                   bTiltSpeed;
} T_CT_PANTILT_RELATIVE_CONTROL;

/* CT_ROLL_ABSOLUTE_CONTROL */
typedef struct __PACKED_ALIGNED ST_CT_ROLL_ABSOLUTE_CONTROL {
    UInt16                  wRollAbsolute;
} T_CT_ROLL_ABSOLUTE_CONTROL;

/* CT_ROLL_RELATIVE_CONTROL */
typedef struct __PACKED_ALIGNED ST_CT_ROLL_RELATIVE_CONTROL {
    Int8                    bRollRelative;
    UInt8                   bSpeed;
} T_CT_ROLL_RELATIVE_CONTROL;

/* CT_PRIVACY_CONTROL */
typedef struct __PACKED_ALIGNED ST_CT_PRIVACY_CONTROL {
    UInt8                   bPrivacy;
} T_CT_PRIVACY_CONTROL;

/* PU_BACKLIGHT_COMPENSATION_CONTROL 0028:2 */
typedef struct __PACKED_ALIGNED ST_PU_BACKLIGHT_COMPENSATION_CONTROL {
    UInt16                  wBacklightCompensation;
} T_PU_BACKLIGHT_COMPENSATION_CONTROL;

/* PU_BRIGHTNESS_CONTROL 002A:2*/
typedef struct __PACKED_ALIGNED ST_PU_BRIGHTNESS_CONTROL {
    UInt16                  wBrightness;
} T_PU_BRIGHTNESS_CONTROL;

/* PU_CONTRAST_CONTROL 002C:2 */
typedef struct __PACKED_ALIGNED ST_PU_CONTRAST_CONTROL {
    UInt16                  wContrast;
} T_PU_CONTRAST_CONTROL;

/* PU_GAIN_CONTROL 002E:2 */
typedef struct __PACKED_ALIGNED ST_PU_GAIN_CONTROL {
    UInt16                  wGain;
} T_PU_GAIN_CONTROL;

/* PU_POWER_LINE_FREQUENCY_CONTROL 0030:1 */
typedef struct __PACKED_ALIGNED ST_PU_POWER_LINE_FREQUENCY_CONTROL {
    UInt8                   bPowerLineFrequency;
} T_PU_POWER_LINE_FREQUENCY_CONTROL;

/* PU_HUE_CONTROL 0031:2 */
typedef struct __PACKED_ALIGNED ST_PU_HUE_CONTROL {
    Int16                   wHue;
} T_PU_HUE_CONTROL;

/* PU_HUE_AUTO_CONTROL 0033:1*/
typedef struct __PACKED_ALIGNED ST_PU_HUE_AUTO_CONTROL {
    UInt8                   bHueAuto;
} T_PU_HUE_AUTO_CONTROL;

/* PU_SATURATION_CONTROL 0034:2*/
typedef struct __PACKED_ALIGNED ST_PU_SATURATION_CONTROL {
    UInt16                  wSaturation;
} T_PU_SATURATION_CONTROL;

/* PU_SHARPNESS_CONTROL 0036:2*/
typedef struct __PACKED_ALIGNED ST_PU_SHARPNESS_CONTROL {
    UInt16                  wSharpness;
} T_PU_SHARPNESS_CONTROL;

/* PU_GAMMA_CONTROL 0038:2*/
typedef struct __PACKED_ALIGNED ST_PU_GAMMA_CONTROL {
    UInt16                  wGamma;
} T_PU_GAMMA_CONTROL;

/* PU_WHITE_BALANCE_TEMPERATURE_CONTROL 003A:2 */
typedef struct __PACKED_ALIGNED ST_PU_WHITE_BALANCE_TEMPERATURE_CONTROL {
    UInt16                  wWhiteBalanceTemperature;
} T_PU_WHITE_BALANCE_TEMPERATURE_CONTROL;

/* PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL 003C */
typedef struct __PACKED_ALIGNED ST_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL {
    UInt8                   bWhiteBalanceTemperatureAuto;
} T_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL;

/* PU_WHITE_BALANCE_COMPONENT_CONTROL */
typedef struct __PACKED_ALIGNED ST_PU_WHITE_BALANCE_COMPONENT_CONTROL {
    UInt16                  wWhiteBalanceBlue;
    UInt16                  wWhiteBalanceRed;
} T_PU_WHITE_BALANCE_COMPONENT_CONTROL;

/* PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL */
typedef struct __PACKED_ALIGNED ST_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL {
    UInt8                   bWhiteBalanceComponentAuto;
} T_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL;

/* PU_DIGITAL_MULTIPLIER_CONTROL */
typedef struct __PACKED_ALIGNED ST_PU_DIGITAL_MULTIPLIER_CONTROL {
    UInt16                  wMultiplierStep;
} T_PU_DIGITAL_MULTIPLIER_CONTROL;

/* PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL */
typedef struct __PACKED_ALIGNED ST_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL {
    UInt16                  wMultiplierLimit;
} T_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL;

/* PU_ANALOG_VIDEO_STANDARD_CONTROL */
typedef struct __PACKED_ALIGNED ST_PU_ANALOG_VIDEO_STANDARD_CONTROL {
    UInt8                   bVideoStandard;
} T_PU_ANALOG_VIDEO_STANDARD_CONTROL;

/* PU_ANALOG_LOCK_STATUS_CONTROL */
typedef struct __PACKED_ALIGNED ST_PU_ANALOG_LOCK_STATUS_CONTROL {
    UInt8                   bStatus;
} T_PU_ANALOG_LOCK_STATUS_CONTROL;

/* LXU_VIDEO_COLOR_BOOST_CONTROL 0048:1 */
typedef struct __PACKED_ALIGNED ST_LXU_VIDEO_COLOR_BOOST_CONTROL {
    UInt8                   bOn;
} T_LXU_VIDEO_COLOR_BOOST_CONTROL;

/* LXU_NATIVE_MODE_CONTROL 0049:1*/
typedef struct __PACKED_ALIGNED ST_LXU_NATIVE_MODE_CONTROL {
    UInt8                   wNativeMode;
} T_LXU_NATIVE_MODE_CONTROL;

/* LXU_NATIVE_MODE_AUTO_CONTROL 004A:1*/
typedef struct __PACKED_ALIGNED ST_LXU_NATIVE_MODE_AUTO_CONTROL {
    UInt8                   bNativeModeControl;
} T_LXU_NATIVE_MODE_AUTO_CONTROL;

/* LXU_VIDEO_RIGHTLIGHT_MODE_CONTROL 004B:1*/
typedef struct __PACKED_ALIGNED ST_LXU_VIDEO_RIGHTLIGHT_MODE_CONTROL {
    UInt8                   bRightLightModeControl;
} T_LXU_VIDEO_RIGHTLIGHT_MODE_CONTROL;

/* LXU_VIDEO_RIGHTLIGHT_ZOI 004C:10 */
typedef struct __PACKED_ALIGNED ST_LXU_VIDEO_RIGHTLIGHT_ZOI {
	UInt16              wControl;
/*
    union {
        UInt16              wControl;
        UInt16              bAE;
    };
*/
    struct {
        UInt16              X;
        UInt16              Y;
    } dwOrigin;
    struct {
        UInt16              X;
        UInt16              Y;
    } dwExtent;
} T_LXU_VIDEO_RIGHTLIGHT_ZOI;

/* LXU_VIDEO_FW_ZOOM_CONTROL 0056:6*/
typedef struct __PACKED_ALIGNED ST_LXU_VIDEO_FW_ZOOM_CONTROL {
    UInt16                  wPanTiltStartX;
    UInt16                  wPanTiltStartY;
    UInt8                   bZoom1x;
    UInt8                   bZoomFractional;
} T_LXU_VIDEO_FW_ZOOM_CONTROL;

/* LXU_TEST_TDE_MODE_CONTROL 005C*/
typedef struct __PACKED_ALIGNED ST_LXU_TEST_TDE_MODE_CONTROL {
    UInt8                   bMode;
} T_LXU_TEST_TDE_MODE_CONTROL;

/* LXU_TEST_GAIN_CONTROL 005D */
typedef struct __PACKED_ALIGNED ST_LXU_TEST_GAIN_CONTROL {
    UInt8                   bGain;
} T_LXU_TEST_GAIN_CONTROL;

/* LXU_TEST_LOW_LIGHT_PRIORITY_CONTROL 005E */
typedef struct __PACKED_ALIGNED ST_LXU_TEST_LOW_LIGHT_PRIORITY_CONTROL {
    UInt8                   bLowLightMode;
} T_LXU_TEST_LOW_LIGHT_PRIORITY_CONTROL;

/* LXU_TEST_COLOR_PROCESSING_DISABLE_CONTROL 005F */
typedef struct __PACKED_ALIGNED ST_LXU_TEST_COLOR_PROCESSING_DISABLE_CONTROL {
    UInt8                   bDisable;
} T_LXU_TEST_COLOR_PROCESSING_DISABLE_CONTROL;

/* LXU_TEST_PIXEL_DEFECT_CORRECTION_CONTROL 0060*/
typedef struct __PACKED_ALIGNED ST_LXU_TEST_PIXEL_DEFECT_CORRECTION_CONTROL {
    UInt8                   bPixelDefect;
} T_LXU_TEST_PIXEL_DEFECT_CORRECTION_CONTROL;

/* LXU_TEST_LENS_SHADING_COMPENSATION_CONTRO 0061 */
typedef struct __PACKED_ALIGNED ST_LXU_TEST_LENS_SHADING_COMPENSATION_CONTROL {
    UInt8                   bLensComp;
} T_LXU_TEST_LENS_SHADING_COMPENSATION_CONTROL;

/* LXU_TEST_GAMMA_CONTROL 0062*/
typedef struct __PACKED_ALIGNED ST_LXU_TEST_GAMMA_CONTROL {
    UInt8                   bValue;
} T_LXU_TEST_GAMMA_CONTROL;

/* LXU_TEST_INTEGRATION_TIME_CONTROL 0063:4 */
typedef struct __PACKED_ALIGNED ST_LXU_TEST_INTEGRATION_TIME_CONTROL {
    UInt32                  dwIntegrationTime;
} T_LXU_TEST_INTEGRATION_TIME_CONTROL;

/* LXU_TEST_RAW_DATA_BITS_PER_PIXEL_CONTROL 0067 */
typedef struct __PACKED_ALIGNED ST_LXU_TEST_RAW_DATA_BITS_PER_PIXEL_CONTROL {
    UInt8                   bBitsPerPixel;
} T_LXU_TEST_RAW_DATA_BITS_PER_PIXEL_CONTROL;

/* LXU_VIDEO_FORMAT_CONTROL 0068:4*/
typedef struct __PACKED_ALIGNED ST_LXU_VIDEO_FORMAT_CONTROL {
    UInt16                  wFrameRate;
    UInt8                   bFrameIndex;
    UInt8                   bFormatIndex;
} T_LXU_VIDEO_FORMAT_CONTROL;

/* LXU_RING_TONE_CONTROL  006C:4  */
typedef struct __PACKED_ALIGNED ST_LXU_RING_TONE_CONTROL {
    UInt32                  dwRingTone;
} T_LXU_RING_TONE_CONTROL;

/* LXU_POWER_MODE_CONTROL  0070:4 */
typedef struct __PACKED_ALIGNED ST_LXU_POWER_MODE_CONTROL {
    UInt32                  dwPowerMode;
} T_LXU_POWER_MODE_CONTROL;

/* LXU_ENTER_DFU_MODE  0074:4     */
typedef struct __PACKED_ALIGNED ST_LXU_ENTER_DFU_MODE {
    UInt32                  dwDFUMode;
} T_LXU_ENTER_DFU_MODE;

/* LXU_DATA_DOWNLOAD_MODE 0078:4  */
typedef struct __PACKED_ALIGNED ST_LXU_DATA_DOWNLOAD_MODE {
    UInt32                  dwDataDownloadMode;
} T_LXU_DATA_DOWNLOAD_MODE;

/* CXU_ENCODER_CONFIGURATION_CONTROL 007C:16 */
typedef struct __PACKED_ALIGNED ST_CXU_ENCODER_CONFIGURATION_CONTROL {
    UInt32                  nSliceSize;
    UInt16                  nNumberOfSlices;
    UInt8                   nProfile;
    UInt8                   nLevel;
    UInt32                  nBitRate;
    UInt32                  nBitRateMax;
} T_CXU_ENCODER_CONFIGURATION_CONTROL;

/* CXU_ENCODER_CONFIGURATION_ADV_CONTROL 008C:22 */
typedef struct __PACKED_ALIGNED ST_CXU_ENCODER_CONFIGURATION_ADV_CONTROL {
    UInt8                   nQuantizerMin;
    UInt8                   nQuantizerMax;
    UInt8                   nQuantizerFixed;
    UInt8                   nMotionEstimationDepth;
    UInt8                   nRCMode;
    UInt8                   nReferenceFrames;
    UInt32                  nVBVBufSize;
    UInt32                  nIntraRefreshFrequency;
    UInt32                  nIntraRefreshMethod;
    UInt8                   bScenecutDetection;
    UInt8                   bEnableLoopFilter;
    UInt8                   bEnable1x11Transform;
    UInt8                   bCabac;
} T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL;

/* CXU_FRAME_TYPE_CONTROL */
typedef struct __PACKED_ALIGNED ST_CXU_FRAME_TYPE_CONTROL {
    UInt8                   nFrameType;
} T_CXU_FRAME_TYPE_CONTROL;

/* CXU_BIT_RATE_CONTROL */
typedef struct __PACKED_ALIGNED ST_CXU_BIT_RATE_CONTROL {
    UInt32                  nBitRate;
    UInt32                  nBitRateMax;
} T_CXU_BIT_RATE_CONTROL;

/* CXU_FRAME_RATE_CONTROL */
typedef struct __PACKED_ALIGNED ST_CXU_FRAME_RATE_CONTROL {
    UInt32                  nFrameRate;
} T_CXU_FRAME_RATE_CONTROL;

/* CXU_CAMERA_PPS_CONTROL */
typedef struct __PACKED_ALIGNED ST_CXU_CAMERA_PPS_CONTROL {
    UInt8                   pPPS[512];
} T_CXU_CAMERA_PPS_CONTROL;

/* CXU_CAMERA_SPS_CONTROL */
typedef struct __PACKED_ALIGNED ST_CXU_CAMERA_SPS_CONTROL {
    UInt8                   pSPS[512];
} T_CXU_CAMERA_SPS_CONTROL;

/* CXU_CAMERA_DELAY */
typedef struct __PACKED_ALIGNED ST_CXU_CAMERA_DELAY {
    UInt32                  nDelay;
    UInt32                  nDelayVariance;
} T_CXU_CAMERA_DELAY;

/* CXXU_FILTER_CONTROL */
typedef struct __PACKED_ALIGNED ST_CXXU_FILTER_CONTROL {
    UInt8                   bTemporalFilter;
    UInt8                   bSpatialFilter;
    UInt8                   bColor;
} T_CXXU_FILTER_CONTROL;

/* CXU_PROFILING_DATA */
typedef struct __PACKED_ALIGNED ST_CXU_PROFILING_DATA {
    UInt8                   bProfileOn;
    UInt8                   byFirstUnit;
    UInt8                   byNbOfProcessingUnits;
} T_CXU_PROFILING_DATA;

/* Interface variables to appear at I2C sub address FFAA0000 */
typedef struct __PACKED_ALIGNED ST_WOLVERINE_INTERFACE_VARS {
    /* 0xFFAA_0000 */   T_DM365_STATUS                                  dm365_status                              ;
    /* 0xFFAA_0002 */   T_CT_SCANNING_MODE_CONTROL                      ct_scanning_mode_control                  ;
    /* 0xFFAA_0003 */   T_CT_AE_MODE_CONTROL                            ct_ae_mode_control                        ;
    /* 0xFFAA_0004 */   T_CT_AE_PRIORITY_CONTROL                        ct_ae_priority_control                    ;
    /* 0xFFAA_0005 */   T_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL             ct_exposure_time_absolute_control         ;
    /* 0xFFAA_0009 */   T_CT_EXPOSURE_TIME_RELATIVE_CONTROL             ct_exposure_time_relative_control         ;
    /* 0xFFAA_000A */   T_CT_FOCUS_ABSOLUTE_CONTROL                     ct_focus_absolute_control                 ;
    /* 0xFFAA_000C */   T_CT_FOCUS_RELATIVE_CONTROL                     ct_focus_relative_control                 ;
    /* 0xFFAA_000E */   T_CT_FOCUS_AUTO_CONTROL                         ct_focus_auto_control                     ;
    /* 0xFFAA_000F */   T_CT_IRIS_ABSOLUTE_CONTROL                      ct_iris_absolute_control                  ;
    /* 0xFFAA_0011 */   T_CT_IRIS_RELATIVE_CONTROL                      ct_iris_relative_control                  ;
    /* 0xFFAA_0012 */   T_CT_ZOOM_ABSOLUTE_CONTROL                      ct_zoom_absolute_control                  ;
    /* 0xFFAA_0014 */   T_CT_ZOOM_RELATIVE_CONTROL                      ct_zoom_relative_control                  ;
    /* 0xFFAA_0017 */   T_CT_PANTILT_ABSOLUTE_CONTROL                   ct_pantilt_absolute_control               ;
    /* 0xFFAA_001F */   T_CT_PANTILT_RELATIVE_CONTROL                   ct_pantilt_relative_control               ;
    /* 0xFFAA_0023 */   T_CT_ROLL_ABSOLUTE_CONTROL                      ct_roll_absolute_control                  ;
    /* 0xFFAA_0025 */   T_CT_ROLL_RELATIVE_CONTROL                      ct_roll_relative_control                  ;
    /* 0xFFAA_0027 */   T_CT_PRIVACY_CONTROL                            ct_privacy_control                        ;

    /* 0xFFAA_0028 */   T_PU_BACKLIGHT_COMPENSATION_CONTROL             pu_backlight_compensation_control         ; 
    /* 0xFFAA_002A */   T_PU_BRIGHTNESS_CONTROL                         pu_brightness_control                     ; 
    /* 0xFFAA_002C */   T_PU_CONTRAST_CONTROL                           pu_contrast_control                       ; 
    /* 0xFFAA_002E */   T_PU_GAIN_CONTROL                               pu_gain_control                           ; 
    /* 0xFFAA_0030 */   T_PU_POWER_LINE_FREQUENCY_CONTROL               pu_power_line_frequency_control           ; 
    /* 0xFFAA_0031 */   T_PU_HUE_CONTROL                                pu_hue_control                            ; 
    /* 0xFFAA_0033 */   T_PU_HUE_AUTO_CONTROL                           pu_hue_auto_control                       ; 
    /* 0xFFAA_0034 */   T_PU_SATURATION_CONTROL                         pu_saturation_control                     ; 
    /* 0xFFAA_0036 */   T_PU_SHARPNESS_CONTROL                          pu_sharpness_control                      ; 
    /* 0xFFAA_0038 */   T_PU_GAMMA_CONTROL                              pu_gamma_control                          ; 
    /* 0xFFAA_003A */   T_PU_WHITE_BALANCE_TEMPERATURE_CONTROL          pu_white_balance_temperature_control      ; 
    /* 0xFFAA_003C */   T_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL     pu_white_balance_temperature_auto_control ; 
    /* 0xFFAA_003D */   T_PU_WHITE_BALANCE_COMPONENT_CONTROL            pu_white_balance_component_control        ; 
    /* 0xFFAA_0041 */   T_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL       pu_white_balance_component_auto_control   ; 
    /* 0xFFAA_0042 */   T_PU_DIGITAL_MULTIPLIER_CONTROL                 pu_digital_multiplier_control             ; 
    /* 0xFFAA_0044 */   T_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL           pu_digital_multiplier_limit_control       ; 
    /* 0xFFAA_0046 */   T_PU_ANALOG_VIDEO_STANDARD_CONTROL              pu_analog_video_standard_control          ; 
    /* 0xFFAA_0047 */   T_PU_ANALOG_LOCK_STATUS_CONTROL                 pu_analog_lock_status_control             ; 

    /* 0xFFAA_0048 */   T_LXU_VIDEO_COLOR_BOOST_CONTROL                 lxu_video_color_boost_control             ; 
    /* 0xFFAA_0049 */   T_LXU_NATIVE_MODE_CONTROL                       lxu_native_mode_control                   ; 
    /* 0xFFAA_004A */   T_LXU_NATIVE_MODE_AUTO_CONTROL                  lxu_native_mode_auto_control              ; 
    /* 0xFFAA_004B */   T_LXU_VIDEO_RIGHTLIGHT_MODE_CONTROL             lxu_video_rightlight_mode_control         ; 
    /* 0xFFAA_004C */   T_LXU_VIDEO_RIGHTLIGHT_ZOI                      lxu_video_rightlight_zoi                  ; 
    /* 0xFFAA_0056 */   T_LXU_VIDEO_FW_ZOOM_CONTROL                     lxu_video_fw_zoom_control                 ; 
    /* 0xFFAA_005C */   T_LXU_TEST_TDE_MODE_CONTROL                     lxu_test_tde_mode_control                 ; 
    /* 0xFFAA_005D */   T_LXU_TEST_GAIN_CONTROL                         lxu_test_gain_control                     ; 
    /* 0xFFAA_005E */   T_LXU_TEST_LOW_LIGHT_PRIORITY_CONTROL           lxu_test_low_light_priority_control       ; 
    /* 0xFFAA_005F */   T_LXU_TEST_COLOR_PROCESSING_DISABLE_CONTROL     lxu_test_color_processing_disable_control ; 
    /* 0xFFAA_0060 */   T_LXU_TEST_PIXEL_DEFECT_CORRECTION_CONTROL      lxu_test_pixel_defect_correction_control  ; 
    /* 0xFFAA_0061 */   T_LXU_TEST_LENS_SHADING_COMPENSATION_CONTROL    lxu_test_lens_shading_compensation_control; 
    /* 0xFFAA_0062 */   T_LXU_TEST_GAMMA_CONTROL                        lxu_test_gamma_control                    ; 
    /* 0xFFAA_0063 */   T_LXU_TEST_INTEGRATION_TIME_CONTROL             lxu_test_integration_time_control         ; 
    /* 0xFFAA_0067 */   T_LXU_TEST_RAW_DATA_BITS_PER_PIXEL_CONTROL      lxu_test_raw_data_bits_per_pixel_control  ; 

    /* 0xFFAA_0068 */   T_LXU_VIDEO_FORMAT_CONTROL                      lxu_video_format_control                  ;
    /* 0xFFAA_006C */   T_LXU_RING_TONE_CONTROL                         lxu_ring_tone_control                     ;
    /* 0xFFAA_0070 */   T_LXU_POWER_MODE_CONTROL                        lxu_power_mode_control                    ;
    /* 0xFFAA_0074 */   T_LXU_ENTER_DFU_MODE                            lxu_enter_dfu_mode                        ;
    /* 0xFFAA_0078 */   T_LXU_DATA_DOWNLOAD_MODE                        lxu_data_download_mode                    ;

    /* 0xFFAA_007C */   T_CXU_ENCODER_CONFIGURATION_CONTROL             cxu_encoder_configuration_control         ;
    /* 0xFFAA_008C */   T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL         cxu_encoder_configuration_adv_control     ;
    /* 0xFFAA_00A2 */   T_CXU_FRAME_TYPE_CONTROL                        cxu_frame_type_control                    ;
    /* 0xFFAA_00A3 */   T_CXU_BIT_RATE_CONTROL                          cxu_bit_rate_control                      ;
    /* 0xFFAA_00AB */   T_CXU_FRAME_RATE_CONTROL                        cxu_frame_rate_control                    ;
    /* 0xFFAA_00AF */   T_CXU_CAMERA_PPS_CONTROL                        cxu_camera_pps_control                    ;
    /* 0xFFAA_02AF */   T_CXU_CAMERA_SPS_CONTROL                        cxu_camera_sps_control                    ;
    /* 0xFFAA_04AF */   T_CXU_CAMERA_DELAY                              cxu_camera_delay                          ;
    /* 0xFFAA_04B7 */   T_CXXU_FILTER_CONTROL                           cxxu_filter_control                       ;
    /* 0xFFAA_04BA */   T_CXU_PROFILING_DATA                            cxu_profiling_data                        ;
} T_WOLVERINE_INTERFACE_VARS, *T_WOLVERINE_INTERFACE_VARS_HANDLE;

/* Managed interface type */
typedef struct ST_WOLVERINE_INTERFACE {
    /*pthread_mutex_t             mutex;*/
    T_WOLVERINE_INTERFACE_VARS  vars;
    UInt8                       dirty[(sizeof(T_WOLVERINE_INTERFACE_VARS)+7)/8]; /* One bit per byte in the T_WOLVERINE_INTERFACE rounded up */
} T_WOLVERINE_INTERFACE, *T_WOLVERINE_INTERFACE_HANDLE;

/* Private all be it Global interface handle */
extern T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface;

/* Private Interface prototypes on to be called from one of the Macros below */
extern Int _wolverineIntIsDirty(T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface, Int offset, Int length);
extern Int _wolverineIntClrDirty(T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface, Int offset, Int length);
extern Int _wolverineIntSetDirty(T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface, Int offset, Int length);

/* Macros for accessing the dirty list in a readable way */
#define wolverineIntDirtyOffset(hWolverineInterface,member) (offsetof(T_WOLVERINE_INTERFACE_VARS,member))
#define wolverineIntDirtyLength(hWolverineInterface,member) (sizeof(hWolverineInterface->vars.member))
#define wolverineIntIsDirty(hWolverineInterface,member) \
    _wolverineIntIsDirty(hWolverineInterface, \
                        wolverineIntDirtyOffset(hWolverineInterface,member), \
                        wolverineIntDirtyLength(hWolverineInterface,member))
#define wolverineIntClrDirty(hWolverineInterface,member) \
    _wolverineIntClrDirty(hWolverineInterface, \
                        wolverineIntDirtyOffset(hWolverineInterface,member), \
                        wolverineIntDirtyLength(hWolverineInterface,member))
#define wolverineIntSetDirty(hWolverineInterface,member) \
    _wolverineIntSetDirty(hWolverineInterface, \
                        wolverineIntDirtyOffset(hWolverineInterface,member), \
                        wolverineIntDirtyLength(hWolverineInterface,member))
#define wolverineIntClrDirtyBit(hWolverineInterface,offset) \
    (hWolverineInterface)->dirty[(offset) / 8] &= ~(1 << ((offset) % 8))
#define wolverineIntSetDirtyBit(hWolverineInterface,offset) \
    (hWolverineInterface)->dirty[(offset) / 8] |=  (1 << ((offset) % 8))

/* Public Interface prototypes */
extern T_WOLVERINE_INTERFACE_HANDLE initWolverineInt(Void);
extern T_WOLVERINE_INTERFACE_VARS_HANDLE getWolverineIntVars(T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface);
extern Void releaseWolverineIntVars(T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface);
extern Int sendWolverineIntStatus(T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface, UInt8 index, UInt8 mode);

extern Int wolverineIntWrite(UInt32 offset, UInt8 value);
extern Int wolverineIntWriteBuffer(T_I2C_WRITE* buffer, UInt8 length);
extern Int wolverineIntRead(UInt32 offset);
extern Int wolverineIntReadBuffer(UInt8* buffer, UInt8 length, UInt32 offset);

#if 0
/* not support in u-boot */
extern Int wolverineIntSendMessage(Fifo_Handle hFifo, hFifo_Msg hMsg);
#endif

#ifdef CHECK_STRUCTURES
    extern Void dumpInterfaceTypes (T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface);
#endif // CHECK_STRUCTURES

#endif // __IWOLVERINE_H__
