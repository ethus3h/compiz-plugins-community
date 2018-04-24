/*
 * Compiz Visual Event plugin
 *
 * Author : Guillaume Seguin
 * Email : guillaume@segu.in
 *
 * Copyright (c) 2007 Guillaume Seguin <guillaume@segu.in>
 *
 * Using some Cairo bits from wall.c:
 * Copyright (c) 2007 Robert Carr <racarr@beryl-project.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <compiz-core.h>

#include <cairo-xlib-xrender.h>
#include "compiz-cairo.h"

#include <pthread.h>

#include <X11/extensions/record.h>
#include <X11/Xlibint.h>

#include "visualevent_options.h"

/*
 * Buttons and keys states are stored in Bool arrays and indexed by our own
 * indexes defined here. Keycodes are stored in an int array indexed by the
 * same indexes than for keys states.
 */

enum
{
    Button_1 = 0,
    Button_2,
    Button_3,
    Buttons_Num
} VisualEventButton;

enum
{
    Control_L = 0,
    Control_R,
    Alt_L,
    Alt_R,
    Shift_L,
    Shift_R,
    Super_L,
    Super_R,
    L3_Shift,
    Keys_Num
} VisualEventKey;

typedef enum
{
    StateReleased = 0,
    StatePressed
} EventState;

/* Stolen to Xnee
 * This should really be in record.h */
typedef union
{
    unsigned char    type;
    xEvent           event;
    xResourceReq     req;
    xGenericReply    reply;
    xError           error;
    xConnSetupPrefix setup;
} XRecordDatum;

static int displayPrivateIndex;

typedef struct _VisualEventDisplay
{
    int			screenPrivateIndex;
    HandleEventProc	handleEvent;

    pthread_t		eventsThread;
    pthread_mutex_t	lock;
    Bool		endThread;
    XRecordContext	context;

    Bool		enabled;

    EventState		buttons[Buttons_Num];
    EventState		keys[Keys_Num];

    int			keycodes[Keys_Num];
    char *		keynames[Keys_Num];

    CompTimeoutHandle	timeoutHandles[Buttons_Num + Keys_Num];
} VisualEventDisplay;

typedef struct _VisualEventScreen
{
    PaintOutputProc paintOutput;

    CairoContext    *mouseContext;
    CairoContext    *keyboardContext;

    char	    *font;

    int		    keyx[Keys_Num];
    int		    keyy[Keys_Num];
    int		    keywidth[Keys_Num];
    int		    keyheight[Keys_Num];
} VisualEventScreen;

/*
 * This struct is used as closure for timeouts
 */
typedef struct _DisplayValue
{
    CompDisplay	    *display;
    unsigned int    value;
} DisplayValue;

