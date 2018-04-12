/*
 * Compiz Fusion Head Tracking Plugin
 * 
 * Some of this code is based on Freewins
 * Portions were inspired and tested on a modified
 * Zoom plugin, but no code from Zoom has been taken.
 * 
 * Copyright 2010 Kevin Lange <kevin.lange@phpwnage.com>
 *
 * facedetect.c is from the OpenCV sample library, modified to run
 * threaded.
 *
 * Face detection is done through OpenCV.
 * Wiimote tracking is done through the `wiimote` plugin and probably
 * doesn't work anymore.
 *
 * Video demonstrations of both webcams and wiimotes are available
 * online. Check YouTube, as well as the C-F forums.
 *
 * More information is available in README.
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

#include <compiz-core.h>
#include <compiz-cube.h>

#define USE_WIIMOTE FALSE // Change as necessary

#if USE_WIIMOTE
#include "compiz-wiimote.h" // FIXME - Blame Sam.
#endif

#include <math.h>
#include <stdio.h>

#include <X11/extensions/shape.h>
#include "headtracking_options.h"
#include "facedetect.h"

#define PI 3.14159265358979323846
#define ADEG2RAD(DEG) ((DEG)*((PI)/(180.0)))

// Macros/*{{{*/
#define GET_HEADTRACKING_DISPLAY(d)                                       \
    ((WTDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define HEADTRACKING_DISPLAY(d)                      \
    WTDisplay *wtd = GET_HEADTRACKING_DISPLAY (d)

#define GET_HEADTRACKING_SCREEN(s, wtd)                                        \
    ((WTScreen *) (s)->base.privates[(wtd)->screenPrivateIndex].ptr)

#define HEADTRACKING_SCREEN(s)                                                      \
    WTScreen *wts = GET_HEADTRACKING_SCREEN (s, GET_HEADTRACKING_DISPLAY (s->display))

#define GET_HEADTRACKING_WINDOW(w, wts)                                        \
    ((WTWindow *) (w)->base.privates[(wts)->windowPrivateIndex].ptr)

#define HEADTRACKING_WINDOW(w)                                         \
    WTWindow *wtw = GET_HEADTRACKING_WINDOW  (w,                    \
                       GET_HEADTRACKING_SCREEN  (w->screen,            \
                       GET_HEADTRACKING_DISPLAY (w->screen->display)))


#define WIN_REAL_X(w) (w->attrib.x - w->input.left)
#define WIN_REAL_Y(w) (w->attrib.y - w->input.top)

#define WIN_REAL_W(w) (w->width + w->input.left + w->input.right)
#define WIN_REAL_H(w) (w->height + w->input.top + w->input.bottom)

/*}}}*/

typedef struct _WTDisplay{
    int screenPrivateIndex;

    HandleEventProc handleEvent;

    CompWindow *grabWindow;
    CompWindow *focusWindow;

} WTDisplay;

typedef struct _WTHead {
	float x;
	float y;
	float z;
} WTHead;

typedef struct _WTScreen {
    PreparePaintScreenProc	 preparePaintScreen;
    PaintOutputProc		 paintOutput;
    PaintWindowProc 		 paintWindow;
    CompTimeoutHandle mouseIntervalTimeoutHandle;
    int mouseX;
    int mouseY;
    int grabIndex;
    int rotatedWindows;
    int windowPrivateIndex;
    Bool trackMouse;
    
    DamageWindowRectProc damageWindowRect;
    WTHead head;
} WTScreen;

typedef struct _WTWindow{
    float depth;
    float manualDepth;
    float zDepth;
    float oldDepth;
    float newDepth;
    int timeRemaining; // 100 to 0
    int zIndex;
    Bool isAnimating;
    Bool isManualDepth;
    Bool grabbed;
} WTWindow;

int displayPrivateIndex;
static CompMetadata headtrackingMetadata;
static int cubeDisplayPrivateIndex = -1;

static void WTHandleEvent(CompDisplay *d, XEvent *ev){

    HEADTRACKING_DISPLAY(d);
    UNWRAP(wtd, d, handleEvent);
    (*d->handleEvent)(d, ev);
    WRAP(wtd, d, handleEvent, WTHandleEvent);
    
}

static Bool
windowIs3D (CompWindow *w)
{
    // Is this window one we need to consider
    // for automatic z depth and 3d positioning?
    
    if (w->attrib.override_redirect)
	return FALSE;

    if (!(w->shaded || w->attrib.map_state == IsViewable))
	return FALSE;

    if (w->state & (CompWindowStateSkipPagerMask |
		    CompWindowStateSkipTaskbarMask))
	return FALSE;
	
    if (w->state & (CompWindowStateStickyMask))
    	return FALSE;

    if (w->type & (NO_FOCUS_MASK))
    	return FALSE;
    return TRUE;
}

static void WTPreparePaintScreen (CompScreen *s, int msSinceLastPaint) {

    CompWindow *w;
    HEADTRACKING_SCREEN (s);
    int maxDepth = 0;
    
    // First establish our maximum depth
    for (w = s->windows; w; w = w->next)
    {
	    HEADTRACKING_WINDOW (w);
	    if (!(WIN_REAL_X(w) + WIN_REAL_W(w) <= 0.0 || WIN_REAL_X(w) >= w->screen->width)) {
	        if (!(wtw->isManualDepth)) {
	            if (!windowIs3D (w))
		            continue;
	            maxDepth++;
	        }
	    }
    }
    
    // Then set our windows as such
    for (w = s->windows; w; w = w->next)
    {
	    HEADTRACKING_WINDOW (w);
	    if (!(wtw->isManualDepth) && wtw->zDepth > 0.0) {
	        wtw->zDepth = 0.0;
	    }
	    if (!(WIN_REAL_X(w) + WIN_REAL_W(w) <= 0.0 || WIN_REAL_X(w) >= w->screen->width)) {
	        if (!(wtw->isManualDepth)) {
	            if (!windowIs3D (w))
		            continue;
	            maxDepth--;
	            float tempDepth = 0.0f - ((float)maxDepth * (float)headtrackingGetWindowDepth(s) / 100.0);
	            if (!wtw->isAnimating) {
	                wtw->newDepth = tempDepth;
	                if (wtw->zDepth != wtw->newDepth) {
	                    wtw->oldDepth = wtw->zDepth;
	                    wtw->isAnimating = TRUE;
	                    wtw->timeRemaining = 0;
	                }
	            } else {
	                if (wtw->newDepth != tempDepth) {
	                    wtw->newDepth = tempDepth;
	                    wtw->oldDepth = wtw->zDepth;
	                    wtw->isAnimating = TRUE;
	                    wtw->timeRemaining = 0;
	                } else {
                        wtw->timeRemaining++;
                        float dz = (float)(wtw->oldDepth - wtw->newDepth) * (float)wtw->timeRemaining / headtrackingGetFadeTime (s);
                        wtw->zDepth = wtw->oldDepth - dz;
                        if (wtw->timeRemaining >= headtrackingGetFadeTime (s)) {
                            wtw->isAnimating = FALSE;
                            wtw->zDepth = wtw->newDepth;
                            wtw->oldDepth = wtw->newDepth;
                        }
                    }
	            }
	        } else {
		        wtw->zDepth = wtw->depth;
	        }
	    } else {
	        wtw->zDepth = 0.0f;
	    }
    }
    
    UNWRAP (wts, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, msSinceLastPaint);
    WRAP (wts, s, preparePaintScreen, WTPreparePaintScreen);
}


static Bool shouldPaintStacked(CompWindow *w) {
    
    // Should we draw the windows or not?
    // TODO: Add more checks, ie, Expo, Scale
    // XXX: You'll get your checks in 0.9.0. Don't bug me now.
    
    if (cubeDisplayPrivateIndex >= 0) {
        // Cube is enabled, is it rotating?
        CUBE_SCREEN(w->screen);
        if (cs->rotationState != RotationNone || otherScreenGrabExist (w->screen, "headtracking", "move", "resize", 0))
    	    return FALSE;
    }
    return TRUE;
}


static Bool WTPaintWindow(CompWindow *w, const WindowPaintAttrib *attrib, 
	const CompTransform *transform, Region region, unsigned int mask){
	
	// Draw the window with its correct z level
    CompTransform wTransform = *transform;
    Bool status;
    Bool wasCulled = glIsEnabled(GL_CULL_FACE);
    
    HEADTRACKING_SCREEN(w->screen);
    HEADTRACKING_WINDOW(w);
    
    if (shouldPaintStacked(w)) {
        if (!(w->type == CompWindowTypeDesktopMask)) {
            if (!(WIN_REAL_X(w) + WIN_REAL_W(w) <= 0.0 || WIN_REAL_X(w) >= w->screen->width))
        	    matrixTranslate(&wTransform, 0.0, 0.0, wtw->zDepth + (float)headtrackingGetStackPadding(w->screen) / 100.0f);
        	if (wtw->zDepth + (float)headtrackingGetStackPadding(w->screen) / 100.0f != 0.0)
        	    mask |= PAINT_WINDOW_TRANSFORMED_MASK;
        } else {
        	matrixTranslate(&wTransform, 0.0, 0.0, 0.0);
        }
    }
    
    damageScreen(w->screen);
    
    if (wasCulled)
        glDisable(GL_CULL_FACE);
    
    UNWRAP(wts, w->screen, paintWindow);
    status = (*w->screen->paintWindow)(w, attrib, &wTransform, region, mask);
    WRAP(wts, w->screen, paintWindow, WTPaintWindow);
    
    if(wasCulled)
	    glEnable(GL_CULL_FACE);

    return status;
}

// Track head position with Johnny Chung Lee's trig stuff
// XXX: Note that positions should be float values from 0-1024
//      and 0-720 (width, height, respectively).
static void WTLeeTrackPosition (CompScreen *s, float x1, float y1,
                                float x2, float y2) {
                                
    HEADTRACKING_SCREEN (s);
    float radPerPix = (PI / 3.0f) / 1024.0f;
    // Where is the middle of the head?
    float dx = x1 - x2, dy = y1 - y2;
    float pointDist = (float)sqrt(dx * dx + dy * dy);
    float angle = radPerPix * pointDist / 2.0;
    // Set the head distance in units of screen size
    wts->head.z = ((float)headtrackingGetBarWidth (s) / 1000.0) / (float)tan(angle);
    float aX = (x1 + x2) / 2.0f, aY = (y1 + y2) / 2.0f;
    // Set the head position horizontally
    wts->head.x = (float)sin(radPerPix * (aX - 512.0)) * wts->head.z;
    float relAng = (aY - 384.0) * radPerPix;
    // Set the head height
    wts->head.y = -0.5f + (float)sin((float)headtrackingGetWiimoteVerticalAngle (s) / 100.0 + relAng) * wts->head.z;
    // And adjust it to suit our needs
    wts->head.y = wts->head.y + (float)headtrackingGetWiimoteAdjust (s) / 100.0;
    // And if our Wiimote is above our screen, adjust appropriately
    if (headtrackingGetWiimoteAbove (s))
        wts->head.y = wts->head.y + 0.5;
}

static Bool WTPaintOutput(CompScreen *s, const ScreenPaintAttrib *sAttrib, 
	const CompTransform *transform, Region region, CompOutput *output, unsigned int mask){
	
	Bool status;
	HEADTRACKING_SCREEN(s);
	CompTransform zTransform = *transform;
	mask |= PAINT_SCREEN_CLEAR_MASK;
#if USE_WIIMOTE
	if (headtrackingGetEnableTracking (s)) {
	    // Headtracking is enabled in options, so let's track your head...
	    // Grab data from the Wiimote and run it through our tracking algorithm
	    WIIMOTE_DISPLAY (s->display);
	    if (ad->cWiimote[0].ir[0].valid && ad->cWiimote[0].ir[1].valid) {
	        WTLeeTrackPosition (s, (float)ad->cWiimote[0].ir[0].x, (float)ad->cWiimote[0].ir[0].y,
	                           (float)ad->cWiimote[0].ir[1].x, (float)ad->cWiimote[0].ir[1].y);
            }
	}
#endif
	if (headtrackingGetEnableCamtracking (s)) {
	    int x1, y1, x2, y2;
        if (headtrack(&x1, &y1, &x2, &y2, headtrackingGetWebcamLissage(s), headtrackingGetWebcamSmooth(s), 0, headtrackingGetScale(s))) {
            WTLeeTrackPosition(s, (float)x1, (float)y1, (float)x2, (float)y2);
            wts->head.z = (float)headtrackingGetDepthAdjust (s) / 100.0;
        }
    }
    
    if (wts->trackMouse) {
	    // Mouse-Tracking is enabled, and overides true tracking.
	    // FIXME: Replace this with MousePoll calls {{{
	    Window root_return;
	    Window child_return;
	    int rootX, rootY;
	    int winX, winY;
	    unsigned int maskReturn;
	    XQueryPointer (s->display->display, s->root,
		       &root_return, &child_return,
		       &rootX, &rootY, &winX, &winY, &maskReturn);
		// }}} End MousePoll options
		// The following lets us scale mouse movement to get beyond the screen
	    float mult = 100.0 / ((float)headtrackingGetScreenPercent(s));
	    wts->head.x = (-(float)rootX / (float)s->width + 0.5) * mult;
	    wts->head.y = ((float)rootY / (float)s->height - 0.5) * mult;
	    wts->head.z = (float)1.0;
	}
	float nearPlane = 0.05; // Our near rendering plane
	float screenAspect = 1.0; // Leave this for future fixes
	glMatrixMode(GL_PROJECTION); // We're going to modify the projection matrix
    glLoadIdentity(); // Clear it first
    // Set our frustum view matrix.
	glFrustum(	nearPlane*(-0.5 * screenAspect + wts->head.x)/wts->head.z,
				nearPlane*(0.5 * screenAspect + wts->head.x)/wts->head.z,
				nearPlane*(-0.5 + wts->head.y)/wts->head.z,
				nearPlane*(0.5 + wts->head.y)/wts->head.z,
				nearPlane, 100.0);
	
	glMatrixMode(GL_MODELVIEW); // Switch back to model matrix
	// Move our scene so that it appears to actually be the screen
	matrixTranslate (&zTransform, 
			 wts->head.x,
			 wts->head.y,
			 DEFAULT_Z_CAMERA - wts->head.z);
    // Update: Do not set the mask, it's not needed and blurs desktop parts that don't need to be.
	// And wrap us into the paintOutput set
	UNWRAP (wts, s, paintOutput);
	status = (*s->paintOutput) (s, sAttrib, &zTransform, region, output, mask);
	WRAP (wts, s, paintOutput, WTPaintOutput);
	return status;
}

// Fairly standard stuff, I don't even mess with anything here...
static Bool WTDamageWindowRect(CompWindow *w, Bool initial, BoxPtr rect){

    Bool status = TRUE;
    HEADTRACKING_SCREEN(w->screen);
    if (!initial) {
	REGION region;
	region.rects = &region.extents;
	region.numRects = region.size = 1;
	region.extents.x1 = w->serverX;
	region.extents.y1 = w->serverY;
	region.extents.x2 = w->serverX + w->serverWidth;
	region.extents.y2 = w->serverY + w->serverHeight;
	damageScreenRegion (w->screen, &region);
	return TRUE;
    }
    UNWRAP(wts, w->screen, damageWindowRect);
    status = (*w->screen->damageWindowRect)(w, initial, rect);
    WRAP(wts, w->screen, damageWindowRect, WTDamageWindowRect);
    damagePendingOnScreen (w->screen);
    if (initial) {
    	damagePendingOnScreen(w->screen);
    }
    return status;
}

///  Manual Movement Keybindings

// Move window away from viewer
static Bool WTManualMoveAway (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    CompWindow* w;
    Window xid;
    
    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    HEADTRACKING_WINDOW(w);
    wtw->isManualDepth = TRUE;
    wtw->depth = wtw->depth - (float)headtrackingGetWindowDepth (w->screen) / 100.0;
    damagePendingOnScreen (w->screen);
    return TRUE;
}

// Move window closer to viewer
static Bool WTManualMoveCloser (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    CompWindow* w;
    Window xid;
    
    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    HEADTRACKING_WINDOW(w);
    wtw->isManualDepth = TRUE;
    wtw->depth = wtw->depth + (float)headtrackingGetWindowDepth (w->screen) / 100.0;
    damagePendingOnScreen (w->screen);
    return TRUE;
}

// Reset window
static Bool WTManualReset (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    CompWindow* w;
    Window xid;
    
    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    HEADTRACKING_WINDOW(w);
    wtw->depth = 0.0f;
    wtw->isManualDepth = FALSE;
    damagePendingOnScreen (w->screen);
    return TRUE;
}

/// Head Movement (Debug)
static Bool WTDebugCameraForward (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    if (headtrackingGetDebugEnabled (d)) {
    CompScreen *s;
    Window     xid;
    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);
    if (s){
	HEADTRACKING_SCREEN (s);
	wts->head.z = wts->head.z - (float)headtrackingGetCameraMove (s) / 100.0;
    }
    }
    return TRUE;
}
static Bool WTDebugCameraBack (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    if (headtrackingGetDebugEnabled (d)) {
    CompScreen *s;
    Window     xid;
    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);
    if (s){
	HEADTRACKING_SCREEN (s);
	wts->head.z = wts->head.z + (float)headtrackingGetCameraMove (s) / 100.0;
    }
    }
    return TRUE;
}
static Bool WTDebugCameraLeft (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    if (headtrackingGetDebugEnabled (d)) {
    CompScreen *s;
    Window     xid;
    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);
    if (s){
	HEADTRACKING_SCREEN (s);
	wts->head.x = wts->head.x + (float)headtrackingGetCameraMove (s) / 100.0;
    }
    }
    return TRUE;
}
static Bool WTDebugCameraRight (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    if (headtrackingGetDebugEnabled (d)) {
    CompScreen *s;
    Window     xid;
    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);
    if (s){
	HEADTRACKING_SCREEN (s);
	wts->head.x = wts->head.x - (float)headtrackingGetCameraMove (s) / 100.0;
    }
    }
    return TRUE;
}
static Bool WTDebugCameraUp (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    if (headtrackingGetDebugEnabled (d)) {
    CompScreen *s;
    Window     xid;
    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);
    if (s){
	HEADTRACKING_SCREEN (s);
	wts->head.y = wts->head.y - (float)headtrackingGetCameraMove (s) / 100.0;
    }
    }
    return TRUE;
}
static Bool WTDebugCameraDown (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    if (headtrackingGetDebugEnabled (d)) {
    CompScreen *s;
    Window     xid;
    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);
    if (s){
	HEADTRACKING_SCREEN (s);
	wts->head.y = wts->head.y + (float)headtrackingGetCameraMove (s) / 100.0;
    }
    }
    return TRUE;
}
static Bool WTDebugCameraReset (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    if (headtrackingGetDebugEnabled (d)) {
    CompScreen *s;
    Window     xid;
    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);
    if (s){
	HEADTRACKING_SCREEN (s);
	wts->head.y = 0.0f;
	wts->head.x = 0.0f;
	wts->head.z = 1.0f;
    }
    }
    return TRUE;
}

