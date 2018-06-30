/*
 * Compiz desktopclick plugin
 *
 * desktopclick.c
 *
 * Copyright (c) 2008 Sam Spilsbury <smspillaz@gmail.com>
 *
 * Based on code by vpswitch, thanks to:
 * Copyright (c) 2007 Dennis Kasprzyk <onestone@opencompositing.org>
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
 * TODO: cleanup indentation
 */

#include <compiz-core.h>
#include <string.h>
#include "dclick_options.h"

static int displayPrivateIndex;

typedef struct _VpSwitchDisplay
{
    HandleEventProc handleEvent;

    int activeMods;
    Bool active;
} VpSwitchDisplay;

#define GET_DCLICK_DISPLAY(d)					\
    ((VpSwitchDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define DCLICK_DISPLAY(d)			\
    VpSwitchDisplay * vd = GET_DCLICK_DISPLAY (d)

#define MAX_INFO 32

/* Option Code Handling  --------------------------------------------------- */

static int dclickModMaskFromEnum ( DclickClickModsEnum num )
{
	unsigned int mask = 0;

	switch (num)
	{
		case ClickModsControl:
			mask |= ControlMask;
			break;
		case ClickModsAlt:
			mask |= CompAltMask;
			break;
		case ClickModsShift:
			mask |= ShiftMask;
			break;
		case ClickModsSuper:
			mask |= CompMetaMask;
			break;
		case ClickModsControlAlt:
			mask |= ControlMask;
			mask |= CompAltMask;
			break;
		case ClickModsControlShift:
			mask |= ControlMask;
			mask |= ShiftMask;
			break;
		case ClickModsControlSuper:
			mask |= ControlMask;
			mask |= CompMetaMask;
			break;
		case ClickModsAltShift:
			mask |= ShiftMask;
			mask |= CompAltMask;
			break;
		case ClickModsAltSuper:
			mask |= ShiftMask;
			mask |= CompMetaMask;
			break;
		case ClickModsShiftSuper:
			mask |= ShiftMask;
			mask |= CompMetaMask;
			break;
		default:
			break;
	}

	return mask;

}

static int
dclickButtonFromEnum ( DclickClickButtonsEnum button)
{
	return button + 1;
}

/* Inter-plugin communication --------------------------------------- */

static Bool
dclickSendInfoToPlugin (CompDisplay *d, CompOption *argument, int nArgument, char *pluginName, char *actionName, Bool initiate)
{
	Bool pluginExists = FALSE;
	CompOption *options, *option;
	CompPlugin *p;
	CompObject *o;
	int nOptions;

	p = findActivePlugin (pluginName);
	o = compObjectFind (&core.base, COMP_OBJECT_TYPE_DISPLAY, NULL);

	if (!o)
		return FALSE;

	if (!p || !p->vTable->getObjectOptions)
	{
		compLogMessage ("desktopclick", CompLogLevelError,
		"Reporting plugin '%s' does not exist!", pluginName);
		return FALSE;
	}

	if (p && p->vTable->getObjectOptions)
	{
		options = (*p->vTable->getObjectOptions) (p, o, &nOptions);
		option = compFindOption (options, nOptions, actionName, 0);
		pluginExists = TRUE;
	}

	if (pluginExists)
	{

		if (initiate)
		{

		if (option && option->value.action.initiate)
		{
			(*option->value.action.initiate)  (d,
		                           &option->value.action,
		                           0,
		                           argument,
		                           nArgument);
		}
		else
		{
			compLogMessage ("desktopclick", CompLogLevelError,
			"Plugin '%s' does exist, but no option named '%s' was found!", pluginName, actionName);
			return FALSE;
		}
		}
		else
		{
		if (option && option->value.action.terminate)
		{
			(*option->value.action.terminate)  (d,
		                           &option->value.action,
		                           0,
		                           argument,
		                           nArgument);
		}
		else
		{
			compLogMessage ("desktopclick", CompLogLevelError,
			"Plugin '%s' does exist, but no option named '%s' was found!", pluginName, actionName);
			return FALSE;
		}
		}
	}

	return TRUE;
}

/* Event Handlers ------------------------------------------------ */

static Bool
dclickHandleDesktopButtonPress (CompDisplay    *d,
		   int buttonMask,
		   int win,
		   int root)
{
	CompScreen *s;

	s = findScreenAtDisplay(d, root);
	
	if (s)
	{

	DCLICK_DISPLAY (d);

	CompListValue *cModsEnum = dclickGetClickMods (d);
	CompListValue *cButtonEnum = dclickGetClickButtons (d);
	CompListValue *cPluginName = dclickGetPluginNames (d);
	CompListValue *cActionName = dclickGetActionNames (d);
	
	int nInfo;
	int iInfo = 0;

	if ((cModsEnum->nValue != cButtonEnum->nValue) ||
            (cModsEnum->nValue != cPluginName->nValue)  ||
            (cModsEnum->nValue != cActionName->nValue))
	{
	/* Options have not been set correctly */
	return FALSE;
	}

	nInfo = cModsEnum->nValue;
	iInfo = 0;

	/* Basically, here we check to see if the button clicked and the mods == the option, and if so call it*/

	while (nInfo-- && iInfo < MAX_INFO)
	{

			if ((buttonMask & dclickButtonFromEnum (cButtonEnum->value[iInfo].i)))
			{

				if ((cModsEnum->value[iInfo].i != ClickModsNone) && !(vd->activeMods & dclickModMaskFromEnum(cModsEnum->value[iInfo].i)))
					return FALSE; /* Mods not pressed*/

				CompOption arg[2];
				int nArg = 0;

				arg[nArg].name = "window";
				arg[nArg].type = CompOptionTypeInt;
				arg[nArg].value.i = win;

				nArg++;

				arg[nArg].name = "root";
				arg[nArg].type = CompOptionTypeInt;
				arg[nArg].value.i = root;

				nArg++;

				/* Ensure that the strings are there before we do anything stupid */
				if (cPluginName->value[iInfo].s && cActionName->value[iInfo].s)
				{
				dclickSendInfoToPlugin (d, arg, nArg, cPluginName->value[iInfo].s, cActionName->value[iInfo].s, TRUE);
				vd->active = TRUE;
				}
		      }

			iInfo++;

	}

	}

	return TRUE;
}


static Bool
dclickHandleDesktopButtonRelease (CompDisplay    *d,
		   int buttonMask,
		   int win,
		   int root)
{
	CompScreen *s;

	s = findScreenAtDisplay(d, root);

	if (s)
	{

	DCLICK_DISPLAY (d);
	
	fprintf(stderr, "Release action called\n");

	if (!vd->active)
	return FALSE; 	/* We aren't active, so don't call actions */

	CompListValue *cModsEnum = dclickGetClickMods (d);
	CompListValue *cButtonEnum = dclickGetClickButtons (d);
	CompListValue *cPluginName = dclickGetPluginNames (d);
	CompListValue *cActionName = dclickGetActionNames (d);
	CompListValue *cCallType = dclickGetCallType (d);
	
	//fprintf(stderr, "... And we're allowed to call it\n");

	int nInfo;
	int iInfo = 0;

	if ((cModsEnum->nValue != cButtonEnum->nValue) ||
            (cModsEnum->nValue != cPluginName->nValue)  ||
            (cModsEnum->nValue != cActionName->nValue) ||
            (cModsEnum->nValue != cCallType->nValue))
	{
	/* Options have not been set correctly */
	//fprintf(stderr, "... Options are not set correctly %i %i %i %i %i\n", cModsEnum->nValue, cButtonEnum->nValue, cPluginName->nValue, cActionName->nValue, cCallType->nValue);
	return FALSE;
	}

	nInfo = cModsEnum->nValue;
	iInfo = 0;

	/* Basically, here we check to see if the button clicked and the mods == the option, and if so call it*/

	while (nInfo-- && iInfo < MAX_INFO)
	{
	
	    //fprintf(stderr, "Call type is %i\n", cCallType->value[iInfo].i);

			if ((buttonMask & dclickButtonFromEnum (cButtonEnum->value[iInfo].i)) && cCallType->value[iInfo].i != CallTypeToggle)
			{

				CompOption arg[2];
				int nArg = 0;

				arg[nArg].name = "window";
				arg[nArg].type = CompOptionTypeInt;
				arg[nArg].value.i = win;

				nArg++;

				arg[nArg].name = "root";
				arg[nArg].type = CompOptionTypeInt;
				arg[nArg].value.i = root;

				nArg++;

				/* Ensure that the strings are there before we do anything stupid */
				if (cPluginName->value[iInfo].s && cActionName->value[iInfo].s)
				{
				dclickSendInfoToPlugin (d, arg, nArg, cPluginName->value[iInfo].s, cActionName->value[iInfo].s, FALSE);
				//vd->active = FALSE;
				}
		      }

			iInfo++;

	}

	}

	return TRUE;
}


static void
dclickHandleEvent (CompDisplay *d,
	  	     XEvent      *event)
{
    DCLICK_DISPLAY (d);

    switch (event->type)
    {

    case ButtonPress:
	dclickHandleDesktopButtonPress(d, event->xbutton.button, event->xbutton.window, event->xbutton.root);
	break;
    case ButtonRelease:
	dclickHandleDesktopButtonRelease(d, event->xbutton.button, event->xbutton.window, event->xbutton.root);
	break;
    default:
	break;    
    }

	if (event->type == d->xkbEvent)
	{
		XkbAnyEvent *xkbEvent = (XkbAnyEvent *) event;

		if (xkbEvent->xkb_type == XkbStateNotify)
		{
			XkbStateNotifyEvent *stateEvent = (XkbStateNotifyEvent *) event;

			vd->activeMods = stateEvent->mods;
		}
	}

    UNWRAP (vd, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (vd, d, handleEvent, dclickHandleEvent);
}

/* Standard Plugin initialization --------------------------------------- */

static Bool
dclickInitDisplay (CompPlugin  *p,
		     CompDisplay *d)
{
    VpSwitchDisplay *vd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    vd = malloc (sizeof (VpSwitchDisplay));
    if (!vd)
	return FALSE;

    WRAP (vd, d, handleEvent, dclickHandleEvent);

    d->base.privates[displayPrivateIndex].ptr = vd;

    return TRUE;
}

static void
dclickFiniDisplay(CompPlugin  *p,
	  	    CompDisplay *d)
{
    DCLICK_DISPLAY (d);

    UNWRAP (vd, d, handleEvent);

    free (vd);
}

static CompBool
dclickInitObject (CompPlugin *p,
		    CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) dclickInitDisplay
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
dclickFiniObject (CompPlugin *p,
		    CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) dclickFiniDisplay,
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
dclickInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
dclickFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}

CompPluginVTable dclickVTable = {
    "dclick",
    0,
    dclickInit,
    dclickFini,
    dclickInitObject,
    dclickFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &dclickVTable;
}