#define GET_VISUALEVENT_DISPLAY(d)					    \
    ((VisualEventDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define VISUALEVENT_DISPLAY(d)						    \
    VisualEventDisplay *ved = GET_VISUALEVENT_DISPLAY (d)
#define GET_VISUALEVENT_SCREEN(s, ved)					    \
    ((VisualEventScreen *) (s)->base.privates[(ved)->screenPrivateIndex].ptr)
#define VISUALEVENT_SCREEN(s)						    \
    VisualEventScreen *ves = GET_VISUALEVENT_SCREEN (s,			    \
			     GET_VISUALEVENT_DISPLAY (s->display))

/* Cairo drawing ------------------------------------------------------------ */

/*
 * Draw a mouse using Cairo
 */
static void
visualevDrawMouse (CompScreen *s)
{
    VISUALEVENT_SCREEN (s);
    VISUALEVENT_DISPLAY (s->display);

    cairo_t *cr = ves->mouseContext->cr;
    int width = ves->mouseContext->width, height = ves->mouseContext->height;
    int x0, x1, y0, y1, y2, mx, my1, my2, w;
    int mbx, mby, mbw, mbh;
    unsigned short *border, *body, *color;

    border = visualeventGetBorderColor (s);
    body = visualeventGetBodyColor (s);

    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);

    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    
    x0 = 5;			    /* Left of the mouse */
    x1 = width - x0;		    /* Right of the mouse */
    y0 = 5;			    /* Top of the mouse */
    y1 = y0 + (height - y0) * 0.42; /* Middle of the mouse (top of the body) */
    y2 = height - y0;		    /* Bottom of the mouse */
    mx = (x0 + x1) / 2;		    /* Horizontal middle of the mouse */
    my1 = (y0 + y1) / 2;	    /* Middle of the top section */
    my2 = (y1 + y2) / 2;	    /* Middle of the bottom section */
    w = x1 - x0;		    /* Width of the mouse */
    /* Body */
    cairo_move_to (cr, x0, y1);
    cairo_curve_to (cr, x0, my2, x0, y2, mx, y2);
    cairo_curve_to (cr, x1, y2, x1, my2, x1, y1);
    cairo_set_source_rgba (cr, body[0] / 65535.0f, body[1] / 65535.0f,
			   body[2] / 65535.0f, body[3] / 65535.0f);
    cairo_fill_preserve (cr);
    cairo_set_source_rgba (cr, border[0] / 65535.0f, border[1] / 65535.0f,
			   border[2] / 65535.0f, border[3] / 65535.0f);
    cairo_stroke (cr);
    /* Left button */
    color = visualeventGetButton1Color (s);
    cairo_move_to (cr, mx, y1);
    cairo_line_to (cr, x0, y1);
    cairo_curve_to (cr, x0, my1, x0, y0, mx, y0);
    cairo_line_to (cr, mx, y1);
    if (ved->buttons[Button_1])
	cairo_set_source_rgba (cr, color[0] / 65535.0f, color[1] / 65535.0f,
			       color[2] / 65535.0f, color[3] / 65535.0f);
    else
	cairo_set_source_rgba (cr, body[0] / 65535.0f, body[1] / 65535.0f,
			       body[2] / 65535.0f, body[3] / 65535.0f);
    cairo_fill_preserve (cr);
    cairo_set_source_rgba (cr, border[0] / 65535.0f, border[1] / 65535.0f,
			   border[2] / 65535.0f, border[3] / 65535.0f);
    cairo_stroke (cr);
    /* Right button */
    color = visualeventGetButton3Color (s);
    cairo_move_to (cr, mx, y1);
    cairo_line_to (cr, x1, y1);
    cairo_curve_to (cr, x1, my1, x1, y0, mx, y0);
    cairo_line_to (cr, mx, y1);
    if (ved->buttons[Button_3])
	cairo_set_source_rgba (cr, color[0] / 65535.0f, color[1] / 65535.0f,
			       color[2] / 65535.0f, color[3] / 65535.0f);
    else
	cairo_set_source_rgba (cr, body[0] / 65535.0f, body[1] / 65535.0f,
			       body[2] / 65535.0f, body[3] / 65535.0f);
    cairo_fill_preserve (cr);
    cairo_set_source_rgba (cr, border[0] / 65535.0f, border[1] / 65535.0f,
			   border[2] / 65535.0f, border[3] / 65535.0f);
    cairo_stroke (cr);
    /* Middle button */
    color = visualeventGetButton2Color (s);
    mbw = width / 5;		/* Middle button width */
    mbh = height / 4;		/* Middle button height */
    mbx = x0 + w / 2 - mbw / 2;	/* Middle button left position */
    mby = y1 - mbh / 4 * 3;	/* Middle button top position */
    cairoDrawRoundedRectangle (cr, mbx, mby, mbw, mbh, mbw / 4, mbw / 12);
    if (ved->buttons[Button_2])
	cairo_set_source_rgba (cr, color[0] / 65535.0f, color[1] / 65535.0f,
			       color[2] / 65535.0f, color[3] / 65535.0f);
    else
	cairo_set_source_rgba (cr, body[0] / 65535.0f, body[1] / 65535.0f,
			       body[2] / 65535.0f, body[3] / 65535.0f);
    cairo_fill_preserve (cr);
    cairo_set_source_rgba (cr, border[0] / 65535.0f, border[1] / 65535.0f,
			   border[2] / 65535.0f, border[3] / 65535.0f);
    cairo_stroke (cr);
}

/*
 * Draw key
 */
static void
visualevDrawKey (CompScreen *s, int key, char *name)
{
    VISUALEVENT_SCREEN (s);
    cairo_t *cr = ves->keyboardContext->cr;
    int x, y, width, height;

    x = ves->keyx[key];
    y = ves->keyy[key];
    width = ves->keywidth[key];
    height = ves->keyheight[key];

    cairoDrawRoundedRectangle (cr, x, y, width, height, 15, 5);  
    cairo_set_source_rgba (cr, 0, 0, 0, 0.8);
    cairo_fill_preserve (cr);
    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_stroke (cr);

    cairoDrawText (cr, name, ves->font, x + 12, y + 13, NULL, NULL);
}

/*
 * Draw pressed keys using Cairo
 */
static void
visualevDrawKeyboard (CompScreen *s)
{
    int i;
    VISUALEVENT_DISPLAY (s->display);
    VISUALEVENT_SCREEN (s);
 
    cairo_t *cr = ves->keyboardContext->cr;

    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);

    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

    for (i = 0; i < Keys_Num; i++)
    {
	if (!ved->keys[i])
	    continue;
	visualevDrawKey (s, i, ved->keynames[i]);
    }
}

