/**
 *
 * dodge.c
 *
 * Copyright (c) 2007 Douglas Young <rcxdude@gmail.com>
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
 **/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>

#include <compiz-core.h>
#include <compiz-mousepoll.h>
#include <math.h>

#include "dodge_options.h"



static int displayPrivateIndex;

typedef struct _DodgeDisplay
{
    int screenPrivateIndex;

    MousePollFunc *mpFunc;	
    Atom resizeNotifyAtom;
} DodgeDisplay;

#define NORTH 0
#define SOUTH 1
#define EAST  2
#define WEST  3

typedef struct _DodgeWindow
{
    float   vx; /* velocity */
    float   vy;
    float    x; /* position as float */
    float    y;

    int     ox; /* original position */
    int	    oy;
    
    int     pos; /* North South East West */

    Bool    selected;
    Bool    isdodge;
} DodgeWindow;

typedef struct _DodgeScreen
{
    PaintOutputProc             paintOutput;
    PreparePaintScreenProc      preparePaintScreen;
    WindowStateChangeNotifyProc windowStateChangeNotify;
    WindowMoveNotifyProc	windowMoveNotify;
    WindowResizeNotifyProc      windowResizeNotify;
    DonePaintScreenProc         donePaintScreen;

    PositionPollingHandle  pollHandle;
    
    int windowPrivateIndex;
    CompWindow *window;
    
    int px;
    int py;

    int model;
    int padding;
    float springk;
    float friction;
    Bool active;
    Bool exiting;
    Bool movelock;
    Bool moreAdjust;

} DodgeScreen;


