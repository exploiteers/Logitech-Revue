/*
 * IWolverine.c
 */

#include <common.h>
#include <asm/arch/hardware.h> /* GPIO defines */
#include <asm-arm/errno.h>
#include <malloc.h> /* assert() function */
/*
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <arpa/inet.h>
*/
/*#include "helper.h"*/
#include <linux/stddef.h>
#include <IWolverine.h>
#include <dfu.h>

#define DBG printf
#define CHECK_STRUCTURES

/* Private all be it Global interface handle */
T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface = NULL;

/******************************************************************************
 * utility functions: toggle_gpio_to_notify_2211
 * return: 0: success
 *         1: fail
 ******************************************************************************/
#define GPIO_OUT_DATA0	0x01c67014
#define GPIO_SET_DATA0	0x01c67018
#define GPIO_CLR_DATA0	0x01c6701c

#define GPIO_31_MASK	0x80000000
int toggle_gpio_to_notify_2211(void)
{
	/* GPIO31 is driven (from high to) low in OUT_DATA0 register */
	REG(GPIO_OUT_DATA0) &= ~GPIO_31_MASK;
	udelay(100); /* 100 us */
	/* GPIO31 is driven (from low to) high in OUT_DATA0 register */
	REG(GPIO_OUT_DATA0) |= GPIO_31_MASK;

	return 0;
}

/******************************************************************************
 * initWolverineInt
 ******************************************************************************/
T_WOLVERINE_INTERFACE_HANDLE initWolverineInt(Void)
{
    static T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface = NULL;

    /* return the current interface if it allready has been setup */
    if (hWolverineInterface) return hWolverineInterface;

    /* Create the interface object */
    hWolverineInterface = calloc(1, sizeof(T_WOLVERINE_INTERFACE));

#ifndef FAST_BOOT
	printf("%s(): sizeof(T_WOLVERINE_INTERFACE)=0x%x\n", __FUNCTION__, sizeof(T_WOLVERINE_INTERFACE));
#endif

    /* If we can't create it error out */
    if (!hWolverineInterface) return NULL;

	/*dumpInterfaceTypes( hWolverineInterface );*/
    
	/* Initialize the mutex: not applicable to u-boot */
    /*pthread_mutex_init(&hWolverineInterface->mutex, NULL);*/

    /* Load any defaults ... */

    /* ToDo: Setup I2C slave */

    return hWolverineInterface;
}

/******************************************************************************
 * getWolverineIntVars
 ******************************************************************************/
T_WOLVERINE_INTERFACE_VARS_HANDLE getWolverineIntVars(T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface)
{
    assert(hWolverineInterface);

    /*pthread_mutex_lock(&hWolverineInterface->mutex);*/

    return &hWolverineInterface->vars;
}

/******************************************************************************
 * releaseWolverineIntVars
 ******************************************************************************/
Void releaseWolverineIntVars(T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface)
{
    assert(hWolverineInterface);

    /*pthread_mutex_unlock(&hWolverineInterface->mutex);*/
}

/******************************************************************************
 * sendWolverineIntStatus
 ******************************************************************************/
Int sendWolverineIntStatus(T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface, UInt8 index, UInt8 mode)
{
    T_WOLVERINE_INTERFACE_VARS_HANDLE    hWolverineVars = getWolverineIntVars(hWolverineInterface);
    if (!hWolverineVars) return -EINVAL;

    hWolverineVars->dm365_status.index    = index;
    hWolverineVars->dm365_status.mode     = mode;

    /* ToDo Add code to actually send the status to the 2211/host */
	toggle_gpio_to_notify_2211();

    releaseWolverineIntVars(hWolverineInterface);
    return 0;
}

/******************************************************************************
 * _wolverineIntIsDirty should be accessed through the wolverineIntIsDirty macro
 ******************************************************************************/
