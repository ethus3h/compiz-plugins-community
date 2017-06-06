/*
 * Compiz Fusion Newton plugin
 *
 * Copyright (c) 2008 Patrick Nicolas <patrick.nicolas@rez-gif.supelec.fr>
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
 * Author(s):
 * Patrick Nicolas <patrick.nicolas@rez-gif.supelec.fr>
 *
 * Description:
 *
 */

#include <compiz-core.h>
#include "newton_options.h"
#include <compiz-mousepoll.h>
#include "chipmunk.h"
#include <math.h>

static int displayPrivateIndex;

#define GET_NEWTON_DISPLAY(d)					    \
    ((NewtonDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define NEWTON_DISPLAY(d)		       \
    NewtonDisplay *nd = GET_NEWTON_DISPLAY (d)

#define GET_NEWTON_SCREEN(s, nd)					\
    ((NewtonScreen *) (s)->base.privates[(nd)->screenPrivateIndex].ptr)

#define NEWTON_SCREEN(s)						      \
    NewtonScreen *ns = GET_NEWTON_SCREEN (s, GET_NEWTON_DISPLAY (s->display))

#define GET_NEWTON_WINDOW(w, ns) \
    ((NewtonWindow *) (w)->base.privates[(ns)->windowPrivateIndex].ptr)

#define NEWTON_WINDOW(w) \
    NewtonWindow *nw = GET_NEWTON_WINDOW  (w,                  \
		      GET_NEWTON_SCREEN  (w->screen,          \
		      GET_NEWTON_DISPLAY (w->screen->display)))

/* helper macros to get the full dimensions of a window,
   including decorations */
#define WIN_FULL_X(w) ((w)->serverX - (w)->input.left)
#define WIN_FULL_Y(w) ((w)->serverY - (w)->input.top)
#define WIN_FULL_W(w) ((w)->serverWidth + 2 * (w)->serverBorderWidth + \
		       (w)->input.left + (w)->input.right)
#define WIN_FULL_H(w) ((w)->serverHeight + 2 * (w)->serverBorderWidth + \
		       (w)->input.top + (w)->input.bottom)

#define WINDOW_MASS(w) \
    (WIN_FULL_W(w) * WIN_FULL_H(w))

#define WINDOW_POSITION(w) \
    cpv(WIN_FULL_X(w) + WIN_FULL_W(w)/2, \
        WIN_FULL_Y(w) + WIN_FULL_H(w)/2 )

#define WINDOW_BORDER(w) \
    {	cpv ( -WIN_FULL_W(w)/2, -WIN_FULL_H(w)/2 ), \
	cpv ( -WIN_FULL_W(w)/2,  WIN_FULL_H(w)/2 ), \
	cpv (  WIN_FULL_W(w)/2,  WIN_FULL_H(w)/2 ), \
	cpv (  WIN_FULL_W(w)/2, -WIN_FULL_H(w)/2 )  \
    }

/**
 *Display data structure
 */
typedef struct _NewtonDisplay {
    /**
      *Compiz internal, link to the data structure that holds the screens pointer
      */
    int screenPrivateIndex;

    /**
      *True if the mouse effect is inverted (eg repulsion if default is set to attraction)
      */
    Bool mouseInvert;
    /**
      *Mouse polling function collection, from compiz-mousepoll.h
      */
    MousePollFunc *mpFunc;
    /**
      *Event handler function
      */
    HandleEventProc handleEvent;
} NewtonDisplay;

