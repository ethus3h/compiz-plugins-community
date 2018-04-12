/*
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

#include <compiz-core.h>
#include <cwiid.h>

COMPIZ_BEGIN_DECLS

#define WIIMOTE_ABIVERSION 20080119

typedef enum _WiimoteButtonType{
    WiimoteButtonTypeA = 0,
    WiimoteButtonTypeB,
    WiimoteButtonTypeMinus,
    WiimoteButtonTypePlus,
    WiimoteButtonTypeHome,
    WiimoteButtonTypeOne,
    WiimoteButtonTypeTwo,
    WiimoteButtonTypeUp,
    WiimoteButtonTypeDown,
    WiimoteButtonTypeLeft,
    WiimoteButtonTypeRight,
    WiimoteButtonTypeLength
} WiimoteButtonType;

typedef enum _WiimoteGestureType{
    WiimoteGestureTypeUp = 0,
    WiimoteGestureTypeDown,
    WiimoteGestureTypeLeft,
    WiimoteGestureTypeRight,
    WiimoteGestureTypeLength
} WiimoteGestureType;

typedef enum _WiimoteNunchuckGestureType{
    WiimoteNunchuckGestureTypeUp = 0,
    WiimoteNunchuckGestureTypeDown,
    WiimoteNunchuckGestureTypeLeft,
    WiimoteNunchuckGestureTypeRight,
    WiimoteNunchuckGestureTypeLength
} WiimoteNunchuckGestureType;

typedef enum _WiimotePointerType{
    WiimotePointerTypeNone = 0,
    WiimotePointerTypeAcc,
    WiimotePointerTypeAccNunchuck,
    WiimotePointerTypeIR,
    WiimotePointerTypeLength,
} WiimotePointerType;

typedef struct _WiimoteButton{
    WiimoteButtonType type;
    char pluginName;
    char actionName;
} WiimoteButton;

typedef struct _WiimoteGesture{
    WiimoteGestureType type;
    char pluginName;
    char actionName;
} WiimoteGesture;

typedef struct _WiimoteNunchuckGesture{
    WiimoteNunchuckGestureType type;
    char pluginName;
    char actionName;
} WiimoteNunchuckGesture;

typedef struct _WiimoteCore {
    int data;
} WiimoteCore;

typedef struct _WiiNunchuck
{
    Bool connected;

    /* Accellerometer Stuff */

    float accX;
    float accY;
    float accZ;
    
    float initAccX;
    float initAccY;
    float initAccZ;
    
    float accDX;
    float accDY;
    float accDZ;
} WiiNunchuck;

typedef struct _WiiAcc
{
    float accX;
    float accY;
    float accZ;
    
    /* Used for determining 'angle' */
    
    float oldAccX;
    float oldAccY;
    float oldAccZ;
    
    float accDX;
    float accDY;
    float accDZ;
    
    /* Calibration */
    
    float initAccX;
    float initAccY;
    float initAccZ;
    
    float accAngX;
    float accAngY;
    float accAngZ;

} WiiAcc;

typedef struct _WiiIR
{
    Bool valid;
   
    int x;
    int y;
    float size;
} WiiIR;    
    

typedef struct _CompWiimote
{
    cwiid_wiimote_t *wiimote; /* Wii Remote Handler */
    
    Bool   connected; /* Initial Bluetooth Connection Phase */
    Bool   initiated; /* Initiate Action Phase (Prevent multiple wiimotes from being connected) */
    Bool   initSet;
    
    /* Value structs */
    
    WiiAcc acc;
    WiiIR  ir[3];
    
    /* Attachments */
    
    WiiNunchuck nunchuck;
    
    /* Gestures */
    
    WiimoteButton button[10];
    WiimoteGesture gesture[3];
    WiimoteNunchuckGesture nunchuckGesture[3];
    
    /* Pointer Control */
    
    WiimotePointerType pointerType;
    
    /* Determine what the Wiimote is allowed to do */
    
    Bool   canReport;
    Bool   canAccGest;
    Bool   canAccNunchuckGest;
    Bool   canButtonGest;
    
    /* Action callback handlers */
    
    char *accPluginName;
    char *accActionName;
    
    char *nunchuckPluginName;
    char *nunchuckActionName;
    
    char *irPluginName;
    char *irActionName;
    
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
} CompWiimote;