// Toggle Mouse-Tracking (overides normal tracking!)
static Bool WTDebugToggleMouse (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    if (headtrackingGetDebugEnabled (d)) {
    CompScreen *s;
    Window     xid;
    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);
    if (s) {
        HEADTRACKING_SCREEN (s);
        wts->trackMouse = !wts->trackMouse;
    }
    }
    return TRUE;
}

static Bool headtrackingInitWindow(CompPlugin *p, CompWindow *w){
    WTWindow *wtw;
    HEADTRACKING_SCREEN(w->screen);

    if( !(wtw = (WTWindow*)malloc( sizeof(WTWindow) )) )
	return FALSE;

    wtw->depth = 0.0; // Reset window actual depth
    wtw->zDepth = 0.0; // Reset window z orders
    wtw->isManualDepth = FALSE; // Reset windows to automatic
    wtw->manualDepth = 0.0;
    wtw->grabbed = 0;
    
    w->base.privates[wts->windowPrivateIndex].ptr = wtw;
    
    return TRUE;
}

static void headtrackingFiniWindow(CompPlugin *p, CompWindow *w){

    HEADTRACKING_WINDOW(w);
    HEADTRACKING_DISPLAY(w->screen->display);
    
    wtw->depth = 0.0;
    

    if(wtd->grabWindow == w){
	wtd->grabWindow = NULL;
    }
    
    free(wtw); 
}
/*}}}*/