typedef struct _NewtonScreen {
    /**
      *Compiz internal, link to the data structure that holds the windows pointer
      */
    int windowPrivateIndex;
    /**
      *Known position of the mouse
      */
    cpVect mousePos;

    /**
      *Chipmunk's space, holds all the physic entities that have to be treated
      */
    cpSpace *space;

    /**
     *Static Body, used to attach things to it
     */
    cpBody *staticBody;

    /**
     * Number of physic-enabled windows on the screen
     */
    int physWindows;

    /**
      *Function to call when mouse position has changed
      */
    PositionPollingHandle	   pollHandle;
    /**
      *Function to call when a window is painted
      */
    PaintWindowProc		   paintWindow;
    /**
      *Function to call before painting the screen
      */
    PreparePaintScreenProc	   preparePaintScreen;
    /**
      *Function to call after painting the screen
      */
    DonePaintScreenProc	   donePaintScreen;
    /**
     * Function to call when a window is resized
     */
    WindowResizeNotifyProc         windowResizeNotify;

    WindowGrabNotifyProc windowGrabNotify;
    WindowUngrabNotifyProc windowUngrabNotify;

} NewtonScreen;

typedef struct _NewtonWindow {
    /**
      *Physical body that represents the window.
      *It contains information about weight, momentum, position and velocity
      */
    cpBody *body;
    /**
      *Physical collision shape of the window.
      *It is simply a rectangle, it holds information about the window size
      */
    cpShape *shape;

    /**
      *True if the window generates repulsion with windows overlapping it
      */
    Bool repulsion;
    /**
      *True if the mouse was over the window on the last frame
      */
    Bool wasHovered;

    Bool movable;

    Bool rotate;

    Bool bodyInSpace; //nothing to do with astronauts

    cpJoint *joint;

} NewtonWindow;

/**
  *Updates the mouse position in the NewtonScreen data structure.
  */
static void
positionUpdate (CompScreen *s///Screen
		,int        x///New x position
		,int        y///New y position
		)
{
    NEWTON_SCREEN (s);

    ns->mousePos = cpv(x,y);
}

/**
 * Dummy update velocity function (for stopped windows)
 */
static void newtonNoVelocityUpdate(struct cpBody *body,
				   cpVect gravity,
				   cpFloat damping,
				   cpFloat dt
				   )
{

}

/**
 * Dummy update position function (for stopped windows)
 */
static void newtonNoPositionUpdate(struct cpBody *body,
				   cpFloat dt
				   )
{

}


/**
  *Enables physics on the window.
  */
static void
newtonSetPhysics (CompWindow *w///Window to enable physic on
		  )
{
    NEWTON_WINDOW (w);
    NEWTON_SCREEN (w->screen);

    nw->body->p = WINDOW_POSITION(w);
    cpSpaceAddBody(ns->space, nw->body);
    nw->bodyInSpace = TRUE;
    cpSpaceAddShape (ns->space, nw->shape);
    nw->movable = TRUE;
    ns->physWindows++;
}

/**
  *Disables physics on the window.
  */
static void
newtonSetNoPhysics (CompWindow *w///Window to disable physics on
		    )
{
    NEWTON_WINDOW (w);
    NEWTON_SCREEN (w->screen);

    //reset rotation if any
    if (nw->body->a != 0.0f)
    {
        cpBodySetAngle(nw->body, 0.0f);
        damageScreen (w->screen);
    }

    if (nw->bodyInSpace)
    {
		cpSpaceRemoveBody (ns->space, nw->body);
		nw->bodyInSpace = FALSE;
    }
    if (nw->joint)
    {
	cpSpaceRemoveJoint(ns->space, nw->joint);
	cpJointDestroy(nw->joint);
    }
    cpSpaceRemoveShape (ns->space, nw->shape);

    nw->movable = FALSE;
    ns->physWindows--;
}

/**
 * Handles window resizing
 */