Int _wolverineIntIsDirty(T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface, Int offset, Int length)
{
    Int     result = 0;
    Int     iByte = offset / 8;         // First byte to process
    Int     iBit  = offset % 8;         // First bit in the current byte
    UInt8   bMask;                      // Mask to apply to current byte

    assert(hWolverineInterface);
    assert(offset <= sizeof(T_WOLVERINE_INTERFACE_VARS));

    while (length > 0) {
        bMask = ((1 << length) - 1) << iBit;
        result |= hWolverineInterface->dirty[iByte] & bMask;

        /* Now update the length based on the portion we just did */
        length -= (8 - iBit);
        iBit = 0;
        iByte++;
    }

    return result;
}

/******************************************************************************
 * _wolverineIntClrDirty should be accessed through the wolverineIntClrDirty macro
 ******************************************************************************/
Int _wolverineIntClrDirty(T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface, 
                          Int offset, Int length)
{
    Int     result = 0;
    Int     iByte = offset / 8;         // First byte to process
    Int     iBit  = offset % 8;         // First bit in the current byte
    UInt8   bMask;                      // Mask to apply to current byte

    assert(hWolverineInterface);
    assert(offset <= sizeof(T_WOLVERINE_INTERFACE_VARS));

    while (length > 0) {
        bMask = ((1 << length) - 1) << iBit;
        hWolverineInterface->dirty[iByte] &= ~bMask;

        /* Now update the length based on the portion we just did */
        length -= (8 - iBit);
        iBit = 0;
        iByte++;
    }

    return result;
}

/******************************************************************************
 * _wolverineIntSetDirty should be accessed through the wolverineIntSetDirty macro
 ******************************************************************************/
Int _wolverineIntSetDirty(T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface, 
                          Int offset, Int length)
{
    Int     result = 0;
    Int     iByte = offset / 8;         // First byte to process
    Int     iBit  = offset % 8;         // First bit in the current byte
    UInt8   bMask;                      // Mask to apply to current byte

    assert(hWolverineInterface);
    assert(offset <= sizeof(T_WOLVERINE_INTERFACE_VARS));

    while (length > 0) {
        bMask = ((1 << length) - 1) << iBit;
        hWolverineInterface->dirty[iByte] |= bMask;

        /* Now update the length based on the portion we just did */
        length -= (8 - iBit);
        iBit = 0;
        iByte++;
    }

    return result;
}

/******************************************************************************
 * wolverineIntWrite to be called from the I2C slave interface
 ******************************************************************************/
Int wolverineIntWrite(UInt32 offset, UInt8 value)
{
    UInt8* pData;

    assert(offset >= 0xFFAA0000);
    offset -= 0xFFAA0000; // Convert to offset into the structure;
    if (offset >= sizeof(T_WOLVERINE_INTERFACE_VARS)) {
		printf("offset larger than data structure\n");
        return -1;
    }
    assert(hWolverineInterface);

    // Get a pointer and lock the vars
    pData = (UInt8*) getWolverineIntVars(hWolverineInterface) + offset;

    // Write to the structure
    *pData = value;

    // Mark the piece written to as dirty
    wolverineIntSetDirtyBit(hWolverineInterface,offset);

    // Release the vars
    releaseWolverineIntVars(hWolverineInterface);

    return 0;
}

/******************************************************************************
 * wolverineIntWriteBuffer to be called from the I2C slave interface
 ******************************************************************************/
Int wolverineIntWriteBuffer(T_I2C_WRITE* buffer, UInt8 length)
{
    UInt8* pData;
    UInt32 offset = ntohl(buffer->addr);

    assert(offset >= 0xFFAA0000);
    offset -= 0xFFAA0000; // Convert to offset into the structure;
    if (offset >= sizeof(T_WOLVERINE_INTERFACE_VARS)) {
        printf("offset larger than datastructure\n");
        return -1;
    }
    assert(hWolverineInterface);

    length -= offsetof(T_I2C_WRITE,data);
    assert(length > 0);

    if (offset + length >= sizeof(T_WOLVERINE_INTERFACE_VARS)) {
        printf("offset + length larger than datastructure\n");
        return -1;
    }

    // Get a pointer and lock the vars
    pData = (UInt8*) getWolverineIntVars(hWolverineInterface) + offset;

    // Write to the structure
    memcpy(pData, buffer->data, length);

    // Mark the piece(s) written to as dirty
    _wolverineIntSetDirty(hWolverineInterface, offset, length);

    // Release the vars
    releaseWolverineIntVars(hWolverineInterface);

    return 0;
}

