/*
    lm75b.h - Part of lm_sensors, Linux kernel modules

    Copyright (C) Atmark Techno, Inc. All Rights Reserved.

    Based on:
    Copyright (c) 2003 Mark M. Hoffman <mhoffman@lightlink.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
    This file contains common code for encoding/decoding LM75B type
    temperature readings, which are emulated by many of the chips
    we support.  As the user is unlikely to load more than one driver
    which contains this code, we don't worry about the wasted space.
*/

/* straight from the datasheet */
#define LM75B_VALUE_MIN (-55000)
#define LM75B_VALUE_MAX (125000)

/*
 * Divide positive or negative dividend by positive divisor and round
 * to closest integer. Result is undefined for negative divisors and
 * for negative dividends if the divisor variable type is unsigned.
 */
#define DIV_ROUND_CLOSEST(x, divisor)(			\
{							\
	typeof(x) __x = x;				\
	typeof(divisor) __d = divisor;			\
	(((typeof(x))-1) > 0 ||				\
	 ((typeof(divisor))-1) > 0 || (__x) > 0) ?	\
		(((__x) + ((__d) / 2)) / (__d)) :	\
		(((__x) - ((__d) / 2)) / (__d));	\
}							\
)

/* Scale user input to sensible values */
static inline int SENSORS_LIMIT(long value, long low, long high)
{
	if (value < low)
		return low;
	else if (value > high)
		return high;
	else
		return value;
}

/* TEMP: 0.125C/bit (-55C to +125C) << 5
   REG : 0.500C/bit (-55C to +125C) << 7 */
static inline u16 LM75B_MC_TO_REG(long temp, int step_mc, int reg_shift)
{
	int ntemp = SENSORS_LIMIT(temp, LM75B_VALUE_MIN, LM75B_VALUE_MAX);
	return (u16)(DIV_ROUND_CLOSEST(ntemp, step_mc) << reg_shift);
}

static inline int LM75B_MC_FROM_REG(u16 reg, int step_mc, int reg_shift)
{
	/* use integer division instead of equivalent right shift to
	   guarantee arithmetic shift and preserve the sign */
	return ((s16)reg / (1<<reg_shift)) * step_mc;
}

#define LM75B_TEMP_STEP_MC	(125)
#define LM75B_TEMP_REG_SHIFT	(5)
#define LM75B_TEMP_TO_REG(t)	LM75B_MC_TO_REG(t, LM75B_TEMP_STEP_MC, LM75B_TEMP_REG_SHIFT)
#define LM75B_TEMP_FROM_REG(t)	LM75B_MC_FROM_REG(t, LM75B_TEMP_STEP_MC, LM75B_TEMP_REG_SHIFT)

#define LM75B_TOS_STEP_MC	(500)
#define LM75B_TOS_REG_SHIFT	(7)
#define LM75B_TOS_TO_REG(t)	LM75B_MC_TO_REG(t, LM75B_TOS_STEP_MC, LM75B_TOS_REG_SHIFT)
#define LM75B_TOS_FROM_REG(t)	LM75B_MC_FROM_REG(t, LM75B_TOS_STEP_MC, LM75B_TOS_REG_SHIFT)

#define LM75B_THYST_STEP_MC	LM75B_TOS_STEP_MC
#define LM75B_THYST_REG_SHIFT	LM75B_TOS_REG_SHIFT
#define LM75B_THYST_TO_REG(t)	LM75B_TOS_TO_REG(t)
#define LM75B_THYST_FROM_REG(t)	LM75B_TOS_FROM_REG(t)