static void
newtonWindowResizeNotify (CompWindow * w, int dx, int dy, int dwidth, int dheight)
{
    NEWTON_WINDOW (w);
    NEWTON_SCREEN (w->screen);

    cpVect verts[] = WINDOW_BORDER(w) ;
    cpBodySetMass (nw->body, WINDOW_MASS(w) );
    nw->body->p = WINDOW_POSITION(w);
    cpSpaceRemoveShape (ns->space, nw->shape);
    cpFloat e = nw->shape->e;
    cpFloat u = nw->shape->u;
    cpShapeFree (nw->shape);
    nw->shape = cpPolyShapeNew (nw->body,
				4,
				verts,
				cpvzero);
    nw->shape->e = e;
    nw->shape->u = u;
    if (nw->movable)
        cpSpaceAddShape(ns->space, nw->shape);

    UNWRAP(ns, w->screen, windowResizeNotify);
    (*w->screen->windowResizeNotify) (w, dx, dy, dwidth, dheight);
    WRAP(ns, w->screen, windowResizeNotify, newtonWindowResizeNotify);
}

/**
 * Handles grabbing of a window
 */
static void
newtonGrabNotify (CompWindow   *w,
				      int	   x,
				      int	   y,
				      unsigned int state,
				      unsigned int mask)
{
	NEWTON_SCREEN(w->screen);
	NEWTON_WINDOW(w);

	if (nw->movable && nw->bodyInSpace)
	{
		cpSpaceRemoveBody(ns->space, nw->body);
    	nw->bodyInSpace = FALSE;
	}

    UNWRAP(ns, w->screen, windowGrabNotify);
    (*w->screen->windowGrabNotify) (w, x, y, state, mask);
    WRAP(ns, w->screen, windowGrabNotify, newtonGrabNotify);
}

/**
 * Handles releasing of a window
 */
static void
newtonUngrabNotify (CompWindow *w)
{
	NEWTON_SCREEN(w->screen);
	NEWTON_WINDOW(w);

	if (nw->movable)
	{
		cpSpaceAddBody(ns->space, nw->body);
		nw->bodyInSpace = TRUE;
	}

    UNWRAP(ns, w->screen, windowUngrabNotify);
    (*w->screen->windowUngrabNotify) (w);
    WRAP(ns, w->screen, windowUngrabNotify, newtonUngrabNotify);

}

/**
  *Handles events on the display.
  */
  /*
static void
newtonHandleEvent (CompDisplay *d ///Display where the event has occured
		   ,XEvent      *event ///Event (as defined by X11)
		   )
{
    CompWindow *w;
    NewtonWindow *nw;
    NewtonScreen *ns;

    NEWTON_DISPLAY (d);

    UNWRAP (nd, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (nd, d, handleEvent, newtonHandleEvent);

    switch (event->type)
    {

    }
}
*/

/**
  *Toggles physics on the selected window.
  */
static Bool
newtonTogglePhysics (CompDisplay     *d,
	      CompAction      *action,
	      CompActionState state,
	      CompOption      *option,
	      int             nOption)
{
    CompWindow *w = findWindowAtDisplay (d, d->activeWindow);
    if (!w)
	return TRUE;

    NEWTON_WINDOW (w);

    if (!nw->movable)
    {
		newtonSetPhysics (w);
    } else
    {
		newtonSetNoPhysics (w);
    }

    return TRUE;
}

/**
  *Toggles rotation on the selected window.
  */
static Bool
newtonToggleRotation (CompDisplay     *d,
	      CompAction      *action,
	      CompActionState state,
	      CompOption      *option,
	      int             nOption)
{
    CompWindow *w = findWindowAtDisplay (d, d->activeWindow);
    if (!w)
	return TRUE;

    NEWTON_WINDOW (w);

    if (!nw->rotate)
    {
		nw->rotate = TRUE;
		cpVect verts[] = WINDOW_BORDER(w) ;
		cpBodySetMoment (nw->body, cpMomentForPoly (nw->body->m, 4, verts, cpvzero));

    } else
    {
		nw->rotate = FALSE;
		nw->body->w =0;
		cpBodySetMoment (nw->body, INFINITY);
    }

    return TRUE;
}
/**
  *Inverts mouse effect between attraction and repulsion.
  */