typedef struct _WiimoteDisplay
{
    int screenPrivateIndex;
    
    float accX;
    float accY;
    float accZ;
    
    Bool   connected; /* Initial Bluetooth Connection Phase */
    Bool   initiated; /* Initiate Action Phase */
    Bool   initSet;
    
    /* Used for determining 'angle' */
    
    float oldAccX;
    float oldAccY;
    float oldAccZ;
    
    float accDX;
    float accDY;
    float accDZ;
    
    /* Calibration */
    
    float initAccX;
    float initAccY;
    float initAccZ;
    
    float accAngX;
    float accAngY;
    float accAngZ;
    
    int ir[3][1]; // Four points, 2 values for each point
    int irSize[3];
    float irDistance;
    float irMidX;
    float irMidY;
    
    //int sortedIRX[3];
    
    Bool isIR[4];
    
    int count; // Oh, it's just for a nice little thing
    
    cwiid_wiimote_t *wiimote; /* Wii Remote Handler */
    
    CompWiimote cWiimote[3];
    int nWiimote;
    
    /* Hack */
    
    Bool CallbackSet;
    
    pthread_t connectWiimote; /* Threading Handler */

    HandleEventProc handleEvent;
    CompTimeoutHandle infoTimeoutHandle;
    CompTimeoutHandle checkingTimeoutHandle;
    
} WiimoteDisplay;

typedef struct _WiimoteScreen
{
    PaintOutputProc        paintOutput;
    int windowPrivateIndex;
    /* text display support */
    CompTexture textTexture;
    Pixmap      textPixmap;
    int       textWidth;
    int       textHeight;
    
    Bool title;

} WiimoteScreen;

cwiid_mesg_callback_t cwiid_callback0;
cwiid_mesg_callback_t cwiid_callback1;
cwiid_mesg_callback_t cwiid_callback2;
cwiid_mesg_callback_t cwiid_callback3;
static int wiimoteDisplayPrivateIndex;
CompDisplay *firstDisplay;
cwiid_err_t err;

//static int findMin (CompDisplay *, int, int, int, int); // No you can't have fminf
//static int findMax (CompDisplay *, int, int, int, int); // Or fmaxf

#define GET_WIIMOTE_CORE(c) \
    ((WiimoteCore *) (c)->base.privates[corePrivateIndex].ptr)
#define WIIMOTE_CORE(c) \
    WiimoteCore *wc = GET_WIIMOTE_CORE (c)
#define GET_WIIMOTE_DISPLAY(d)                            \
    ((WiimoteDisplay *) (d)->base.privates[wiimoteDisplayPrivateIndex].ptr)
#define WIIMOTE_DISPLAY(d)                                \
    WiimoteDisplay *ad = GET_WIIMOTE_DISPLAY (d)
#define GET_WIIMOTE_SCREEN(s, ad)                         \
    ((WiimoteScreen *) (s)->base.privates[(ad)->screenPrivateIndex].ptr)
#define WIIMOTE_SCREEN(s)                                 \
    WiimoteScreen *as = GET_WIIMOTE_SCREEN (s, GET_WIIMOTE_DISPLAY (s->display))
#define GET_WIIMOTE_WINDOW(w, as) \
    ((WiimoteWindow *) (w)->base.privates[ (as)->windowPrivateIndex].ptr)
#define WIIMOTE_WINDOW(w) \
    WiimoteWindow *aw = GET_WIIMOTE_WINDOW (w,          \
			  GET_WIIMOTE_SCREEN  (w->screen, \
			  GET_WIIMOTE_DISPLAY (w->screen->display)))

COMPIZ_END_DECLS

#endif