// Screen init / clean/*{{{*/
static Bool headtrackingInitScreen(CompPlugin *p, CompScreen *s){
    WTScreen *wts;

    HEADTRACKING_DISPLAY(s->display);

    if( !(wts = (WTScreen*)malloc( sizeof(WTScreen) )) )
	return FALSE;

    if( (wts->windowPrivateIndex = allocateWindowPrivateIndex(s)) < 0){
	free(wts);
	return FALSE;
    }

    wts->grabIndex = 0;
    wts->head.x = 0.0;
    wts->head.y = 0.0;
    wts->head.z = 1.0;
    wts->trackMouse = FALSE;
    

    s->base.privates[wtd->screenPrivateIndex].ptr = wts;
    WRAP(wts, s, preparePaintScreen, WTPreparePaintScreen);
    WRAP(wts, s, paintWindow, WTPaintWindow);
    WRAP(wts, s, paintOutput, WTPaintOutput);

    WRAP(wts, s, damageWindowRect, WTDamageWindowRect);

    return TRUE;
}

static void headtrackingFiniScreen(CompPlugin *p, CompScreen *s){

    HEADTRACKING_SCREEN(s);

    freeWindowPrivateIndex(s, wts->windowPrivateIndex);

    UNWRAP(wts, s, preparePaintScreen);
    UNWRAP(wts, s, paintWindow);
    UNWRAP(wts, s, paintOutput);
    

    UNWRAP(wts, s, damageWindowRect);

    free(wts);
}
/*}}}*/