static Bool
newtonMouseInvert (CompDisplay     *d,
	      CompAction      *action,
	      CompActionState state,
	      CompOption      *option,
	      int             nOption)
{
    NEWTON_DISPLAY (d);
    nd->mouseInvert = !nd->mouseInvert;

    return TRUE;
}

/**
  *Handles changes of the elasticity constant configuration.
  */
static void
newtonElasticityNotify (CompDisplay *d,
			CompOption *opt,
			NewtonDisplayOptions num)
{
    CompScreen *s;
    CompWindow *w;

    NewtonWindow *nw;
    NewtonScreen *ns;
    NEWTON_DISPLAY (d);

    cpFloat elasticity = newtonGetBounceConstant (d);

    for (s = d->screens ; s ; s = s->next)
    {
		ns = GET_NEWTON_SCREEN (s, nd);
		for (w = s->windows; w; w = w->next)
		{
			nw = GET_NEWTON_WINDOW (w, ns);
			if (nw->shape)
			nw->shape->e = elasticity;
		}
    }
}

/**
  *Handles changes of the gravity constant configuration.
  */
static void
newtonGravityNotify (CompDisplay *d,
		     CompOption *opt,
		     NewtonDisplayOptions num)
{
    CompScreen *s;
    NewtonScreen *ns;
    NEWTON_DISPLAY (d);
    for (s = d->screens ; s ; s = s->next)
    {
		ns = GET_NEWTON_SCREEN (s, nd);
		ns->space->gravity = cpv (0,(cpFloat)newtonGetGravity (d)/50000);
    }
}

/**
  *Handles changes of the friction constant configuration.
  */
static void
newtonFrictionNotify (CompDisplay *d,
		      CompOption *opt,
		      NewtonDisplayOptions num)
{
    CompScreen *s;
    NewtonScreen *ns;
    NEWTON_DISPLAY (d);
    for (s = d->screens ; s ; s = s->next)
    {
		ns = GET_NEWTON_SCREEN (s, nd);
		ns->space->damping = (cpFloat)newtonGetFrictionConstant (d);
    }
}

/**
 * Callback function when a window is hovered
 * Stops movement of that window
 */
static void
newtonHoveredWindow(cpShape *shape
					,void *data
					)
{
	CompWindow *w = (CompWindow *) shape->body->data;
	NEWTON_WINDOW(w);


	nw->body->velocity_func = newtonNoVelocityUpdate;
	nw->body->position_func = newtonNoPositionUpdate;
	nw->wasHovered = TRUE;
}

/**
 * Callback function to pin the window
 * Add a join at the current mouse position
 */
static void
newtonDoPinWindow(cpShape *shape
		, void *data
		)
{
	CompWindow *w = (CompWindow *) shape->body->data;
	NEWTON_WINDOW(w);
	NEWTON_SCREEN(w->screen);

	if (nw->bodyInSpace)
	{
		if (nw->joint)
		{
			cpSpaceRemoveJoint(ns->space, nw->joint);
			cpJointDestroy(nw->joint);
		}
		nw->joint = cpPivotJointNew(ns->staticBody,shape->body,ns->mousePos);
		cpSpaceAddJoint(ns->space,nw->joint);
	}
}
static Bool
newtonPinWindow(CompDisplay     *d,
		CompAction      *action,
		CompActionState state,
		CompOption      *option,
		int             nOption)
{
	CompScreen *s;
	NewtonScreen *ns;
	NEWTON_DISPLAY (d);

	for (s = d->screens ; s ; s = s->next)
	{
		ns = GET_NEWTON_SCREEN (s, nd);
		cpSpaceShapePointQuery(ns->space, ns->mousePos, newtonDoPinWindow,NULL);
	}
	return TRUE;
}

/**
 * Removes all joints from the window
 */
