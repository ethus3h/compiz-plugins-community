/**
 *
 * Compiz Nintendo(R) Wii(TM) Remote Interface Plugin
 *
 * Copyright : (C) 2008 by Sam Spilsbury
 * E-mail    : smspillaz@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _COMPIZ_WIIMOTE_H
#define _COMPIZ_WIIMOTE_H
#endif

#include <compiz-core.h>
#include <cwiid.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <string.h>
#include <math.h>

#include <unistd.h>

#define WIIMOTE_ABIVERSION 20080119

#define MAX_GESTURES 32
#define MAX_REPORTS  16
#define MAX_WIIMOTES 10

/* Enums */

typedef enum _WiimoteGestureType{
    WiimoteGestureTypeNone = 0,
    WiimoteGestureTypeAButtonPressed,
    WiimoteGestureTypeAButtonDepressed,
    WiimoteGestureTypeBButtonPressed,
    WiimoteGestureTypeBButtonDepressed,
    WiimoteGestureTypeUpButtonPressed,
    WiimoteGestureTypeUpButtonDepressed,
    WiimoteGestureTypeDownButtonPressed,
    WiimoteGestureTypeDownButtonDepressed,
    WiimoteGestureTypeLeftButtonPressed,
    WiimoteGestureTypeLeftButtonDepressed,
    WiimoteGestureTypeRightButtonPressed,
    WiimoteGestureTypeRightButtonDepressed,
    WiimoteGestureTypePlusButtonPressed,
    WiimoteGestureTypePlusButtonDepressed,
    WiimoteGestureTypeMinusButtonPressed,
    WiimoteGestureTypeMinusButtonDepressed,
    WiimoteGestureTypeHomeButtonPressed,
    WiimoteGestureTypeHomeButtonDepressed,
    WiimoteGestureType1ButtonPressed,
    WiimoteGestureType1ButtonDepressed,
    WiimoteGestureType2ButtonPressed,
    WiimoteGestureType2ButtonDepressed,
    WiimoteGestureTypeCButtonPressed,
    WiimoteGestureTypeCButtonDepressed,
    WiimoteGestureTypeZButtonPressed,
    WiimoteGestureTypeZButtonDepressed,
    WiimoteGestureTypeUp,
    WiimoteGestureTypeDown,
    WiimoteGestureTypeLeft,
    WiimoteGestureTypeRight,
    WiimoteGestureTypeNunchuckUp,
    WiimoteGestureTypeNunchuckDown,
    WiimoteGestureTypeNunchuckLeft,
    WiimoteGestureTypeNunchuckRight,
    WiimoteGestureTypeNunchuckStickUp,
    WiimoteGestureTypeNunchuckStickDown,
    WiimoteGestureTypeNunchuckStickLeft,
    WiimoteGestureTypeNunchuckStickRight,
    WiimoteGestureTypeLength
} WiimoteGestureType;

typedef enum _WiimotePointerType{
    WiimotePointerTypeNone = 0,
    WiimotePointerTypeAcc,
    WiimotePointerTypeAccNunchuck,
    WiimotePointerTypeIR,
    WiimotePointerTypeLength,
} WiimotePointerType;

typedef enum _WiimoteReportType{
    WiimoteReportTypeNone = 0,
    WiimoteReportTypeIR,
    WiimoteReportTypeAccellerometer,
    WiimoteReportTypeNunchuckAccellerometer,
    WiimoteReportTypeNunchuckStick,
    WiimoteReportTypeLength,
} WiimoteReportType;

/* Display
 * -> CompWiimote [ MAX_WIIMOTES ]
 *     -> Attachments
 *        -> Nunchuck
 *           -> Buttons
 *           -> Accellerometer
 *           -> Stick
 *        -> Classic
 *     -> IR
 *     -> Accellerometer
 *
 *     -> Reporting
 *        -> Type
 *        -> Action
 *        -> Plugin 
 *     -> Gestures
 *        -> Type
 *        -> Action
 *        -> Plugin 
 *     -> Connection Info
 */

/* Gesture */

typedef struct _WiimoteGesture{
    Bool set;
    WiimoteGestureType type;
    char *pluginName;
    char *actionName;
    int sensitivity;
} WiimoteGesture;

