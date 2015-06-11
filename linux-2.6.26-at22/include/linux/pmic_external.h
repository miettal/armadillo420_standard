/*
 * Copyright 2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef __ASM_ARCH_MXC_PMIC_EXTERNAL_H__
#define __ASM_ARCH_MXC_PMIC_EXTERNAL_H__

#ifdef __KERNEL__
#include <linux/list.h>
#endif

/*!
 * @defgroup PMIC_DRVRS PMIC Drivers
 */

/*!
 * @defgroup PMIC_CORE PMIC Protocol Drivers
 * @ingroup PMIC_DRVRS
 */

/*!
 * @file arch-mxc/pmic_external.h
 * @brief This file contains interface of PMIC protocol driver.
 *
 * @ingroup PMIC_CORE
 */

#include <linux/pmic_status.h>
#include <asm/ioctl.h>

/*!
 * This is the enumeration of versions of PMIC
 */
typedef enum {
	PMIC_MC34704 = 1,
} pmic_id_t;

/*!
 * @struct pmic_version_t
 * @brief PMIC version and revision
 */
typedef struct {
	/*!
	 * PMIC version identifier.
	 */
	pmic_id_t id;
	/*!
	 * Revision of the PMIC.
	 */
	int revision;
} pmic_version_t;

/*!
 * struct pmic_event_callback_t
 * @brief This structure contains callback function pointer and its
 * parameter to be used when un/registering and launching a callback
 * for an event.
 */
typedef struct {
	/*!
	 * call back function
	 */
	void (*func) (void *);

	/*!
	 * call back function parameter
	 */
	void *param;
} pmic_event_callback_t;

/*!
 * This structure is used with IOCTL.
 * It defines register, register value, register mask and event number
 */
typedef struct {
	/*!
	 * register number
	 */
	int reg;
	/*!
	 * value of register
	 */
	unsigned int reg_value;
	/*!
	 * mask of bits, only used with PMIC_WRITE_REG
	 */
	unsigned int reg_mask;
} register_info;

/*!
 * @name IOCTL definitions for sc55112 core driver
 */
/*! @{ */
/*! Read a PMIC register */
#define PMIC_READ_REG          _IOWR('P', 0xa0, register_info*)
/*! Write a PMIC register */
#define PMIC_WRITE_REG         _IOWR('P', 0xa1, register_info*)
/*! Subscribe a PMIC interrupt event */
#define PMIC_SUBSCRIBE         _IOR('P', 0xa2, int)
/*! Unsubscribe a PMIC interrupt event */
#define PMIC_UNSUBSCRIBE       _IOR('P', 0xa3, int)
/*! Subscribe a PMIC event for user notification*/
#define PMIC_NOTIFY_USER       _IOR('P', 0xa4, int)
/*! Get the PMIC event occured for which user recieved notification */
#define PMIC_GET_NOTIFY	       _IOW('P', 0xa5, int)
/*! @} */

/*!
 * This is PMIC registers valid bits
 */
#define PMIC_ALL_BITS           0xFFFFFF
#define PMIC_MAX_EVENTS		48

#define PMIC_ARBITRATION	"NULL"

#if defined(CONFIG_MXC_PMIC_MC34704) || defined(CONFIG_MXC_PMIC_MC34704_MODULE)