static Bool
newtonRemoveJoints(CompDisplay     *d,
		   CompAction      *action,
		   CompActionState state,
		   CompOption      *option,
		   int             nOption)
{
	CompWindow *w = findWindowAtDisplay (d, d->activeWindow);
	if (!w)
		return TRUE;
	NEWTON_WINDOW (w);
	NEWTON_SCREEN(w->screen);
	if (nw->joint)
	{
		cpSpaceRemoveJoint(ns->space, nw->joint);
		cpJointDestroy(nw->joint);
	}
	return TRUE;
}

/**
  *Applies the forces to the movable windows and iterates the physic model.
  *The window position are then updated.
  */
static void
newtonPreparePaintScreen (CompScreen *s ///Screen to be treated
			  ,int	    msSinceLastPaint /// time since last iteration in milliseconds
			  )
{
    CompWindow *w;
    NewtonWindow *nw;
    XPoint points[4];
    int radius;

    NEWTON_SCREEN (s);
    NEWTON_DISPLAY (s->display);

    //mouse position
    if ( (newtonGetMouseAttraction (s->display) || newtonGetMouseStop (s->display))
	  && !ns->pollHandle)
	{
		//(*nd->mpFunc->getCurrentPosition) (s, &ns->mousePos.x, &ns->mousePos.y);
		ns->pollHandle = (*nd->mpFunc->addPositionPolling) (s, positionUpdate);
	}

    //calculate forces
    for (w = s->windows; w; w = w->next)
	{
		nw = GET_NEWTON_WINDOW (w,ns);

		if (nw->movable)
		{
			cpBodyResetForces (nw->body);

			//---mouse attraction---
			if (newtonGetMouseAttraction (s->display))
			{
				cpFloat d2;
				cpVect distance;
				distance = cpvsub(nw->body->p, ns->mousePos);
				d2 = cpvlengthsq(distance);
				d2 = (d2<100)?100:d2;
				cpBodyApplyForce (nw->body,
								  cpv ((cpFloat) nw->body->m*(nd->mouseInvert?-1:1)*newtonGetMouseStrength (s->display) * distance.x / (sqrt (d2) * d2),
								  (cpFloat) nw->body->m*(nd->mouseInvert?-1:1)*newtonGetMouseStrength (s->display) * distance.y / (sqrt (d2) * d2)),
								  cpvzero);
			}

			//---restore status if it was hovered---
			if (nw->wasHovered)
			{
				nw->wasHovered = FALSE;
				nw->body->velocity_func = cpBodyUpdateVelocity;
				nw->body->position_func = cpBodyUpdatePosition;
			}

			//---add some damage when the window is rotated---
			if (nw->body->a != 0.0f)
			{
				radius = (int) (sqrt (WIN_FULL_W(w)*WIN_FULL_W(w)+WIN_FULL_H(w)*WIN_FULL_H(w))/2);
				points[0].x = nw->body->p.x - radius ; points[0].y = nw->body->p.y - radius ;
				points[1].x = nw->body->p.x + radius ; points[1].y = nw->body->p.y - radius ;
				points[2].x = nw->body->p.x + radius ; points[2].y = nw->body->p.y + radius ;
				points[3].x = nw->body->p.x - radius ; points[3].y = nw->body->p.y + radius ;
				damageScreenRegion (s, XPolygonRegion (points, 4, WindingRule));
			}
		}
    }
    //---stop if under mouse---
    if (newtonGetMouseStop (s->display))
    	cpSpaceShapePointQuery(ns->space, ns->mousePos, newtonHoveredWindow, NULL);

    //---update positions---
    cpSpaceStep (ns->space, (cpFloat) msSinceLastPaint);

    for (w = s->windows; w; w = w->next)
    {
	nw = GET_NEWTON_WINDOW (w,ns);
        if (nw->movable && w->mapNum)
        {
			if (!nw->wasHovered && !w->grabbed)
			{
					moveWindow (w,
									(int)nw->body->p.x - WIN_FULL_X(w)-WIN_FULL_W(w)/2,
									(int)nw->body->p.y - WIN_FULL_Y(w)-WIN_FULL_H(w)/2,
									TRUE,
									FALSE);
					syncWindowPosition (w);
			} else
			{
					nw->body->p = WINDOW_POSITION(w) ;
			}
        }
    }

    UNWRAP (ns, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, msSinceLastPaint);
    WRAP (ns, s, preparePaintScreen, newtonPreparePaintScreen);
}