/******************************************************************************
 * wolverineIntRead to be called from the I2C slave interface
 ******************************************************************************/
Int wolverineIntRead(UInt32 offset)
{
    UInt8* pData;
    Int result = -1;

    assert(offset >= 0xFFAA0000);
    offset -= 0xFFAA0000; // Convert to offset into the structure;
    if (offset >= sizeof(T_WOLVERINE_INTERFACE_VARS)) {
        printf("offset larger than data structure\n");
        return -1;
    }
    assert(hWolverineInterface);

    // Get a pointer and lock the vars
    pData = (UInt8*) getWolverineIntVars(hWolverineInterface) + offset;

    // Read from the structure
    result = *pData;

    // Release the vars
    releaseWolverineIntVars(hWolverineInterface);

    return result;
}

/******************************************************************************
 * wolverineIntReadBuffer to be called from the I2C slave interface
 ******************************************************************************/
Int wolverineIntReadBuffer(UInt8* buffer, UInt8 length, UInt32 offset)
{
    UInt8* pData;

    assert(offset >= 0xFFAA0000);
    offset -= 0xFFAA0000; // Convert to offset into the structure;
    if (offset + length >= sizeof(T_WOLVERINE_INTERFACE_VARS)) {
        printf("offset + length larger than datastructure\n");
        return -1;
    }
    assert(hWolverineInterface);

    // Get a pointer and lock the vars
    pData = (UInt8*) getWolverineIntVars(hWolverineInterface) + offset;

    // Read from the structure
    memcpy(buffer, pData, length);

    // Release the vars
    releaseWolverineIntVars(hWolverineInterface);

    return 0;
}

#define DBG printf
#define CHECK_STRUCTURES
#ifdef CHECK_STRUCTURES
/******************************************************************************
 * dumpInterfaceTypes test and debug the Wolverine Interface stuff
 ******************************************************************************/