// Display init / clean/*{{{*/
static Bool headtrackingInitDisplay(CompPlugin *p, CompDisplay *d){

    WTDisplay *wtd; 

    if( !(wtd = (WTDisplay*)malloc( sizeof(WTDisplay) )) )
	return FALSE;
    
    // Set variables correctly
    wtd->grabWindow = 0;
    wtd->focusWindow = 0;
    
    // We don't need to do this for the Wiimote unless we don't trust our users
    // but for our cube, let's see if it's enabled so we know whether or not 
    // to check the rotation status before setting window depths
    getPluginDisplayIndex (d, "cube", &cubeDisplayPrivateIndex);
 
    if( (wtd->screenPrivateIndex = allocateScreenPrivateIndex(d)) < 0 ){
	free(wtd);
	return FALSE;
    }
    
    headtrackingSetManualOutInitiate(d, WTManualMoveAway);
    headtrackingSetManualInInitiate(d, WTManualMoveCloser);
    headtrackingSetManualResetInitiate(d, WTManualReset);
    headtrackingSetCameraInInitiate(d, WTDebugCameraForward);
    headtrackingSetCameraOutInitiate(d, WTDebugCameraBack);
    headtrackingSetCameraLeftInitiate(d, WTDebugCameraLeft);
    headtrackingSetCameraRightInitiate(d, WTDebugCameraRight);
    headtrackingSetCameraUpInitiate(d, WTDebugCameraUp);
    headtrackingSetCameraDownInitiate(d, WTDebugCameraDown);
    headtrackingSetCameraResetInitiate(d, WTDebugCameraReset);
    headtrackingSetToggleMouseInitiate(d, WTDebugToggleMouse);
    
    // NOTICE: Tracking events have been removed
    // A general "set head position" may be added
    // back in at a later time, depends on what
    // is needed. If a separate headtracking
    // program that uses other methods and tools
    // is written, I'll open this up for use anywhere
    // but may also rename the plugin to just
    // be "headtracking" or something.
    
    d->base.privates[displayPrivateIndex].ptr = wtd;
    WRAP(wtd, d, handleEvent, WTHandleEvent);
    
    return TRUE;
}