/* OpenGL drawing ----------------------------------------------------------- */

/*
 * Paint mouse & keyboard surfaces on output if enabled
 */
static Bool
visualevPaintOutput (CompScreen		     *s,
		     const ScreenPaintAttrib *sAttrib,
		     const CompTransform     *transform,
		     Region		     region,
		     CompOutput		     *output,
		     unsigned int	     mask)
{
    Bool status;
    CompTransform sTransform = *transform;
    int x, y, i;
    Bool drawMouse, drawKeyboard;

    VISUALEVENT_SCREEN (s);
    VISUALEVENT_DISPLAY (s->display);

    drawMouse = FALSE;
    for (i = 0; i < Buttons_Num; i++)
    {
	if (ved->buttons[i])
	{
	    drawMouse = TRUE;
	    break;
	}
    }

    drawKeyboard = FALSE;
    for (i = 0; i < Keys_Num; i++)
    {
	if (ved->keys[i])
	{
	    drawKeyboard = TRUE;
	    break;
	}
    }

    UNWRAP (ves, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
    WRAP (ves, s, paintOutput, visualevPaintOutput);

    if (!ved->enabled || (!drawMouse && !drawKeyboard))
	return status;

    transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);

    glPushMatrix ();
    glLoadMatrixf (sTransform.m);

    if (drawMouse)
    {
	x = output->width - ves->mouseContext->width - 
	    visualeventGetMouseRightMargin (s);
	y = output->height - ves->mouseContext->height -
	    visualeventGetMouseBottomMargin (s);

	drawCairoTexture (s, output, ves->mouseContext, x, y);
    }

    if (drawKeyboard)
    {
	x = visualeventGetKeyboardLeftMargin (s);
	y = output->height - ves->keyboardContext->height -
	    visualeventGetKeyboardBottomMargin (s);

	drawCairoTexture (s, output, ves->keyboardContext, x, y);
    }

    glPopMatrix ();

    return status;
}

/* Key/Button events handling ----------------------------------------------- */

/*
 * Disable a timeout if it exists
 */
static void
visualevRemoveTimeout (CompDisplay *d,
		       unsigned int index)
{
    VISUALEVENT_DISPLAY (d);

    if (ved->timeoutHandles[index])
    {
	compRemoveTimeout (ved->timeoutHandles[index]);
	ved->timeoutHandles[index] = 0;
    }
}

/*
 * Find a key from its keycode
 */
static int
visualevFindKey (CompDisplay *d,
		 unsigned int keycode)
{
    int key, i;
    VISUALEVENT_DISPLAY (d);

    key = -1;
    for (i = 0; i < Keys_Num; i++)
    {
	if (ved->keycodes[i] == keycode)
	{
	    key = i;
	    break;
	}
    }

    return key;
}

/*
 * Update key state
 */
static void
visualevKeyStateChanged (CompDisplay *d,
			 unsigned int keycode,
			 EventState state)
{
    CompScreen *s;
    int key;
    VISUALEVENT_DISPLAY (d);

    /* Try to find the key */
    key = visualevFindKey (d, keycode);

    /* Just return if key is unknown */
    if (key == -1)
	return;

    /* Remove timeout after KeyPress */
    if (state == StatePressed)
	visualevRemoveTimeout (d, Buttons_Num + key);

    /* Don't do anything if state hasn't changed */
    if (state == ved->keys[key])
	return;

    ved->keys[key] = state;

    if (!ved->enabled)
	return;

    /* Redraw and damage if enabled */
    for (s = d->screens; s; s = s->next)
    {
	visualevDrawKeyboard (s);
	damageScreen (s);
    }
}