static void
newtonDonePaintScreen (CompScreen *s)
{
	NEWTON_SCREEN(s);
	if (ns->physWindows != 0)
		damagePendingOnScreen (s);

	UNWRAP (ns, s, donePaintScreen);
	(*s->donePaintScreen) (s);
	WRAP (ns, s, donePaintScreen, newtonDonePaintScreen);
}

/**
 * Paints the window if it is rotated
 */
static Bool
newtonPaintWindow (CompWindow * w,
		   const WindowPaintAttrib * attrib,
		   const CompTransform    *transform,
		   Region region, unsigned int mask)
 {
    Bool status;

    NEWTON_SCREEN(w->screen);
    NEWTON_WINDOW(w);

    if (nw->body->a != 0.0f)
    {
        CompTransform wTransform = *transform;

        matrixTranslate ( &wTransform, nw->body->p.x, nw->body->p.y, 0);
        matrixRotate ( &wTransform, nw->body->a * 180/M_PI, 0.0f, 0.0f, 1.0f);
        matrixTranslate ( &wTransform,
                         - (nw->body->p.x),
                         - (nw->body->p.y), 0);

		mask |= PAINT_WINDOW_TRANSFORMED_MASK;

        UNWRAP(ns, w->screen, paintWindow);
        status = (*w->screen->paintWindow) (w, attrib, &wTransform, region,mask);
        WRAP(ns, w->screen, paintWindow, newtonPaintWindow);
    } else {
        UNWRAP(ns, w->screen, paintWindow);
		status = (*w->screen->paintWindow) (w, attrib, transform, region, mask);
		WRAP(ns, w->screen, paintWindow, newtonPaintWindow);
    }


    return status;
}

/**
  *#NewtonWindow constructor.
  */
static Bool
newtonInitWindow (CompPlugin *p,
		  CompWindow *w)
{
    NewtonWindow *nw;

    NEWTON_SCREEN (w->screen);

    nw = malloc (sizeof (NewtonWindow));
    if (!nw)
		return FALSE;

    w->base.privates[ns->windowPrivateIndex].ptr = nw;

    cpVect verts[] = WINDOW_BORDER(w);

    nw->body = cpBodyNew (WINDOW_MASS(w), INFINITY);

    //this pointer is used to know from a body which window it represents
    nw->body->data = (void *) w;
    nw->body->p = WINDOW_POSITION(w) ;

    nw->shape = cpPolyShapeNew (nw->body,
    				4,
    				verts,
    				cpvzero);

    nw->shape->e = newtonGetBounceConstant (w->screen->display);
    nw->shape->u = 0.5;

    nw->repulsion = FALSE;
    nw->wasHovered = FALSE;
    nw->movable = FALSE;
    nw->rotate = FALSE;
    nw->bodyInSpace = FALSE;
    nw->joint = NULL;

    return TRUE;
}

/**
  *#NewtonWindow destructor.
  */
static void
newtonFiniWindow (CompPlugin *p,
		  CompWindow *w)
{
    NEWTON_WINDOW (w);

    newtonSetNoPhysics (w);
    cpBodyFree (nw->body);
    cpShapeFree (nw->shape);
    free (nw);
}

/**
  *#NewtonDisplay constructor.
  */