/* Report */

typedef struct _WiimoteReport
{
    Bool set;
    WiimoteReportType type;
    char *pluginName;
    char *actionName;
    int sensitivity;
    int dataType;

    char *xarg;
    char *yarg;
    char *zarg;
} WiimoteReport;

/* Wiimote Buttons */

typedef struct _WiimoteButtonState{
	Bool A;
	Bool B;
	Bool Up;
	Bool Down;
	Bool Left;
	Bool Right;
	Bool Minus;
	Bool Plus;
	Bool Home;
	Bool One;
	Bool Two;
} WiimoteButtonState;

/* Nunchuck Buttons */

typedef struct _NunchuckButtonState{
	Bool C;
	Bool Z;

	/* The stick is used for reporting */
} NunchuckButtonState;

typedef struct _WiiAcc
{
    float accX;
    float accY;
    float accZ;
    
    /* Used for determining 'angle' */
    
    float accDX;
    float accDY;
    float accDZ;
    
    /* Calibration */
    
    float initAccX;
    float initAccY;
    float initAccZ;

    /* Used for gesture checking */

    float oldAccX;
    float oldAccY;
    float oldAccZ;

} WiiAcc;

typedef struct _WiiIR
{
    Bool valid;
   
    int x;
    int y;
    float size;
} WiiIR;

typedef struct _WiiNunchuck
{
    Bool connected;

    NunchuckButtonState buttons;

    /* Accellerometer Stuff */

    /* Absolute Values */

    float accX;
    float accY;
    float accZ;

    /* Calibration */
    
    float initAccX;
    float initAccY;
    float initAccZ;
    
    /* Angle Values */

    float accDX;
    float accDY;
    float accDZ;

    /* Used for gesture checking */

    float oldAccX;
    float oldAccY;
    float oldAccZ;

    /* Nunchuck Stick */
    /* Calibration */

    float initStickX;
    float initStickY;

    /* Absolute values */

    float stickX;
    float stickY;

    /* Angle values */

    float stickDX;
    float stickDY;

    /* Used for gesture checking */

    float oldStickX;
    float oldStickY;
} WiiNunchuck;

typedef struct _CompWiimote
{
    cwiid_wiimote_t *wiimote; /* Wii Remote Handler */

    int    id;
    
    Bool   connected; /* Initial Bluetooth Connection Phase */
    Bool   initiated; /* Initiate Action Phase (Prevent multiple wiimotes from being connected) */
    Bool   initSet;
    
    /* Value structs */
    
    WiiAcc acc;
    WiiIR  ir[4];
    
    /* Attachments */
    
    WiiNunchuck nunchuck;
    
    /* Gestures */
    
    WiimoteGesture gesture[MAX_GESTURES];
    WiimoteReport  report[MAX_REPORTS];

    /* Buttons */

    WiimoteButtonState buttons;

    int nGesture;
    int nReport;

    /* Pointer Control */
    
    WiimotePointerType pointerType;
    
    /* Positioning Tools */
    
    float irDistance;
    float irMidX;
    float irMidY;
    
    float totalX;
    float totalY;
    
    /* Calibration */
    
    float irMulX;
    float irMulY;
    float irSubX;
    float irSubY;
    
    /* Timeout Handles */
    
    CompTimeoutHandle lightsTimeoutHandle;
    int               count;

    pthread_t connectWiimote; /* Threading Handler */
    struct _CompWiimote *next;

} CompWiimote;

typedef struct _WiimoteBaseFunctions
{
    CompWiimote * (*wiimoteForIter) (CompDisplay *d,
			     	     int         iter);
} WiimoteBaseFunctions;

CompWiimote *
wiimoteForIter (CompDisplay *d,
		int         iter);

/* Shortcut Macros --------------------------------------------------- */

#define toggle_bit(bf,b)	\
	(bf) = ((bf) & b)		\
	       ? ((bf) & ~(b))	\
	       : ((bf) | (b))


#define PI 3.14159265358979323846
#define DEG2RAD2(DEG) ((DEG)*((PI)/(180.0)))
