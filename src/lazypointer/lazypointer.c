/**
 *
 * Compiz lazypointer
 *
 * lazypointer.c
 *
 * Copyright (c) 2008 Sam Spilsbury <smspillaz@gmail.com>
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
 * Warps your pointer based on window events and actions so that
 * you don't have to.
 *
 * TODO: Maybe some animation would look sexy
 **/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <compiz-core.h>

#include "lazypointer_options.h"

#define PI 3.1415926

static int displayPrivateIndex;

typedef struct _LpDisplay
{
    int screenPrivateIndex;
} LpDisplay;

typedef struct _LpScreen
{
    FocusWindowProc    focusWindow;
    ActivateWindowProc activateWindow;
    PlaceWindowProc    placeWindow;
    PreparePaintScreenProc preparePaintScreen;
    DonePaintScreenProc    donePaintScreen;
    int			destX, destY;
    int			initialX, initialY;
    Bool		needsWarp;
} LpScreen;


#define GET_LP_DISPLAY(d)				    \
    ((LpDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define LP_DISPLAY(d)			  \
    LpDisplay *ld = GET_LP_DISPLAY (d)

#define GET_LP_SCREEN(s, ld)					\
    ((LpScreen *) (s)->base.privates[(ld)->screenPrivateIndex].ptr)

#define LP_SCREEN(s)						       \
    LpScreen *ls = GET_LP_SCREEN (s, GET_LP_DISPLAY (s->display))

static void
lpPreparePaintScreen (CompScreen *s,
		      int        ms)
{
    float speed  = lazypointerGetSpeed (s->display);
    float steps = ((float) ms / ((20.1 - speed) * 100));
    LP_SCREEN (s);

    if (steps < 0.0005)
	steps = 0.0005;

    if (ls->needsWarp)
    {

	if (ls->destX >= s->width)
	    ls->destX = s->width - 1;
	if (ls->destX <= 0)
	    ls->destX = 1;
	if (ls->destY >= s->height)
	    ls->destY = s->height -1;
	if (ls->destY <= 0)
	    ls->destY = 1;	    

	int dx = ceilf(steps * (ls->destX - pointerX) * speed);
	int dy = ceilf(steps * (ls->destY - pointerY) * speed);

	if (dx > 10)
	    dx = 10;
	if (dy > 10)
	    dy = 10;

	if (dx < -10)
	    dx = -10;
	if (dy < -10)
	    dy = -10;

	if (dx == 0 && dy == 0)
	    ls->needsWarp = FALSE;

	warpPointer (s, dx, dy);
    }

    UNWRAP (ls, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, ms);
    WRAP (ls, s, preparePaintScreen, lpPreparePaintScreen);
}

static void
lpDonePaintScreen (CompScreen *s)
{
    LP_SCREEN (s);

    if (pointerX != ls->destX || pointerY != ls->destY)
	damageScreen (s); /* Shouldn't do that */

    UNWRAP (ls, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (ls, s, donePaintScreen, lpDonePaintScreen);
}

static Bool
lpPlaceWindow     (CompWindow *w,
		  int        x,
		  int        y,
		  int        *newX,
		  int        *newY)
{
    LP_SCREEN (w->screen);

    int cX = (*newX - w->input.left) + ((w->width + w->input.right + w->input.left) / 2);
    int cY = (*newY - w->input.top) + ((w->height + w->input.top + w->input.bottom) / 2);
    Window       root, child;
    int          rx, ry, winx, winy;
    unsigned int mask;
    Bool         ret;
    Bool         status;

    ret = XQueryPointer(w->screen->display->display, w->screen->root,
                              &root, &child,
                              &rx, &ry, &winx, &winy, &mask);

    if (ret && lazypointerGetCentreOnPlace (w->screen->display))
    {
	ls->initialX = rx;
	ls->initialY = ry;

	ls->destX = cX;
	ls->destY = cY;
	ls->needsWarp = TRUE;
	damageScreen (w->screen);
    }

    UNWRAP (ls, w->screen, placeWindow);
    status = (*w->screen->placeWindow) (w, x, y, newX, newY);
    WRAP (ls, w->screen, placeWindow, lpPlaceWindow);

    return status;
}

static void
lpActivateWindow (CompWindow *w)
{
    LP_SCREEN (w->screen);

    int cX = (w->attrib.x - w->input.left) + ((w->width + w->input.right + w->input.left) / 2);
    int cY = (w->attrib.y - w->input.top) + ((w->height + w->input.top + w->input.bottom) / 2);
    Window       root, child;
    int          rx, ry, winx, winy;
    unsigned int mask;
    Bool         ret;

    ret = XQueryPointer(w->screen->display->display, w->screen->root,
                              &root, &child,
                              &rx, &ry, &winx, &winy, &mask);

    if (ret && lazypointerGetCentreOnActivate (w->screen->display))
    {
	ls->initialX = rx;
	ls->initialY = ry;

	ls->destX = cX;
	ls->destY = cY;
	ls->needsWarp = TRUE;
	damageScreen (w->screen);
    }

    UNWRAP (ls, w->screen, activateWindow);
    (*w->screen->activateWindow) (w);
    WRAP (ls, w->screen, activateWindow, lpActivateWindow);
}

static Bool
lpFocusWindow (CompWindow *w)
{
    LP_SCREEN (w->screen);

    int cX = (w->attrib.x - w->input.left) + ((w->width + w->input.right + w->input.left) / 2);
    int cY = (w->attrib.y - w->input.top) + ((w->height + w->input.top + w->input.bottom) / 2);
    Window       root, child;
    int          rx, ry, winx, winy;
    unsigned int mask;
    Bool         ret;
    Bool         status;

    ret = XQueryPointer(w->screen->display->display, w->screen->root,
                              &root, &child,
                              &rx, &ry, &winx, &winy, &mask);

    if (ret && lazypointerGetCentreOnFocus (w->screen->display))
    {
	ls->initialX = rx;
	ls->initialY = ry;

	ls->destX = cX;
	ls->destY = cY;
	ls->needsWarp = TRUE;
	damageScreen (w->screen);
    }

    UNWRAP (ls, w->screen, focusWindow);
    status = (*w->screen->focusWindow) (w);
    WRAP (ls, w->screen, focusWindow, lpFocusWindow);

    return status;
}

static Bool
lpWinInitiate (CompDisplay     *d,
	      CompAction      *action,
	      CompActionState state,
	      CompOption      *option,
	      int	      nOption)
{
    CompWindow *w;

    w = findWindowAtDisplay (d, getIntOptionNamed (option, nOption, "window", 0));

    if (!w)
	return FALSE;

    LP_SCREEN (w->screen);

    int cX = (w->attrib.x - w->input.left) + ((w->width + w->input.right + w->input.left) / 2);
    int cY = (w->attrib.y - w->input.top) + ((w->height + w->input.top + w->input.bottom) / 2);
    Window       root, child;
    int          rx, ry, winx, winy;
    unsigned int mask;
    Bool         ret;

    ret = XQueryPointer(d->display, w->screen->root,
                              &root, &child,
                              &rx, &ry, &winx, &winy, &mask);

    if (ret)
    {
	ls->initialX = rx;
	ls->initialY = ry;

	ls->destX = cX;
	ls->destY = cY;
	ls->needsWarp = TRUE;
	damageScreen (w->screen);
	return TRUE;
    }

    return FALSE;
}

static Bool
lpScreenInitiate (CompDisplay     *d,
	      	  CompAction      *action,
	      	  CompActionState state,
	      	  CompOption      *option,
	      	  int	      nOption)
{
    CompScreen *s;

    s = findScreenAtDisplay (d, getIntOptionNamed (option, nOption, "screen", 0));

    if (!s)
	return FALSE;

    LP_SCREEN (s);

    int cX = (s->width / 2);
    int cY = (s->height / 2);
    Window       root, child;
    int          rx, ry, winx, winy;
    unsigned int mask;
    Bool         ret;

    ret = XQueryPointer(d->display, s->root,
                              &root, &child,
                              &rx, &ry, &winx, &winy, &mask);

    if (ret)
    {
	ls->initialX = rx;
	ls->initialY = ry;

	ls->destX = cX;
	ls->destY = cY;
	ls->needsWarp = TRUE;
	damageScreen (s);
	return TRUE;
    }

    return FALSE;
}

static Bool
lpLeftInitiate (CompDisplay     *d,
	      	  CompAction      *action,
	      	  CompActionState state,
	      	  CompOption      *option,
	      	  int	      nOption)
{
    CompScreen *s;

    s = findScreenAtDisplay (d, getIntOptionNamed (option, nOption, "screen", 0));

    if (!s)
	return FALSE;

    warpPointer (s, -1, 0);

    return TRUE;
}

static Bool
lpRightInitiate (CompDisplay     *d,
	      	  CompAction      *action,
	      	  CompActionState state,
	      	  CompOption      *option,
	      	  int	      nOption)
{
    CompScreen *s;

    s = findScreenAtDisplay (d, getIntOptionNamed (option, nOption, "screen", 0));

    if (!s)
	return FALSE;

    warpPointer (s, 1, 0);

    return TRUE;
}

static Bool
lpUpInitiate (CompDisplay     *d,
	      	  CompAction      *action,
	      	  CompActionState state,
	      	  CompOption      *option,
	      	  int	      nOption)
{
    CompScreen *s;

    s = findScreenAtDisplay (d, getIntOptionNamed (option, nOption, "screen", 0));

    if (!s)
	return FALSE;

    warpPointer (s, 0, -1);

    return TRUE;
}

static Bool
lpDownInitiate (CompDisplay     *d,
	      	  CompAction      *action,
	      	  CompActionState state,
	      	  CompOption      *option,
	      	  int	      nOption)
{
    CompScreen *s;

    s = findScreenAtDisplay (d, getIntOptionNamed (option, nOption, "screen", 0));

    if (!s)
	return FALSE;

    warpPointer (s, 0, -1);

    return TRUE;
}

static Bool
lpInitDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    LpDisplay *ld;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    ld = malloc (sizeof (LpDisplay));
    if (!ld)
	return FALSE;

    ld->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (ld->screenPrivateIndex < 0)
    {
	free (ld);
	return FALSE;
    }

    lazypointerSetInitiateWindowKeyInitiate (d, lpWinInitiate);
    lazypointerSetInitiateScreenKeyInitiate (d, lpScreenInitiate);
    lazypointerSetInitiateLeftKeyInitiate (d, lpLeftInitiate);
    lazypointerSetInitiateRightKeyInitiate (d, lpRightInitiate);
    lazypointerSetInitiateUpKeyInitiate (d, lpUpInitiate);
    lazypointerSetInitiateDownKeyInitiate (d, lpDownInitiate);

    d->base.privates[displayPrivateIndex].ptr = ld;

    return TRUE;
}

static void
lpFiniDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    LP_DISPLAY (d);

    freeScreenPrivateIndex (d, ld->screenPrivateIndex);
	
    free (ld);
}

static Bool
lpInitScreen (CompPlugin *p,
		CompScreen *s)
{
    LpScreen * ls;

    LP_DISPLAY (s->display);

    ls = malloc (sizeof (LpScreen));
    if (!ls)
	return FALSE;

    WRAP (ls, s, activateWindow, lpActivateWindow);
    WRAP (ls, s, focusWindow, lpFocusWindow);
    WRAP (ls, s, placeWindow, lpPlaceWindow);
    WRAP (ls, s, preparePaintScreen, lpPreparePaintScreen);
    WRAP (ls, s, donePaintScreen, lpDonePaintScreen);

    ls->needsWarp = FALSE;
    ls->destX = 0;
    ls->destY = 0;

    s->base.privates[ld->screenPrivateIndex].ptr = ls;

    return TRUE;
}

static void
lpFiniScreen (CompPlugin *p,
		CompScreen *s)
{
    LP_SCREEN (s);

    UNWRAP (ls, s, activateWindow);
    UNWRAP (ls, s, focusWindow);
    UNWRAP (ls, s, placeWindow);
    UNWRAP (ls, s, preparePaintScreen);
    UNWRAP (ls, s, donePaintScreen);

    free (ls);
}

static CompBool
lpInitObject (CompPlugin *p,
		CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) lpInitDisplay,
	(InitPluginObjectProc) lpInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
lpFiniObject (CompPlugin *p,
		CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) lpFiniDisplay,
	(FiniPluginObjectProc) lpFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
lpInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;
	
    return TRUE;
}

static void
lpFini (CompPlugin *p)
{
    freeDisplayPrivateIndex(displayPrivateIndex);
}

static CompPluginVTable lpVTable = {
    "lazypointer",
    0,
    lpInit,
    lpFini,
    lpInitObject,
    lpFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &lpVTable;
}