static Bool
newtonInitDisplay (CompPlugin  *p,
		   CompDisplay *d)
{
    NewtonDisplay *nd;
    int mousepollIndex;

    if (!checkPluginABI ("core", CORE_ABIVERSION) ||
        !checkPluginABI ("mousepoll", MOUSEPOLL_ABIVERSION))
		return FALSE;

    if (!getPluginDisplayIndex (d, "mousepoll", &mousepollIndex))
		return FALSE;

    nd = malloc (sizeof (NewtonDisplay));
    if (!nd)
		return FALSE;

    nd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (nd->screenPrivateIndex < 0)
    {
		free (nd);
		return FALSE;
    }

    nd->mpFunc = d->base.privates[mousepollIndex].ptr;

    nd->mouseInvert = newtonGetMouseAttractionMode (d);

    newtonSetGravityNotify (d, newtonGravityNotify);
    newtonSetBounceConstantNotify (d, newtonElasticityNotify);
    newtonSetFrictionConstantNotify (d, newtonFrictionNotify);
    newtonSetTriggerPhysicsKeyInitiate (d, newtonTogglePhysics);
    newtonSetTriggerRotationKeyInitiate (d, newtonToggleRotation);
    newtonSetMouseInvertKeyInitiate (d, newtonMouseInvert);
    newtonSetMouseInvertKeyTerminate (d, newtonMouseInvert);
    newtonSetPinKeyInitiate (d, newtonPinWindow);
    newtonSetFreeWindowKeyInitiate (d, newtonRemoveJoints);

    //WRAP (nd, d, handleEvent, newtonHandleEvent);

    d->base.privates[displayPrivateIndex].ptr = nd;

    return TRUE;
}

/**
  *#NewtonDisplay destructor.
  */
static void
newtonFiniDisplay (CompPlugin  *p,
		   CompDisplay *d)
{
    NEWTON_DISPLAY (d);

    freeScreenPrivateIndex (d, nd->screenPrivateIndex);

    //UNWRAP (nd, d, handleEvent);

    free (nd);
}

/**
  *#NewtonScreen constructor.
  */