/*
 * Callback for key release timeout
 */
static Bool visualevHandleKeyTimeout (void *data)
{
    DisplayValue *dv = (DisplayValue *) data;
    visualevKeyStateChanged (dv->display, dv->value, StateReleased);
    free (dv);
    return FALSE;
}

/*
 * A key was pressed, update state
 */
static void
visualevKeyPressed (CompDisplay *d,
		    unsigned int keycode)
{
    visualevKeyStateChanged (d, keycode, StatePressed);
}

/*
 * A key was released, update state if disabled or add timed callback
 */
static void
visualevKeyReleased (CompDisplay *d,
		     unsigned int keycode)
{
    VISUALEVENT_DISPLAY (d);
    int key = visualevFindKey (d, keycode);
    int index = key + Buttons_Num;
    int timeout;
    CompTimeoutHandle handle;

    if (key == -1)
	return;

    if (!ved->enabled)
    {
	visualevKeyStateChanged (d, keycode, StateReleased);
	return;
    }

    /* Remove previous timeout before setting up a new one */
    visualevRemoveTimeout (d, Buttons_Num + key);

    DisplayValue *dv = malloc (sizeof (DisplayValue));
    if (!dv)
	return;
    dv->display = d;
    dv->value = keycode;

    timeout = visualeventGetTimeout (d) * 1000;
    handle = compAddTimeout (timeout, timeout + 200,
			     visualevHandleKeyTimeout, dv);
    ved->timeoutHandles[index] = handle;
}

/*
 * Find a button index
 */
static int
visualevFindButton (unsigned int button)
{
    int index;

    switch (button)
    {
	case Button1:
	    index = Button_1;
	    break;
	case Button2:
	    index = Button_2;
	    break;
	case Button3:
	    index = Button_3;
	    break;
	default:
	    return -1;
    }

    return index;
}

/*
 * Update button state
 */
static void
visualevButtonStateChanged (CompDisplay *d,
			    unsigned int button,
			    EventState state)
{
    CompScreen *s;
    unsigned int index;
    VISUALEVENT_DISPLAY (d);

    index = visualevFindButton (button);

    if (index == -1)
	return;

    /* Remove timeout after a ButtonPress */
    if (state == StatePressed)
	visualevRemoveTimeout (d, index);

    /* Don't do anything if state hasn't changed */
    if (state == ved->buttons[index])
	return;

    ved->buttons[index] = state;

    if (!ved->enabled)
	return;

    /* Redraw and damage if enabled */
    for (s = d->screens; s; s = s->next)
    {
	visualevDrawMouse (s);
	damageScreen (s);
    }
}

/*
 * Callback for button release timeout
 */
static Bool visualevHandleButtonTimeout (void *data)
{
    DisplayValue *dv = (DisplayValue *) data;
    visualevButtonStateChanged (dv->display, dv->value, StateReleased);
    free (dv);
    return FALSE;
}

/*
 * A button was pressed, update state
 */
static void
visualevButtonPressed (CompDisplay *d,
		       unsigned int button)
{
    visualevButtonStateChanged (d, button, StatePressed);
}

/*
 * A button was released, update state if disabled or add timed callback
 */
static void
visualevButtonReleased (CompDisplay *d,
			unsigned int button)
{
    VISUALEVENT_DISPLAY (d);
    int index = visualevFindButton (button);
    CompTimeoutHandle handle;
    int timeout;

    if (index == -1)
	return;

    if (!ved->enabled)
    {
	visualevButtonStateChanged (d, button, StateReleased);
	return;
    }

    /* Remove previous timeout before setting up a new one */
    visualevRemoveTimeout (d, index);

    DisplayValue *dv = malloc (sizeof (DisplayValue));
    if (!dv)
	return;
    dv->display = d;
    dv->value = button;

    timeout = visualeventGetTimeout (d) * 1000;
    handle = compAddTimeout (timeout, timeout + 200,
			     visualevHandleButtonTimeout, dv);
    ved->timeoutHandles[index] = handle;
}

/*
 * Process an event knowing type and the "detail" value, which is either
 * the keycode or the button number)
 */