static void headtrackingFiniDisplay(CompPlugin *p, CompDisplay *d){

    HEADTRACKING_DISPLAY(d);
    
    freeScreenPrivateIndex(d, wtd->screenPrivateIndex);

    UNWRAP(wtd, d, handleEvent);

    free(wtd);
}
/*}}}*/

// Object init / clean
static CompBool headtrackingInitObject(CompPlugin *p, CompObject *o){

    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, // InitCore
	(InitPluginObjectProc) headtrackingInitDisplay,
	(InitPluginObjectProc) headtrackingInitScreen,
	(InitPluginObjectProc) headtrackingInitWindow
    };

    RETURN_DISPATCH(o, dispTab, ARRAY_SIZE(dispTab), TRUE, (p, o));
}

static void headtrackingFiniObject(CompPlugin *p, CompObject *o){

    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, // FiniCore
	(FiniPluginObjectProc) headtrackingFiniDisplay,
	(FiniPluginObjectProc) headtrackingFiniScreen,
	(FiniPluginObjectProc) headtrackingFiniWindow
    };

    DISPATCH(o, dispTab, ARRAY_SIZE(dispTab), (p, o));
}

// Plugin init / clean
static Bool headtrackingInit(CompPlugin *p){
    if( (displayPrivateIndex = allocateDisplayPrivateIndex()) < 0 )
	return FALSE;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
	compAddMetadataFromFile (&headtrackingMetadata, p->vTable->name);

    return TRUE;
}

static void headtrackingFini(CompPlugin *p){
    if(displayPrivateIndex >= 0)
	freeDisplayPrivateIndex( displayPrivateIndex );
	
	glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    endThread();
}

// Plugin implementation export
CompPluginVTable headtrackingVTable = {
    "headtracking",
    0,
    headtrackingInit,
    headtrackingFini,
    headtrackingInitObject,
    headtrackingFiniObject,
    0,
    0
};

CompPluginVTable *getCompPluginInfo (void){ return &headtrackingVTable; }