Void dumpInterfaceTypes (T_WOLVERINE_INTERFACE_HANDLE hWolverineInterface)
{
    getWolverineIntVars(hWolverineInterface);
    DBG("T_DM365_STATUS                                size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, dm365_status                              ), wolverineIntDirtyOffset(hWolverineInterface, dm365_status                              ) );

    DBG("T_CT_SCANNING_MODE_CONTROL                    size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_scanning_mode_control                  ), wolverineIntDirtyOffset(hWolverineInterface, ct_scanning_mode_control                  ) );

    DBG("T_CT_AE_MODE_CONTROL                          size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_ae_mode_control                        ), wolverineIntDirtyOffset(hWolverineInterface, ct_ae_mode_control                        ) );

    DBG("T_CT_AE_PRIORITY_CONTROL                      size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_ae_priority_control                    ), wolverineIntDirtyOffset(hWolverineInterface, ct_ae_priority_control                    ) );

    DBG("T_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL           size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_exposure_time_absolute_control         ), wolverineIntDirtyOffset(hWolverineInterface, ct_exposure_time_absolute_control         ) );

    DBG("T_CT_EXPOSURE_TIME_RELATIVE_CONTROL           size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_exposure_time_relative_control         ), wolverineIntDirtyOffset(hWolverineInterface, ct_exposure_time_relative_control         ) );

    DBG("T_CT_FOCUS_ABSOLUTE_CONTROL                   size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_focus_absolute_control                 ), wolverineIntDirtyOffset(hWolverineInterface, ct_focus_absolute_control                 ) );

    DBG("T_CT_FOCUS_RELATIVE_CONTROL                   size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_focus_relative_control                 ), wolverineIntDirtyOffset(hWolverineInterface, ct_focus_relative_control                 ) );

    DBG("T_CT_FOCUS_AUTO_CONTROL                       size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_focus_auto_control                     ), wolverineIntDirtyOffset(hWolverineInterface, ct_focus_auto_control                     ) );

    DBG("T_CT_IRIS_ABSOLUTE_CONTROL                    size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_iris_absolute_control                  ), wolverineIntDirtyOffset(hWolverineInterface, ct_iris_absolute_control                  ) );

    DBG("T_CT_IRIS_RELATIVE_CONTROL                    size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_iris_relative_control                  ), wolverineIntDirtyOffset(hWolverineInterface, ct_iris_relative_control                  ) );

    DBG("T_CT_ZOOM_ABSOLUTE_CONTROL                    size= %d offset= %04X\n", 
wolverineIntDirtyLength(hWolverineInterface, ct_zoom_absolute_control                  ), 
wolverineIntDirtyOffset(hWolverineInterface, ct_zoom_absolute_control                  ) );

    DBG("T_CT_ZOOM_RELATIVE_CONTROL                    size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_zoom_relative_control                  ), wolverineIntDirtyOffset(hWolverineInterface, ct_zoom_relative_control                  ) );

    DBG("T_CT_PANTILT_ABSOLUTE_CONTROL                 size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_pantilt_absolute_control               ), wolverineIntDirtyOffset(hWolverineInterface, ct_pantilt_absolute_control               ) );

    DBG("T_CT_PANTILT_RELATIVE_CONTROL                 size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_pantilt_relative_control               ), wolverineIntDirtyOffset(hWolverineInterface, ct_pantilt_relative_control               ) );
    DBG("T_CT_ROLL_ABSOLUTE_CONTROL                    size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_roll_absolute_control                  ), wolverineIntDirtyOffset(hWolverineInterface, ct_roll_absolute_control                  ) );
    DBG("T_CT_ROLL_RELATIVE_CONTROL                    size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_roll_relative_control                  ), wolverineIntDirtyOffset(hWolverineInterface, ct_roll_relative_control                  ) );
    DBG("T_CT_PRIVACY_CONTROL                          size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, ct_privacy_control                        ), wolverineIntDirtyOffset(hWolverineInterface, ct_privacy_control                        ) );
    DBG("T_PU_BACKLIGHT_COMPENSATION_CONTROL           size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_backlight_compensation_control         ), wolverineIntDirtyOffset(hWolverineInterface, pu_backlight_compensation_control         ) );
    DBG("T_PU_BRIGHTNESS_CONTROL                       size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_brightness_control                     ), wolverineIntDirtyOffset(hWolverineInterface, pu_brightness_control                     ) );
    DBG("T_PU_CONTRAST_CONTROL                         size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_contrast_control                       ), wolverineIntDirtyOffset(hWolverineInterface, pu_contrast_control                       ) );
    DBG("T_PU_GAIN_CONTROL                             size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_gain_control                           ), wolverineIntDirtyOffset(hWolverineInterface, pu_gain_control                           ) );
    DBG("T_PU_POWER_LINE_FREQUENCY_CONTROL             size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_power_line_frequency_control           ), wolverineIntDirtyOffset(hWolverineInterface, pu_power_line_frequency_control           ) );
    DBG("T_PU_HUE_CONTROL                              size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_hue_control                            ), wolverineIntDirtyOffset(hWolverineInterface, pu_hue_control                            ) );
    DBG("T_PU_HUE_AUTO_CONTROL                         size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_hue_auto_control                       ), wolverineIntDirtyOffset(hWolverineInterface, pu_hue_auto_control                       ) );
    DBG("T_PU_SATURATION_CONTROL                       size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_saturation_control                     ), wolverineIntDirtyOffset(hWolverineInterface, pu_saturation_control                     ) );
    DBG("T_PU_SHARPNESS_CONTROL                        size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_sharpness_control                      ), wolverineIntDirtyOffset(hWolverineInterface, pu_sharpness_control                      ) );
    DBG("T_PU_GAMMA_CONTROL                            size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_gamma_control                          ), wolverineIntDirtyOffset(hWolverineInterface, pu_gamma_control                          ) );
    DBG("T_PU_WHITE_BALANCE_TEMPERATURE_CONTROL        size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_white_balance_temperature_control      ), wolverineIntDirtyOffset(hWolverineInterface, pu_white_balance_temperature_control      ) );
    DBG("T_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL   size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_white_balance_temperature_auto_control ), wolverineIntDirtyOffset(hWolverineInterface, pu_white_balance_temperature_auto_control ) );
    DBG("T_PU_WHITE_BALANCE_COMPONENT_CONTROL          size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_white_balance_component_control        ), wolverineIntDirtyOffset(hWolverineInterface, pu_white_balance_component_control        ) );
    DBG("T_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL     size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_white_balance_component_auto_control   ), wolverineIntDirtyOffset(hWolverineInterface, pu_white_balance_component_auto_control   ) );
    DBG("T_PU_DIGITAL_MULTIPLIER_CONTROL               size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_digital_multiplier_control             ), wolverineIntDirtyOffset(hWolverineInterface, pu_digital_multiplier_control             ) );
    DBG("T_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL         size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_digital_multiplier_limit_control       ), wolverineIntDirtyOffset(hWolverineInterface, pu_digital_multiplier_limit_control       ) );
    DBG("T_PU_ANALOG_VIDEO_STANDARD_CONTROL            size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_analog_video_standard_control          ), wolverineIntDirtyOffset(hWolverineInterface, pu_analog_video_standard_control          ) );
    DBG("T_PU_ANALOG_LOCK_STATUS_CONTROL               size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, pu_analog_lock_status_control             ), wolverineIntDirtyOffset(hWolverineInterface, pu_analog_lock_status_control             ) );
    DBG("T_LXU_VIDEO_COLOR_BOOST_CONTROL               size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_video_color_boost_control             ), wolverineIntDirtyOffset(hWolverineInterface, lxu_video_color_boost_control             ) );
    DBG("T_LXU_NATIVE_MODE_CONTROL                     size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_native_mode_control                   ), wolverineIntDirtyOffset(hWolverineInterface, lxu_native_mode_control                   ) );
    DBG("T_LXU_NATIVE_MODE_AUTO_CONTROL                size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_native_mode_auto_control              ), wolverineIntDirtyOffset(hWolverineInterface, lxu_native_mode_auto_control              ) );
    DBG("T_LXU_VIDEO_RIGHTLIGHT_MODE_CONTROL           size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_video_rightlight_mode_control         ), wolverineIntDirtyOffset(hWolverineInterface, lxu_video_rightlight_mode_control         ) );
    DBG("T_LXU_VIDEO_RIGHTLIGHT_ZOI                    size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_video_rightlight_zoi                  ), wolverineIntDirtyOffset(hWolverineInterface, lxu_video_rightlight_zoi                  ) );
    DBG("T_LXU_VIDEO_FW_ZOOM_CONTROL                   size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_video_fw_zoom_control                 ), wolverineIntDirtyOffset(hWolverineInterface, lxu_video_fw_zoom_control                 ) );
    DBG("T_LXU_TEST_TDE_MODE_CONTROL                   size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_test_tde_mode_control                 ), wolverineIntDirtyOffset(hWolverineInterface, lxu_test_tde_mode_control                 ) );
    DBG("T_LXU_TEST_GAIN_CONTROL                       size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_test_gain_control                     ), wolverineIntDirtyOffset(hWolverineInterface, lxu_test_gain_control                     ) );
    DBG("T_LXU_TEST_LOW_LIGHT_PRIORITY_CONTROL         size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_test_low_light_priority_control       ), wolverineIntDirtyOffset(hWolverineInterface, lxu_test_low_light_priority_control       ) );
    DBG("T_LXU_TEST_COLOR_PROCESSING_DISABLE_CONTROL   size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_test_color_processing_disable_control ), wolverineIntDirtyOffset(hWolverineInterface, lxu_test_color_processing_disable_control ) );
    DBG("T_LXU_TEST_PIXEL_DEFECT_CORRECTION_CONTROL    size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_test_pixel_defect_correction_control  ), wolverineIntDirtyOffset(hWolverineInterface, lxu_test_pixel_defect_correction_control  ) );
    DBG("T_LXU_TEST_LENS_SHADING_COMPENSATION_CONTROL  size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_test_lens_shading_compensation_control), wolverineIntDirtyOffset(hWolverineInterface, lxu_test_lens_shading_compensation_control) );
    DBG("T_LXU_TEST_GAMMA_CONTROL                      size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_test_gamma_control                    ), wolverineIntDirtyOffset(hWolverineInterface, lxu_test_gamma_control                    ) );
    DBG("T_LXU_TEST_INTEGRATION_TIME_CONTROL           size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_test_integration_time_control         ), wolverineIntDirtyOffset(hWolverineInterface, lxu_test_integration_time_control         ) );
    DBG("T_LXU_TEST_RAW_DATA_BITS_PER_PIXEL_CONTROL    size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_test_raw_data_bits_per_pixel_control  ), wolverineIntDirtyOffset(hWolverineInterface, lxu_test_raw_data_bits_per_pixel_control  ) );
    DBG("T_LXU_VIDEO_FORMAT_CONTROL                    size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_video_format_control                  ), wolverineIntDirtyOffset(hWolverineInterface, lxu_video_format_control                  ) );
    DBG("T_LXU_RING_TONE_CONTROL                       size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_ring_tone_control                     ), wolverineIntDirtyOffset(hWolverineInterface, lxu_ring_tone_control                     ) );
    DBG("T_LXU_POWER_MODE_CONTROL                      size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_power_mode_control                    ), wolverineIntDirtyOffset(hWolverineInterface, lxu_power_mode_control                    ) );
    DBG("T_LXU_ENTER_DFU_MODE                          size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_enter_dfu_mode                        ), wolverineIntDirtyOffset(hWolverineInterface, lxu_enter_dfu_mode                        ) );
    DBG("T_LXU_DATA_DOWNLOAD_MODE                      size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, lxu_data_download_mode                    ), wolverineIntDirtyOffset(hWolverineInterface, lxu_data_download_mode                    ) );
    DBG("T_CXU_ENCODER_CONFIGURATION_CONTROL           size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, cxu_encoder_configuration_control         ), wolverineIntDirtyOffset(hWolverineInterface, cxu_encoder_configuration_control         ) );
    DBG("T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL       size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, cxu_encoder_configuration_adv_control     ), wolverineIntDirtyOffset(hWolverineInterface, cxu_encoder_configuration_adv_control     ) );

    DBG("    nQuantizerMin                           offset= %d\n", offsetof(T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL, nQuantizerMin          ) );
    DBG("    nQuantizerMax                           offset= %d\n", offsetof(T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL, nQuantizerMax          ) );
    DBG("    nQuantizerFixed                         offset= %d\n", offsetof(T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL, nQuantizerFixed        ) );
    DBG("    nMotionEstimationDepth                  offset= %d\n", offsetof(T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL, nMotionEstimationDepth ) );
    DBG("    nRCMode                                 offset= %d\n", offsetof(T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL, nRCMode                ) );
    DBG("    nReferenceFrames                        offset= %d\n", offsetof(T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL, nReferenceFrames       ) );
    DBG("    nVBVBufSize                             offset= %d\n", offsetof(T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL, nVBVBufSize            ) );
    DBG("    nIntraRefreshFrequency                  offset= %d\n", offsetof(T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL, nIntraRefreshFrequency ) );
    DBG("    nIntraRefreshMethod                     offset= %d\n", offsetof(T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL, nIntraRefreshMethod    ) );
    DBG("    bScenecutDetection                      offset= %d\n", offsetof(T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL, bScenecutDetection     ) );
    DBG("    bEnableLoopFilter                       offset= %d\n", offsetof(T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL, bEnableLoopFilter      ) );
    DBG("    bEnable1x11Transform                    offset= %d\n", offsetof(T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL, bEnable1x11Transform   ) );
    DBG("    bCabac                                  offset= %d\n", offsetof(T_CXU_ENCODER_CONFIGURATION_ADV_CONTROL, bCabac                 ) );

    DBG("T_CXU_FRAME_TYPE_CONTROL                      size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, cxu_frame_type_control                    ), wolverineIntDirtyOffset(hWolverineInterface, cxu_frame_type_control                    ) );
    DBG("T_CXU_BIT_RATE_CONTROL                        size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, cxu_bit_rate_control                      ), wolverineIntDirtyOffset(hWolverineInterface, cxu_bit_rate_control                      ) );
    DBG("T_CXU_FRAME_RATE_CONTROL                      size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, cxu_frame_rate_control                    ), wolverineIntDirtyOffset(hWolverineInterface, cxu_frame_rate_control                    ) );
    DBG("T_CXU_CAMERA_PPS_CONTROL                      size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, cxu_camera_pps_control                    ), wolverineIntDirtyOffset(hWolverineInterface, cxu_camera_pps_control                    ) );
    DBG("T_CXU_CAMERA_SPS_CONTROL                      size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, cxu_camera_sps_control                    ), wolverineIntDirtyOffset(hWolverineInterface, cxu_camera_sps_control                    ) );
    DBG("T_CXU_CAMERA_DELAY                            size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, cxu_camera_delay                          ), wolverineIntDirtyOffset(hWolverineInterface, cxu_camera_delay                          ) );
    DBG("T_CXXU_FILTER_CONTROL                         size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, cxxu_filter_control                       ), wolverineIntDirtyOffset(hWolverineInterface, cxxu_filter_control                       ) );
    DBG("T_CXU_PROFILING_DATA                          size= %d offset= %04X\n", wolverineIntDirtyLength(hWolverineInterface, cxu_profiling_data                        ), wolverineIntDirtyOffset(hWolverineInterface, cxu_profiling_data                        ) );
    DBG("T_WOLVERINE_INTERFACE_VARS                     size= %d\n", sizeof(T_WOLVERINE_INTERFACE_VARS) );
    DBG("T_WOLVERINE_INTERFACE                          size= %d\n", sizeof(T_WOLVERINE_INTERFACE) );
    releaseWolverineIntVars(hWolverineInterface);

    // Test the dirty stuff
    T_WOLVERINE_INTERFACE_VARS_HANDLE hWolverineInterfaceVars = getWolverineIntVars(hWolverineInterface);

    // This first entry should be clean
    if (wolverineIntIsDirty(hWolverineInterface,dm365_status)) {
        printf("dm365_status: Dirty\n");
    } else {
        printf("dm365_status: Clean\n");
    }

    // Mark the entry dirty
    wolverineIntSetDirty(hWolverineInterface,dm365_status);

    // This one should therefor be dirty
    if (wolverineIntIsDirty(hWolverineInterface,dm365_status)) {
        printf("dm365_status: Dirty\n");
    } else {
        printf("dm365_status: Clean\n");
    }

    // Mark the entry clean
    wolverineIntClrDirty(hWolverineInterface,dm365_status);

    // This one should therefor be clean again
    if (wolverineIntIsDirty(hWolverineInterface,dm365_status)) {
        printf("dm365_status: Dirty\n");
    } else {
        printf("dm365_status: Clean\n");
    }

    wolverineIntWrite(0xFFAA0000+ 143,0x55);
    wolverineIntWrite(0xFFAA0000+ 144,0xAA);

    // This one should be dirty
    if (wolverineIntIsDirty(hWolverineInterface,cxu_encoder_configuration_adv_control)) {
        printf("cxu_encoder_configuration_adv_control: Dirty\n");
    } else {
        printf("cxu_encoder_configuration_adv_control: Clean\n");
    }

    // Mark the entry clean
    wolverineIntClrDirty(hWolverineInterface,cxu_encoder_configuration_adv_control);

    // This one should be clean again
    if (wolverineIntIsDirty(hWolverineInterface,cxu_encoder_configuration_adv_control)) {
        printf("cxu_encoder_configuration_adv_control: Dirty\n");
    } else {
        printf("cxu_encoder_configuration_adv_control: Clean\n");
    }

    printf("cxu_encoder_configuration_adv_control.nQuantizerMin          = %d\n", hWolverineInterfaceVars->cxu_encoder_configuration_adv_control.nQuantizerMin         );
    printf("cxu_encoder_configuration_adv_control.nQuantizerMax          = %d\n", hWolverineInterfaceVars->cxu_encoder_configuration_adv_control.nQuantizerMax         );
    printf("cxu_encoder_configuration_adv_control.nQuantizerFixed        = %d\n", hWolverineInterfaceVars->cxu_encoder_configuration_adv_control.nQuantizerFixed       );
    printf("cxu_encoder_configuration_adv_control.nMotionEstimationDepth = %d\n", hWolverineInterfaceVars->cxu_encoder_configuration_adv_control.nMotionEstimationDepth);
    printf("cxu_encoder_configuration_adv_control.nRCMode                = %d\n", hWolverineInterfaceVars->cxu_encoder_configuration_adv_control.nRCMode               );
    printf("cxu_encoder_configuration_adv_control.nReferenceFrames       = %d\n", hWolverineInterfaceVars->cxu_encoder_configuration_adv_control.nReferenceFrames      );
    printf("cxu_encoder_configuration_adv_control.nVBVBufSize            = %u\n", (unsigned int) hWolverineInterfaceVars->cxu_encoder_configuration_adv_control.nVBVBufSize           );
    printf("cxu_encoder_configuration_adv_control.nIntraRefreshFrequency = %u\n", (unsigned int) hWolverineInterfaceVars->cxu_encoder_configuration_adv_control.nIntraRefreshFrequency);
    printf("cxu_encoder_configuration_adv_control.nIntraRefreshMethod    = %u\n", (unsigned int) hWolverineInterfaceVars->cxu_encoder_configuration_adv_control.nIntraRefreshMethod   );
    printf("cxu_encoder_configuration_adv_control.bScenecutDetection     = %d\n", hWolverineInterfaceVars->cxu_encoder_configuration_adv_control.bScenecutDetection    );
    printf("cxu_encoder_configuration_adv_control.bEnableLoopFilter      = %d\n", hWolverineInterfaceVars->cxu_encoder_configuration_adv_control.bEnableLoopFilter     );
    printf("cxu_encoder_configuration_adv_control.bEnable1x11Transform   = %d\n", hWolverineInterfaceVars->cxu_encoder_configuration_adv_control.bEnable1x11Transform  );
    printf("cxu_encoder_configuration_adv_control.bCabac                 = %d\n", hWolverineInterfaceVars->cxu_encoder_configuration_adv_control.bCabac                );

    releaseWolverineIntVars(hWolverineInterface);

    sendWolverineIntStatus(hWolverineInterface,10,100);

    printf("dm365_status[0] = %d\n", wolverineIntRead(0xFFAA0000));
    printf("dm365_status[1] = %d\n", wolverineIntRead(0xFFAA0001));
    printf("dm365_status[2] = %d\n", wolverineIntRead(0xFFAA0002));
    printf("dm365_status[3] = %d\n", wolverineIntRead(0xFFAA0003));
    printf("dm365_status.index = %d\n", hWolverineInterface->vars.dm365_status.index);
    printf("dm365_status.mode  = %d\n", hWolverineInterface->vars.dm365_status.mode);
    printf("hWolverineInterface= 0x%8x\n", hWolverineInterface);

}
#endif // CHECK_STRUCTURES

#if 0
/******************************************************************************
 * wolverineIntSendMessage send a message to a thread
 ******************************************************************************/
Int wolverineIntSendMessage(Fifo_Handle hFifo, hFifo_Msg hMsg)
{
    assert (hFifo);
    assert (hMsg);

    #if 0
        DBG("Sending Msg...\n");
        DBG("  hFifo = %08X\n", (unsigned int) hFifo);
        DBG("  msgId = %d\n", hMsg->msgID);
    #endif

    return Fifo_put(hFifo, (Ptr) hMsg);
}
#endif /* linux version */