static void
visualevProcessEvent (CompDisplay *d,
		      unsigned int type,
		      int detail)
{
    switch (type)
    {
	case KeyPress:
	    visualevKeyPressed (d, detail);
	    break;
	case KeyRelease:
	    visualevKeyReleased (d, detail);
	    break;
	case ButtonPress:
	    visualevButtonPressed (d, detail);
	    break;
	case ButtonRelease:
	    visualevButtonReleased (d, detail);
	    break;
	default:
	    break;
    }
}

/*
 * Handle an event received from Record extension
 */
static void
visualevHandleRecordEvent (XPointer pointer, XRecordInterceptData *data)
{
    CompDisplay	    *display;
    XRecordDatum    *datum;

    display = (CompDisplay *) pointer;

    if (!data)
	return;
    else if (!data->data || data->data_len < 1 || 
	     data->category != XRecordFromServer)
    {
	XRecordFreeData (data);
	return;
    }

    datum = (XRecordDatum *) data->data;

    switch (datum->type)
    {
	case KeyPress:
	case KeyRelease:
	case ButtonPress:
	case ButtonRelease:
	    visualevProcessEvent (display, datum->type,
				  datum->event.u.u.detail);
	default:
	    XRecordFreeData (data);
	    return;
    }
}

/*
 * Events thread main loop
 * In order to use Record extension, we open a new connection to the display,
 * create a context and enable it asynchronously.
 * We then process the replies in a loop, locking the mutex and checking for
 * termination on each loop.
 */
static void * 
visualevEventsThread (void *data)
{
    CompDisplay *d = (CompDisplay *) data;
    Display *display;
    XRecordRange *range;
    XRecordClientSpec client_spec = XRecordAllClients; 
    VISUALEVENT_DISPLAY (d);

    display = XOpenDisplay (d->display->display_name);

    if (!display)
    {
	compLogMessage ("visualevent", CompLogLevelError,
			"Could not open display %s.",
			d->display->display_name);
	return NULL;
    }

    range = XRecordAllocRange ();
    if (!range)
    {
	compLogMessage ("visualevent", CompLogLevelError,
			"Could not allocate record range.");
	XCloseDisplay (display);
	return NULL;
    }

    range->device_events.first = KeyPress;
    range->device_events.last = ButtonRelease;

    ved->context = XRecordCreateContext (display,
					 XRecordFromServerTime | 
					 XRecordFromClientTime |
					 XRecordFromClientSequence,
					 &client_spec,
					 1,
					 &range,
					 1);   

    if (!ved->context)
    {
	compLogMessage ("visualevent", CompLogLevelError,
			"Could not create record context.");
	XFree (range);
	XCloseDisplay (display);
	return NULL;
    }

    if (!XRecordEnableContextAsync (display, ved->context,
				    visualevHandleRecordEvent, (void *) d))
    {
	compLogMessage ("visualevent", CompLogLevelError,
			"Could not enable record context.");
	XRecordFreeContext (display, ved->context);
	XFree (range);
	XCloseDisplay (display);
	return NULL;
    }

    ved->context = 0;

    while (TRUE)
    {
	pthread_mutex_lock (&ved->lock);
	if (ved->endThread)
	{
	    pthread_mutex_unlock (&ved->lock);
	    break;
	}
	pthread_mutex_unlock (&ved->lock);
	XRecordProcessReplies (display);
	usleep (10000);
    }

    XRecordProcessReplies (display);
    XRecordDisableContext (display, ved->context);
    XRecordFreeContext (display, ved->context);
    XFree (range);

    /* FIXME Closing the display just freezes Compiz...
     * XCloseDisplay (display); */

    return NULL;
}

/*
 * Initialise events thread
 */
static Bool
visualevInitThread (CompDisplay *d)
{
    int status;
    VISUALEVENT_DISPLAY (d);

    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);

    status = pthread_create (&ved->eventsThread, &attr,
			     visualevEventsThread, (void *) d);

    pthread_attr_destroy (&attr);

    if (status)
    {
	compLogMessage ("visualevent", CompLogLevelFatal,
			"Could not start thread (error code: %d).", status);
	return FALSE;
    }

    return TRUE;
}

/*
 * Handle events from Compiz
 *
 * This isn't really interesting since Compiz is only getting events for keys
 * and buttons which were passively grabbed before, while we need all the 
 * events...
 */