static Bool
newtonInitScreen (CompPlugin *p,
		  CompScreen *s)
{
    NewtonScreen *ns;
    cpShape *shape;
    cpBody *staticBody;
    REGION *region;
    int i;

    NEWTON_DISPLAY (s->display);

    ns = malloc (sizeof (NewtonScreen));
    if (!ns)
		return FALSE;

    ns->windowPrivateIndex = allocateWindowPrivateIndex (s);

    ns->pollHandle = 0;

    ns->space = cpSpaceNew ();
    ns->space->gravity = cpv(0, newtonGetGravity (s->display)/50000);
    ns->space->damping = newtonGetFrictionConstant (s->display);
    ns->space->elasticIterations = 10;

    staticBody = cpBodyNew(INFINITY, INFINITY);
    ns->staticBody = staticBody;

    shape = cpSegmentShapeNew (staticBody, cpv (s->x,s->y), cpv (s->x + s->width ,s->y), 0.0f);
    shape->e = 1.0; shape->u = 0.5;
    cpSpaceAddStaticShape(ns->space, shape);

    shape = cpSegmentShapeNew (staticBody, cpv (s->x + s->width,s->y), cpv (s->x + s->width,s->y + s->height), 0.0f);
    shape->e = 1.0; shape->u = 0.5;
    cpSpaceAddStaticShape(ns->space, shape);

    shape = cpSegmentShapeNew (staticBody, cpv (s->x + s->width,s->y + s->height), cpv (s->x,s->y + s->height), 0.0f);
    shape->e = 1.0; shape->u = 0.5;
    cpSpaceAddStaticShape(ns->space, shape);

    shape = cpSegmentShapeNew (staticBody, cpv (s->x,s->y + s->height), cpv (s->x,s->y), 0.0f);
    shape->e = 1.0; shape->u = 0.5;
    cpSpaceAddStaticShape(ns->space, shape);

    region = XCreateRegion ();
    XUnionRegion (region, &s->region, region);
    for (i=0; i < s->nOutputDev ; i++)
    {
		XSubtractRegion (region, &s->outputDev[i].region, region);
    }
    for (i=0; i< region->numRects ; i++)
    {
		shape = cpSegmentShapeNew (staticBody,
					   cpv (region->rects[i].x1,region->rects[i].y1),
					   cpv (region->rects[i].x1,region->rects[i].y2), 0.0f);
		shape->e = 1.0; shape->u = 0.5;
		cpSpaceAddStaticShape(ns->space, shape);

		shape = cpSegmentShapeNew (staticBody,
					   cpv (region->rects[i].x1,region->rects[i].y2),
					   cpv (region->rects[i].x2,region->rects[i].y2), 0.0f);
		shape->e = 1.0; shape->u = 0.5;
		cpSpaceAddStaticShape(ns->space, shape);

		shape = cpSegmentShapeNew (staticBody,
					   cpv (region->rects[i].x2,region->rects[i].y2),
					   cpv (region->rects[i].x2,region->rects[i].y1), 0.0f);
		shape->e = 1.0; shape->u = 0.5;
		cpSpaceAddStaticShape(ns->space, shape);

		shape = cpSegmentShapeNew (staticBody,
					   cpv (region->rects[i].x2,region->rects[i].y1),
					   cpv (region->rects[i].x1,region->rects[i].y1), 0.0f);
		shape->e = 1.0; shape->u = 0.5;
		cpSpaceAddStaticShape(ns->space, shape);
    }

    WRAP (ns, s, preparePaintScreen, newtonPreparePaintScreen);
    WRAP (ns, s, paintWindow, newtonPaintWindow);
    WRAP (ns, s, windowResizeNotify, newtonWindowResizeNotify);
    WRAP (ns, s, windowGrabNotify, newtonGrabNotify);
    WRAP (ns, s, windowUngrabNotify, newtonUngrabNotify);
    WRAP (ns, s, donePaintScreen, newtonDonePaintScreen);
    s->base.privates[nd->screenPrivateIndex].ptr = ns;

    return TRUE;
}

/**
  *#NewtonScreen destructor.
  */
static void
newtonFiniScreen (CompPlugin *p,
		  CompScreen *s)
{
    NEWTON_SCREEN (s);
    NEWTON_DISPLAY (s->display);

    freeWindowPrivateIndex (s, ns->windowPrivateIndex);

    cpSpaceFree (ns->space);

    if (ns->pollHandle)
		(*nd->mpFunc->removePositionPolling) (s, ns->pollHandle);

    UNWRAP (ns, s, preparePaintScreen);
    UNWRAP (ns, s, paintWindow);
    UNWRAP (ns, s, windowResizeNotify);
    UNWRAP (ns, s, windowGrabNotify);
    UNWRAP (ns, s, windowUngrabNotify);
    UNWRAP (ns, s, donePaintScreen);

    free (ns);
}

/**
  *Registers the constructors.
  */
static CompBool
newtonInitObject (CompPlugin *p,
		  CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) newtonInitDisplay,
	(InitPluginObjectProc) newtonInitScreen,
	(InitPluginObjectProc) newtonInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

/**
  *Registers the destructors
  */
static void
newtonFiniObject (CompPlugin *p,
		  CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) newtonFiniDisplay,
	(FiniPluginObjectProc) newtonFiniScreen,
	(FiniPluginObjectProc) newtonFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

/**
  *Plugin initialisation.
  */
static Bool
newtonInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
		return FALSE;
    cpInitChipmunk ();

    return TRUE;
}

/**
  *Plugin termination.
  */
static void
newtonFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}


CompPluginVTable newtonVTable = {
    "newton",
    0,//GetMetadata,
    newtonInit,
    newtonFini,
    newtonInitObject,
    newtonFiniObject,
    0,//GetObjectOptions,
    0,//SetObjectOption
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &newtonVTable;
}
