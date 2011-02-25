/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU Lesser General
 * Public License.  You may obtain a copy of the GNU Lesser General
 * Public License Version 2.1 or later at the following locations:
 *
 * http://www.opensource.org/licenses/lgpl-license.html
 * http://www.gnu.org/copyleft/lgpl.html
 */
#ifndef __ASM_ARCH_MXC_PMIC_STATUS_H__
#define __ASM_ARCH_MXC_PMIC_STATUS_H__
#include <asm-generic/errno-base.h>
/*!
 * @file arch-mxc/pmic_status.h
 * @brief PMIC APIs return code definition.
 *
 * @ingroup PMIC_CORE
 */

/*!
 * Define bool type if it is not defined.
 */
#ifndef bool
#define bool int
#endif

/*!
 * @enum PMIC_STATUS
 * @brief Define return values for all PMIC APIs.
 *
 * These return values are used by all of the PMIC APIs.
 *
 * @ingroup PMIC
 */
typedef enum {
	PMIC_SUCCESS = 0,	/*!< The requested operation was successfully
				   completed.                                     */
	PMIC_ERROR = -1,	/*!< The requested operation could not be completed
				   due to an error.                               */
	PMIC_PARAMETER_ERROR = -2,	/*!< The requested operation failed because
					   one or more of the parameters was
					   invalid.                             */
	PMIC_NOT_SUPPORTED = -3,	/*!< The requested operation could not be
					   completed because the PMIC hardware
					   does not support it. */
	PMIC_SYSTEM_ERROR_EINTR = -EINTR,

	PMIC_MALLOC_ERROR = -5,	/*!< Error in malloc function             */
	PMIC_UNSUBSCRIBE_ERROR = -6,	/*!< Error in un-subscribe event          */
	PMIC_EVENT_NOT_SUBSCRIBED = -7,	/*!< Event occur and not subscribed       */
	PMIC_EVENT_CALL_BACK = -8,	/*!< Error - bad call back                */
	PMIC_CLIENT_NBOVERFLOW = -9,	/*!< The requested operation could not be
					   completed because there are too many
					   PMIC client requests */
} PMIC_STATUS;

#endif				/* __ASM_ARCH_MXC_PMIC_STATUS_H__ */