/*
static void
visualevHandleEvent (CompDisplay *d, XEvent *event)
{
    VISUALEVENT_DISPLAY (d);

    UNWRAP (ved, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (ved, d, handleEvent, visualevHandleEvent);

    switch (event->type)
    {
	case KeyPress:
	case KeyRelease:
	    visualevProcessEvent (d, event->type,
				  event->xkey.keycode);
	    break;
	case ButtonPress:
	case ButtonRelease:
	    visualevProcessEvent (d, event->type,
				  event->xbutton.button);
	    break;
	default:
	    break;
    }

    UNWRAP (ved, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (ved, d, handleEvent, visualevHandleEvent);
}
*/

/*
 * Toggle on screen display
 */
static Bool
visualevToggle (CompDisplay *d,
		CompAction * action,
		CompActionState state,
		CompOption * option, int nOption)
{
    CompScreen *s;

    VISUALEVENT_DISPLAY (d);

    ved->enabled = !ved->enabled;

    /* Redraw and damage if enabled */
    for (s = d->screens; s; s = s->next)
    {
	if (ved->enabled)
	{
	    visualevDrawKeyboard (s);
	    visualevDrawMouse (s);
	}
	damageScreen (s);
    }

    return FALSE;
}

/*
 * Redraw the mouse after an option changed
 */
static void
visualevRedrawMouse (CompScreen *s,
		     CompOption *opt,
		     VisualeventScreenOptions num)
{
    visualevDrawMouse (s);
}

/*
 * Fetch keycodes for our keysyms and set key names
 */
static void
visualevInitKeys (CompDisplay *d)
{
    VISUALEVENT_DISPLAY (d);

    ved->keycodes[Control_L] = XKeysymToKeycode (d->display, XK_Control_L);
    ved->keycodes[Control_R] = XKeysymToKeycode (d->display, XK_Control_R);
    ved->keycodes[Alt_L]     = XKeysymToKeycode (d->display, XK_Alt_L);
    ved->keycodes[Alt_R]     = XKeysymToKeycode (d->display, XK_Alt_R);
    ved->keycodes[Shift_L]   = XKeysymToKeycode (d->display, XK_Shift_L);
    ved->keycodes[Shift_R]   = XKeysymToKeycode (d->display, XK_Shift_R);
    ved->keycodes[Super_L]   = XKeysymToKeycode (d->display, XK_Super_L);
    ved->keycodes[Super_R]   = XKeysymToKeycode (d->display, XK_Super_R);
    ved->keycodes[L3_Shift]  = XKeysymToKeycode (d->display,
						 XK_ISO_Level3_Shift);

    ved->keynames[Control_L] = strdup ("Control L");
    ved->keynames[Control_R] = strdup ("Control R");
    ved->keynames[Alt_L]     = strdup ("Alt L");
    ved->keynames[Alt_R]     = strdup ("Alt R");
    ved->keynames[Shift_L]   = strdup ("Shift L");
    ved->keynames[Shift_R]   = strdup ("Shift R");
    ved->keynames[Super_L]   = strdup ("Super L");
    ved->keynames[Super_R]   = strdup ("Super R");
    ved->keynames[L3_Shift]  = strdup ("Alt Gr");
}

static int
getKeyWidth (cairo_t *cr, char *name, char *font)
{
    int width;
    cairoPrepareText (cr, name, font, &width, NULL);
    return width;
}

/*
 * Handy macro for getting a key width + drawing margins
 */
#define KEYWIDTH(key) getKeyWidth (cr, ved->keynames[key], ves->font) + 24

/*
 * Initialize key coordinates and size
 */