typedef enum {
	/* register names for mc34704 */
	REG_MC34704_GENERAL1 = 0x01,
	REG_MC34704_GENERAL2 = 0x02,
	REG_MC34704_GENERAL3 = 0x03,
	REG_MC34704_VGSET1 = 0x04,
	REG_MC34704_VGSET2 = 0x05,
	REG_MC34704_REG2SET1 = 0x06,
	REG_MC34704_REG2SET2 = 0x07,
	REG_MC34704_REG3SET1 = 0x08,
	REG_MC34704_REG3SET2 = 0x09,
	REG_MC34704_REG4SET1 = 0x0A,
	REG_MC34704_REG4SET2 = 0x0B,
	REG_MC34704_REG5SET1 = 0x0C,
	REG_MC34704_REG5SET2 = 0x0D,
	REG_MC34704_REG5SET3 = 0x0E,
	REG_MC34704_REG6SET1 = 0x0F,
	REG_MC34704_REG6SET2 = 0x10,
	REG_MC34704_REG6SET3 = 0x11,
	REG_MC34704_REG7SET1 = 0x12,
	REG_MC34704_REG7SET2 = 0x13,
	REG_MC34704_REG7SET3 = 0x14,
	REG_MC34704_REG8SET1 = 0x15,
	REG_MC34704_REG8SET2 = 0x16,
	REG_MC34704_REG8SET3 = 0x17,
	REG_MC34704_FAULTS = 0x18,
	REG_MC34704_I2CSET1 = 0x19,
	REG_MC34704_REG3DAC = 0x49,
	REG_MC34704_REG7CR0 = 0x58,
	REG_MC34704_REG7DAC = 0x59,
	REG_NB = 0x60,
} pmic_reg;

typedef enum {
	/* events for mc34704 */
	EVENT_FLT1 = 0,
	EVENT_FLT2,
	EVENT_FLT3,
	EVENT_FLT4,
	EVENT_FLT5,
	EVENT_FLT6,
	EVENT_FLT7,
	EVENT_FLT8,
	EVENT_NB,
} type_event;

typedef enum {
	MCU_SENSOR_NOT_SUPPORT
} t_sensor;

typedef enum {
	MCU_SENSOR_BIT_NOT_SUPPORT
} t_sensor_bits;

#endif /* defined(CONFIG_MXC_PMIC_MC34704) || defined(CONFIG_MXC_PMIC_MC34704_MODULE) */

/* EXPORTED FUNCTIONS */
#ifdef __KERNEL__

#if defined(CONFIG_MXC_PMIC)
/*!
 * This function is used to determine the PMIC type and its revision.
 *
 * @return      Returns the PMIC type and its revision.
 */
pmic_version_t pmic_get_version(void);

/*!
 * This function is called by PMIC clients to read a register on PMIC.
 *
 * @param        priority   priority of access
 * @param        reg        number of register
 * @param        reg_value   return value of register
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_read_reg(int reg, unsigned int *reg_value,
			  unsigned int reg_mask);
/*!
 * This function is called by PMIC clients to write a register on MC13783.
 *
 * @param        priority   priority of access
 * @param        reg        number of register
 * @param        reg_value  New value of register
 * @param        reg_mask   Bitmap mask indicating which bits to modify
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_write_reg(int reg, unsigned int reg_value,
			   unsigned int reg_mask);

/*!
 * This function is called by PMIC clients to subscribe on an event.
 *
 * @param        event_sub   structure of event, it contains type of event and callback
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_event_subscribe(type_event event,
				 pmic_event_callback_t callback);
/*!
* This function is called by PMIC clients to un-subscribe on an event.
*
* @param        event_unsub   structure of event, it contains type of event and callback
*
* @return       This function returns PMIC_SUCCESS if successful.
*/
PMIC_STATUS pmic_event_unsubscribe(type_event event,
				   pmic_event_callback_t callback);
PMIC_STATUS pmic_event_unsubscribe(type_event event,
				   pmic_event_callback_t callback);
/*!
* This function is called to read all sensor bits of PMIC.
*
* @param        sensor    Sensor to be checked.
*
* @return       This function returns true if the sensor bit is high;
*               or returns false if the sensor bit is low.
*/
bool pmic_check_sensor(t_sensor sensor);

/*!
* This function checks one sensor of PMIC.
*
* @param        sensor_bits  structure of all sensor bits.
*
* @return       This function returns PMIC_SUCCESS if successful.
*/
PMIC_STATUS pmic_get_sensors(t_sensor_bits * sensor_bits);

void pmic_event_callback(type_event event);
void pmic_event_list_init(void);

#ifdef CONFIG_REGULATOR_MC34704
int reg_mc34704_probe(void);
#endif

#endif				/*CONFIG_MXC_PMIC*/
#endif				/* __KERNEL__ */

#endif				/* __ASM_ARCH_MXC_PMIC_EXTERNAL_H__ */