#define GET_DODGE_DISPLAY(d) \
    ((DodgeDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define DODGE_DISPLAY(d) \
    DodgeDisplay *dd = GET_DODGE_DISPLAY (d)

#define GET_DODGE_SCREEN(s, dd) \
    ((DodgeScreen *) (s)->base.privates[(dd)->screenPrivateIndex].ptr)

#define DODGE_SCREEN(s) \
    DodgeScreen *ds = GET_DODGE_SCREEN (s, \
		      GET_DODGE_DISPLAY (s->display))

#define GET_DODGE_WINDOW(w, ds) \
    ((DodgeWindow *) (w)->base.privates[(ds)->windowPrivateIndex].ptr)

#define DODGE_WINDOW(w) \
    DodgeWindow *dw = GET_DODGE_WINDOW  (w, \
		      GET_DODGE_SCREEN  (w->screen, \
		      GET_DODGE_DISPLAY (w->screen->display)))

#define POSITIVE(x) ((x)>0?(x):0)
	
	
#define PI 3.1415926
#define FRICTION (0.005f * ds->friction)
#define SPRING_K (0.00005f * ds->springk)
#define PADDING (ds->padding)

#define WIN_X(w) ((w)->attrib.x - (w)->input.left)
#define WIN_Y(w) ((w)->attrib.y - (w)->input.top)
#define WIN_W(w) ((w)->width + (w)->input.left + (w)->input.right)
#define WIN_H(w) ((w)->height + (w)->input.top + (w)->input.bottom)

#define WIN_CX(w) (WIN_X(w) + WIN_W(w)/2)
#define WIN_CY(w) (WIN_Y(w) + WIN_H(w)/2) 

static int
dodgestep (CompWindow *w, int ms, int away)
{
    if (!w) return 0;
    DODGE_WINDOW (w);
    DODGE_SCREEN (w->screen);
    int dx; /*  x distance from pointer */
    int dy; /*  y distance from pointer */
    float dp; /*  actual distance from pointer */
    switch (ds->model)
    {
	case ModelSimpleAvoid:
	    /* increase velocity away from the pointer */
	    dx = WIN_CX(w) - ds->px;
	    dy = WIN_CY(w) - ds->py; 
	    
	    dp = sqrt(dx*dx + dy*dy);
	    if(WIN_W(w)/2 + PADDING - abs(dx) > 
	       WIN_H(w)/2 + PADDING - abs(dy))
	    {
		dw->vx += POSITIVE(WIN_H(w)/2 + PADDING - abs(dy)) 
		    * SPRING_K * dx/(abs(dx) + abs(dy)) * ms;
		dw->vy += POSITIVE(WIN_H(w)/2 + PADDING - abs(dy)) 
		    * SPRING_K * dy/(abs(dx) + abs(dy)) * ms;
	    } else {
		dw->vx += POSITIVE(WIN_W(w)/2 + PADDING - abs(dx)) 
		    * SPRING_K * dx/(abs(dx) + abs(dy)) * ms;
		dw->vy += POSITIVE(WIN_W(w)/2 + PADDING - abs(dx)) 
		    * SPRING_K * dy/(abs(dx) + abs(dy)) * ms;
	    }
	    break;
	case ModelReturnToPosition:
	    if ((ds->px > dw->ox - WIN_W(w)/2 - PADDING &&
		ds->py > dw->oy - WIN_H(w)/2 - PADDING &&
		ds->px < dw->ox + WIN_W(w)/2 + PADDING &&
		ds->py < dw->oy + WIN_H(w)/2 + PADDING) &&
		!ds->exiting && away)
	    {
		/* increase velocity away from / toward the pointer */
		dx = WIN_CX(w) - ds->px;
		dy = WIN_CY(w) - ds->py; 
		
		dp = sqrt(dx*dx + dy*dy);
		if(WIN_W(w)/2 + PADDING - abs(dx) > 
		   WIN_H(w)/2 + PADDING - abs(dy))
		{
		    dw->vx += (WIN_H(w)/2 + PADDING - abs(dy)) 
			      * SPRING_K * dx/(abs(dx) + abs(dy)) * ms;
		    dw->vy += (WIN_H(w)/2 + PADDING - abs(dy)) 
			      * SPRING_K * dy/(abs(dx) + abs(dy)) * ms;
		} else {
		    dw->vx += (WIN_W(w)/2 + PADDING - abs(dx)) 
			      * SPRING_K * dx/(abs(dx) + abs(dy)) * ms;
		    dw->vy += (WIN_W(w)/2 + PADDING - abs(dx)) 
			      * SPRING_K * dy/(abs(dx) + abs(dy)) * ms;
		}
	    } else {
		/* increase velocity towards the original position */
		dx = WIN_CX(w) - dw->ox;
		dy = WIN_CY(w) - dw->oy; 
		
		dp = sqrt(dx*dx + dy*dy);
		if (dx || dy) 
		{
		    dw->vx -= dp * SPRING_K * dx/(abs(dx) + abs(dy)) * ms;
		    dw->vy -= dp * SPRING_K * dy/(abs(dx) + abs(dy)) * ms;
		}
	    }
	    break;
	case ModelOffScreen:
	    if ((ds->px > dw->ox - WIN_W(w)/2 - PADDING &&
		ds->py > dw->oy - WIN_H(w)/2 - PADDING &&
		ds->px < dw->ox + WIN_W(w)/2 + PADDING &&
		ds->py < dw->oy + WIN_H(w)/2 + PADDING) &&
		!ds->exiting && away)
	    {
		switch (dw->pos)
		{
		    case NORTH:
			dx = 0;
			dy = WIN_Y(w) + WIN_H(w);
			break;
		    case SOUTH:
			dx = 0;
			dy = WIN_Y(w) - w->screen->height;
			break;
		    case EAST:
			dx = WIN_X(w) + WIN_W(w);
			dy = 0;
			break;
		    case WEST:
			dx = WIN_X(w) - w->screen->width;
			dy = 0;
			break;
		}
		
		dw->vx -= dx * SPRING_K * ms;
		dw->vy -= dy * SPRING_K * ms;
	    } else {
		dx = WIN_CX(w) - dw->ox;
		dy = WIN_CY(w) - dw->oy; 
		
		dp = sqrt(dx*dx + dy*dy);
		if (dx || dy) 
		{
		    dw->vx -= dp * SPRING_K * dx/(abs(dx) + abs(dy)) * ms * 0.5;
		    dw->vy -= dp * SPRING_K * dy/(abs(dx) + abs(dy)) * ms * 0.5;
		}
	    }
	    break;

    }

    /* apply friction */
    dw->vx -= FRICTION * dw->vx * ms;
    dw->vy -= FRICTION * dw->vy * ms;
    
    /* stop the window going offscreen with the simple model */
    if (((WIN_X(w) + WIN_W(w) > w->screen->width  && dw->vx > 0) || 
	(WIN_X(w) < 0                             && dw->vx < 0)) &&
	ds->model == ModelSimpleAvoid)
	dw->vx = 0;
    if (((WIN_Y(w) + WIN_H(w) > w->screen->height && dw->vy > 0) ||
	(WIN_Y(w) < 0                             && dw->vy < 0)) &&
	ds->model == ModelSimpleAvoid)
	dw->vy = 0;
    /* stop excessive velocity  */
    if (dw->vx > w->screen->width  || dw->vx < 0 - w->screen->width ||
	dw->vy > w->screen->height || dw->vx < 0 - w->screen->height)
	    dw->vx = dw->vy = 0;

    dw->x += dw->vx * ms;
    dw->y += dw->vy * ms;
    /* if there's enough differenc, move the window */
    if (fabs(dw->x - WIN_X(w)) >= 1 || fabs(dw->y - WIN_Y(w)) >= 1)
    {
	moveWindow (w,(int) dw->x - WIN_X(w),
		      (int) dw->y - WIN_Y(w),TRUE,TRUE);
	syncWindowPosition (w);
	ds->moreAdjust = TRUE;
    }
    if ((dy || dx) && !ds->model == ModelSimpleAvoid)
    {
	ds->moreAdjust = TRUE;
    }
    return 0;

}


static void
dodgePreparePaintScreen (CompScreen *s,
			int	ms)
{
    DODGE_SCREEN (s);
    CompWindow *w;
    /*  if another plugin has grabbed the screen we don't want to interfere
     *  except for expo or rotate cube, in which case windows return to their original
     *  positions */
    if (otherScreenGrabExist(s,"dodge","expo","rotate",0))
    {
	UNWRAP (ds, s, preparePaintScreen);
	(*s->preparePaintScreen) (s, ms);
	WRAP (ds, s, preparePaintScreen, dodgePreparePaintScreen);
	return;
    }

    if (ds->active)
    {
	ds->movelock = TRUE;
	ds->moreAdjust = FALSE;
	for (w = s->windows; w; w = w->next)
	{
	    DODGE_WINDOW (w);
	    if (dw->isdodge)
		dodgestep(w,ms,!otherScreenGrabExist(s,"dodge",0)
		          && (w->screen->display->activeWindow != w->id 
		          || dodgeGetDodgeActive (s->display)));
	}
	ds->movelock = FALSE;
    }
    if (ds->exiting && !ds->moreAdjust)
    {
	ds->active = FALSE;
	ds->exiting = FALSE;
	ds->moreAdjust = FALSE;
    }

    UNWRAP (ds, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, ms);
    WRAP (ds, s, preparePaintScreen, dodgePreparePaintScreen);
}

static void
dodgeDonePaintScreen (CompScreen *s)
{
    DODGE_SCREEN(s);
    if (ds->active && ds->moreAdjust)
    {
	CompWindow *w;
	for (w = s->windows; w; w = w->next)
	{
	    DODGE_WINDOW(w);
	    if (dw->isdodge)
		addWindowDamage(w);
	}
    }
    UNWRAP (ds, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (ds, s, donePaintScreen, dodgeDonePaintScreen);
}

static void
dodgeUpdateDodgeWindow (CompWindow *w)
{
    DODGE_WINDOW (w);
    DODGE_SCREEN (w->screen);
    if((matchEval(dodgeGetWindowTypes (w->screen->display),w) || dw->selected) &&
       !(w->invisible || w->destroyed || w->hidden || w->minimized))
    {
	if (dw)
	{
	    dw->isdodge = TRUE;
	    dw->vx = 0;
	    dw->vy = 0;
	    dw->x = (float) WIN_X(w);
	    dw->y = (float) WIN_Y(w);
	    /* don't change original position if dodge is active */
	    if (!ds->active || !(dw->ox || dw->oy))
	    {
		dw->ox = WIN_CX(w);
		dw->oy = WIN_CY(w);
	    }
	    if (WIN_CX(w) > WIN_CY(w) &&
		WIN_CX(w) * w->screen->height + 
		WIN_CY(w) * w->screen->width  >
		(w->screen->width * w->screen->height))
		    dw->pos = WEST;
	    if (WIN_CX(w) < WIN_CY(w) &&
		WIN_CX(w) * w->screen->height + 
		WIN_CY(w) * w->screen->width  >
		(w->screen->width * w->screen->height))
		    dw->pos = SOUTH;
	    if (WIN_CX(w) > WIN_CY(w) &&
		WIN_CX(w) * w->screen->height + 
		WIN_CY(w) * w->screen->width  <
		(w->screen->width * w->screen->height))
		    dw->pos = NORTH;
	    if (WIN_CX(w) < WIN_CY(w) &&
		WIN_CX(w) * w->screen->height + 
		WIN_CY(w) * w->screen->width  <
		(w->screen->width * w->screen->height))
		    dw->pos = EAST;
	
	}
    }
    else
    {
	if (dw->isdodge)
	{
	    dw->isdodge = FALSE;
	    dw->ox = 0;
	    dw->oy = 0;
	}
    }

}

static Bool
dodgeSelect  (CompDisplay    *d,
	    CompAction     *action,
	    CompActionState state,
	    CompOption     *option,
	    int           nOption)
{
    Window     xid;
    CompWindow *w;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w   = findWindowAtDisplay (d, xid);
    DODGE_WINDOW(w);
    DODGE_SCREEN(w->screen);
    dw->selected = !dw->selected;
    if (ds->active)
	dodgeUpdateDodgeWindow (w);

    return TRUE;
}

static Bool
dodgeToggle (CompDisplay    *d,
	    CompAction     *action,
	    CompActionState state,
	    CompOption     *option,
	    int           nOption)
{
    CompScreen *s;
    for (s = d->screens; s; s = s->next)
    {
	if (s)
	{
	    DODGE_SCREEN(s);
	    if (ds->active)
		ds->exiting = TRUE;
	    else
	    {
		ds->model    = dodgeGetModel    (d);
		ds->springk  = dodgeGetSpringK  (d);
		ds->friction = dodgeGetFriction (d);
		ds->padding  = dodgeGetPadding  (d);
		CompWindow *w;
		for (w = s->windows; w; w = w->next)
		{
		    dodgeUpdateDodgeWindow (w);
		}
		ds->active = TRUE;
	    }
	}
    }
    return TRUE;
}

static void
dodgeWindowMoveNotify (CompWindow *w,
		       int        dx,
		       int        dy,
		       Bool       immediate)
{
    DODGE_WINDOW (w);
    DODGE_SCREEN (w->screen);
   
    if (!ds->movelock && ds->active)
    {
	dw->ox += dx;
	dw->oy += dy;
    }
    UNWRAP (ds, w->screen, windowMoveNotify);
    (*w->screen->windowMoveNotify) (w, dx, dy, immediate);
    WRAP (ds, w->screen, windowMoveNotify, dodgeWindowMoveNotify);
}

static void
dodgeWindowResizeNotify (CompWindow *w,
			 int        dx,
			 int        dy,
			 int        dwidth,
			 int        dheight)
{
    DODGE_WINDOW (w);
    DODGE_SCREEN (w->screen);
    
    if (!ds->movelock && ds->active)
    {
	dw->ox += dx;
	dw->oy += dy;
    }
    UNWRAP (ds, w->screen, windowResizeNotify);
    (*w->screen->windowResizeNotify) (w, dx, dy, dwidth, dheight);
    WRAP (ds, w->screen, windowResizeNotify, dodgeWindowResizeNotify);
}

static void
positionUpdate (CompScreen *s, int x, int y)
{
    DODGE_SCREEN (s);
    ds->px = x;
    ds->py = y;
    if (ds->active)
    {
	/* damage a small area around the mouse to prevent jerkiness */
	REGION r;
	
	r.rects = &r.extents;
	r.numRects = r.size = 1;
	r.extents.x1 = ds->px - PADDING;
	r.extents.x2 = ds->px + PADDING;
	r.extents.y1 = ds->py - PADDING;
	r.extents.y2 = ds->py + PADDING;

	damageScreenRegion(s,&r);
    }

}

static void
dodgeStateChange (CompWindow *w,
		  unsigned int last)
{
    DODGE_SCREEN (w->screen);
    if(ds->active)
	dodgeUpdateDodgeWindow (w);
    UNWRAP (ds, w->screen, windowStateChangeNotify);
    (*w->screen->windowStateChangeNotify) (w,last);
    WRAP (ds, w->screen, windowStateChangeNotify, dodgeStateChange);

}

static void 
dodgeModelChange     (CompDisplay *d,
		      CompOption *opt,
		      DodgeDisplayOptions num)
{
    CompScreen *s;
    for (s = d->screens; s; s = s->next)
    {
	DODGE_SCREEN (s);
	ds->model    = dodgeGetModel    (d);
    }
}

static void 
dodgeFrictionChange (CompDisplay *d,
		     CompOption *opt,
		     DodgeDisplayOptions num)
{
    CompScreen *s;
    for (s = d->screens; s; s = s->next)
    {
	DODGE_SCREEN (s);
	ds->friction = dodgeGetFriction (d);
    }
}

static void 
dodgePaddingChange (CompDisplay *d,
		     CompOption *opt,
		     DodgeDisplayOptions num)
{
    CompScreen *s;
    for (s = d->screens; s; s = s->next)
    {
	DODGE_SCREEN (s);
	ds->padding  = dodgeGetPadding  (d);
    }
}

static void 
dodgeKChange        (CompDisplay *d,
		     CompOption *opt,
		     DodgeDisplayOptions num)
{
    CompScreen *s;
    for (s = d->screens; s; s = s->next)
    {
	DODGE_SCREEN (s);
	ds->springk  = dodgeGetSpringK  (d);
    }
}

static void 
dodgeTypeChange    (CompDisplay *d,
		    CompOption *opt,
		    DodgeDisplayOptions num)
{
    CompScreen *s;
    for (s = d->screens; s; s = s->next)
    {
	CompWindow *w;
	for (w = s->windows; w; w = w->next)
	{
	    dodgeUpdateDodgeWindow (w);
	}
    }
}

static Bool
dodgeInitDisplay (CompPlugin  *p,
		  CompDisplay *d)
{
    DodgeDisplay *dd;
    int mousepollindex;
    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    dd = malloc (sizeof (DodgeDisplay));
    if (!dd)
	return FALSE;

    if (!getPluginDisplayIndex (d, "mousepoll", &mousepollindex))
	return FALSE;

    dd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (dd->screenPrivateIndex < 0)
    {
	free (dd);
	return FALSE;
    }

    d->base.privates[displayPrivateIndex].ptr = dd;
    dodgeSetDodgeToggleKeyInitiate    (d, dodgeToggle);
    dodgeSetDodgeToggleButtonInitiate (d, dodgeToggle);
    dodgeSetDodgeToggleEdgeInitiate   (d, dodgeToggle);
    dodgeSetDodgeSelectKeyInitiate    (d, dodgeSelect);

    dodgeSetModelNotify       (d, dodgeModelChange);
    dodgeSetFrictionNotify    (d, dodgeFrictionChange);
    dodgeSetPaddingNotify     (d, dodgePaddingChange);
    dodgeSetSpringKNotify     (d, dodgeKChange);
    dodgeSetWindowTypesNotify (d, dodgeTypeChange);
    
    dd->mpFunc = d->base.privates[mousepollindex].ptr;
    return TRUE;
}

static void
dodgeFiniDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    DODGE_DISPLAY (d);
    freeScreenPrivateIndex (d, dd->screenPrivateIndex);
    free (dd);
}

static Bool
dodgeInitScreen (CompPlugin *p,
		CompScreen *s)
{
    DodgeScreen * ds;

    DODGE_DISPLAY (s->display);

    ds = malloc (sizeof (DodgeScreen));
    if (!ds)
	return FALSE;

    ds->active = FALSE;
    ds->exiting = FALSE;
    ds->moreAdjust = FALSE;

    WRAP (ds, s, preparePaintScreen, dodgePreparePaintScreen);
    WRAP (ds, s, windowStateChangeNotify, dodgeStateChange);
    WRAP (ds, s, windowMoveNotify, dodgeWindowMoveNotify);
    WRAP (ds, s, windowResizeNotify, dodgeWindowResizeNotify);
    WRAP (ds, s, donePaintScreen, dodgeDonePaintScreen);
    
    ds->windowPrivateIndex = allocateWindowPrivateIndex (s);
    s->base.privates[dd->screenPrivateIndex].ptr = ds;
    
    (*dd->mpFunc->getCurrentPosition) (s, &ds->px, &ds->py);
    ds->pollHandle = (*dd->mpFunc->addPositionPolling) (s, positionUpdate);
    return TRUE;
}

static void
dodgeFiniScreen (CompPlugin *p,
		CompScreen *s)
{
    DODGE_SCREEN (s);
    DODGE_DISPLAY (s->display);	
    UNWRAP (ds, s, preparePaintScreen);
    UNWRAP (ds, s, windowStateChangeNotify);
    UNWRAP (ds, s, windowMoveNotify);
    UNWRAP (ds, s, windowResizeNotify);
    UNWRAP (ds, s, donePaintScreen);
    
    (*dd->mpFunc->removePositionPolling) (s, ds->pollHandle);
    free (ds);
}

static Bool
dodgeInitWindow (CompPlugin *p,
		 CompWindow *w)
{
    DodgeWindow *dw;

    DODGE_SCREEN (w->screen);

    dw = malloc (sizeof (DodgeWindow));
    if (!dw)
	return FALSE;
    dw->vx = 0;
    dw->vy = 0;
    dw->isdodge = FALSE;
    dw->selected = FALSE;
    w->base.privates[ds->windowPrivateIndex].ptr = dw;
    if (ds->active)
	dodgeUpdateDodgeWindow (w);
    
    return TRUE;
}

static void
dodgeFiniWindow (CompPlugin *p,
		 CompWindow *w)
{
    DODGE_WINDOW (w);
    free (dw);
}

static CompBool
dodgeInitObject (CompPlugin *p,
		CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) dodgeInitDisplay,
	(InitPluginObjectProc) dodgeInitScreen,
	(InitPluginObjectProc) dodgeInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
dodgeFiniObject (CompPlugin *p,
		CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) dodgeFiniDisplay,
	(FiniPluginObjectProc) dodgeFiniScreen,
	(FiniPluginObjectProc) dodgeFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
dodgeInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;
	
    return TRUE;
}

static void
dodgeFini (CompPlugin *p)
{
    freeDisplayPrivateIndex(displayPrivateIndex);
}

static CompPluginVTable dodgeVTable = {
    "dodge",
    0,
    dodgeInit,
    dodgeFini,
    dodgeInitObject,
    dodgeFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &dodgeVTable;
}