static void
visualevInitKeyCoords (CompScreen *s)
{
    VISUALEVENT_SCREEN (s);
    VISUALEVENT_DISPLAY (s->display);

    cairo_t *cr = ves->keyboardContext->cr;
    int height = ves->keyboardContext->height;
    int x, y;
    int margin = 2;
    int keyheight = height / 2 - margin;
    int i;

    x = y = 5;

    for (i = 0; i < Keys_Num; i++)
	ves->keyheight[i] = keyheight;

    if (ved->keycodes[Shift_L])
    {
	ves->keyx[Shift_L] = x;
	ves->keyy[Shift_L] = y;
	ves->keywidth[Shift_L] = KEYWIDTH (Shift_L);
    }

    y += keyheight + margin;

    if (ved->keycodes[Control_L])
    {
	ves->keyx[Control_L] = x;
	ves->keyy[Control_L] = y;
	ves->keywidth[Control_L] = KEYWIDTH (Control_L);
	x += margin + ves->keywidth[Control_L];
    }

    if (ved->keycodes[Super_L])
    {
	ves->keyx[Super_L] = x;
	ves->keyy[Super_L] = y;
	ves->keywidth[Super_L] = KEYWIDTH (Super_L);
	x += margin + ves->keywidth[Super_L];
    }

    if (ved->keycodes[Alt_L])
    {
	ves->keyx[Alt_L] = x;
	ves->keyy[Alt_L] = y;
	ves->keywidth[Alt_L] = KEYWIDTH (Alt_L);
	x += margin + ves->keywidth[Alt_L];
    }

    if (ved->keycodes[L3_Shift])
    {
	ves->keyx[L3_Shift] = x;
	ves->keyy[L3_Shift] = y;
	ves->keywidth[L3_Shift] = KEYWIDTH (L3_Shift);
	x += margin + ves->keywidth[L3_Shift];
    }

    if (ved->keycodes[Super_R])
    {
	ves->keyx[Super_R] = x;
	ves->keyy[Super_R] = y;
	ves->keywidth[Super_R] = KEYWIDTH (Super_R);
	x += margin + ves->keywidth[Super_R];
    }

    if (ved->keycodes[Alt_R])
    {
	ves->keyx[Alt_R] = x;
	ves->keyy[Alt_R] = y;
	ves->keywidth[Alt_R] = KEYWIDTH (Alt_R);
	x += margin + ves->keywidth[Alt_R];
    }

    if (ved->keycodes[Control_R])
    {
	ves->keyx[Control_R] = x;
	ves->keyy[Control_R] = y;
	ves->keywidth[Control_R] = KEYWIDTH (Control_R);
	x += margin + ves->keywidth[Control_R];
    }

    y -= keyheight + margin;

    if (ved->keycodes[Shift_R])
    {
	ves->keyy[Shift_R] = y;
	ves->keywidth[Shift_R] = KEYWIDTH (Shift_R);
	ves->keyx[Shift_R] = x - ves->keywidth[Shift_R];
    }
}

#undef KEYWIDTH

/* Internal stuff ----------------------------------------------------------- */

static Bool
visualevInitDisplay (CompPlugin *p, CompDisplay *d)
{
    VisualEventDisplay * ved;
    int record_ext_major, record_ext_minor;
    int i;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    if (!XRecordQueryVersion (d->display, &record_ext_major, &record_ext_minor))
    {
	compLogMessage ("visualevent", CompLogLevelFatal,
			"XRecord extension missing.");
	return FALSE;
    }

    compLogMessage ("visualevent", CompLogLevelInfo,
		    "XRecord extension %d.%d loaded.",
		    record_ext_major, record_ext_minor);

    ved = malloc (sizeof (VisualEventDisplay));
    if (!ved)
	return FALSE;

    ved->screenPrivateIndex = allocateScreenPrivateIndex (d);

    if (ved->screenPrivateIndex < 0)
    {
	free (ved);
	return FALSE;
    }

    ved->enabled	= FALSE;
    ved->context	= 0;
    ved->eventsThread	= 0;
    ved->endThread	= FALSE;

    for (i = 0; i < Buttons_Num; i++)
    {
	ved->buttons[i] = FALSE;
    }

    for (i = 0; i < Keys_Num; i++)
    {
	ved->keys[i] = FALSE;
	ved->keycodes[i] = 0;
	ved->keynames[i] = NULL;
    }

    for (i = 0; i < Buttons_Num + Keys_Num; i++)
    {
	ved->timeoutHandles[i] = 0;
    }

    pthread_mutex_init (&ved->lock, NULL);

    d->base.privates[displayPrivateIndex].ptr = ved;

    visualevInitKeys (d);

    if (!visualevInitThread (d))
    {
	pthread_mutex_destroy (&ved->lock);
	free (ved);
	return FALSE;
    }

    visualeventSetToggleKeyInitiate (d, visualevToggle);

    /* WRAP (ved, d, handleEvent, visualevHandleEvent); */

    return TRUE;
}

static void
visualevFiniDisplay (CompPlugin *p, CompDisplay *d)
{
    int status, i;
    VISUALEVENT_DISPLAY (d);

    /* Remove the timers to avoid bad crashs on unload */
    for (i = 0; i < Buttons_Num + Keys_Num; i++)
    {
	if (ved->timeoutHandles[i])
	{
	    compRemoveTimeout (ved->timeoutHandles[i]);
	    ved->timeoutHandles[i] = 0;
	}
    }

    /*
     * If thread is still there (the opposite would be seriously worrying),
     * lock the mutex, set termination flag, unlock the mutex and wait for
     * the thread.
     */
    if (ved->eventsThread)
    {
	pthread_mutex_lock (&ved->lock);

	ved->endThread = TRUE;

	pthread_mutex_unlock (&ved->lock);

	status = pthread_join (ved->eventsThread, NULL);
	if (status) 
	    compLogMessage ("visualevent", CompLogLevelError,
			    "pthread_join () failed (error code : %d).",
			    status);
    }

    pthread_mutex_destroy (&ved->lock);

    /* UNWRAP (ved, d, handleEvent); */

    for (i = 0; i < Keys_Num; i++)
    {
	if (ved->keynames[i])
	    free (ved->keynames[i]);
    }

    if (ved->screenPrivateIndex >= 0)
	freeScreenPrivateIndex (d, ved->screenPrivateIndex);

    free (ved);
}

static Bool
visualevInitScreen (CompPlugin * p, CompScreen * s)
{
    VisualEventScreen *ves;
    int i;

    VISUALEVENT_DISPLAY (s->display);

    ves = malloc (sizeof (VisualEventScreen));

    if (!ves)
	return FALSE;

    s->base.privates[ved->screenPrivateIndex].ptr = ves;

    for (i = 0; i < Keys_Num; i++)
    {
	ves->keyx[i] = 0;
	ves->keyy[i] = 0;
	ves->keywidth[i] = 0;
	ves->keyheight[i] = 0;
    }

    ves->font = "Sans 15";

    ves->mouseContext = (CairoContext*) malloc (sizeof (CairoContext));
    if (!ves->mouseContext) 
	return FALSE;

    ves->mouseContext->width = 100;
    ves->mouseContext->height = ves->mouseContext->width * 1.7;
    setupCairoContext (s, ves->mouseContext);
    visualevDrawMouse (s);

    ves->keyboardContext = (CairoContext*) malloc (sizeof (CairoContext));
    if (!ves->keyboardContext) 
	return FALSE;

    ves->keyboardContext->width = 700;
    ves->keyboardContext->height = 105;
    setupCairoContext (s, ves->keyboardContext);
    /* No need to draw this one right now, it would be empty */

    visualevInitKeyCoords (s);

    visualeventSetBorderColorNotify (s, visualevRedrawMouse);
    visualeventSetBodyColorNotify (s, visualevRedrawMouse);
    visualeventSetButton1ColorNotify (s, visualevRedrawMouse);
    visualeventSetButton2ColorNotify (s, visualevRedrawMouse);
    visualeventSetButton3ColorNotify (s, visualevRedrawMouse);

    WRAP (ves, s, paintOutput, visualevPaintOutput);

    return TRUE;
}

static void
visualevFiniScreen (CompPlugin * p, CompScreen * s)
{
    VISUALEVENT_SCREEN (s);

    destroyCairoContext (s, ves->mouseContext);
    destroyCairoContext (s, ves->keyboardContext);

    UNWRAP (ves, s, paintOutput);

    free (ves);
}

static CompBool
visualevInitObject (CompPlugin *p,
		    CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) visualevInitDisplay,
	(InitPluginObjectProc) visualevInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
visualevFiniObject (CompPlugin *p,
		    CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) visualevFiniDisplay,
	(FiniPluginObjectProc) visualevFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
visualevInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
visualevFini (CompPlugin *p)
{
    if (displayPrivateIndex >= 0)
	freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompPluginVTable visualevVTable =
{
    "visualevent",
    0,
    visualevInit,
    visualevFini,
    visualevInitObject,
    visualevFiniObject,
    NULL,
    NULL
};

CompPluginVTable * getCompPluginInfo (void)
{
    return &visualevVTable;
}

