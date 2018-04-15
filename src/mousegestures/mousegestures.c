/*
 * Copyright © 2006 Mike Dransfield
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * Author: Mike Dransfield <mike@blueroot.co.uk>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <compiz-core.h>

#define MOUSEGESTURES_DISPLAY_OPTION_INITIATE_BUTTON 0
#define MOUSEGESTURES_DISPLAY_OPTION_DETECT_DIAGONAL 1
#define MOUSEGESTURES_DISPLAY_OPTION_GESTURE         2
#define MOUSEGESTURES_DISPLAY_OPTION_PLUGIN          3
#define MOUSEGESTURES_DISPLAY_OPTION_ACTION          4
#define MOUSEGESTURES_DISPLAY_OPTION_NUM             5

#define MOUSEGESTURE_MIN_DELTA                   80
#define MOUSEGESTURE_MAX_VARIANCE                (MOUSEGESTURE_MIN_DELTA / 2)
#define MOUSEGESTURES_MAX_STROKE		 32
#define MOUSEGESTURES_MAX_GESTURES		 128

#define M_UP    'u'
#define M_RIGHT 'r'
#define M_DOWN  'd'
#define M_LEFT  'l'
#define M_9     '9'
#define M_3     '3'
#define M_1     '1'
#define M_7     '7'

#define GET_MOUSEGESTURES_DISPLAY(d) \
    ((MousegesturesDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define MOUSEGESTURES_DISPLAY(d) \
    MousegesturesDisplay *md = GET_MOUSEGESTURES_DISPLAY (d)
#define GET_MOUSEGESTURES_SCREEN(s, md) \
    ((MousegesturesScreen *) (s)->base.privates[(md)->screenPrivateIndex].ptr)
#define MOUSEGESTURES_SCREEN(s) \
    MousegesturesScreen *ms = GET_MOUSEGESTURES_SCREEN (s, \
			      GET_MOUSEGESTURES_DISPLAY (s->display))

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))

static int displayPrivateIndex;

static CompMetadata mousegesturesMetadata;

typedef struct _Mousegesture {
    char stroke[MOUSEGESTURES_MAX_STROKE];
    int  nStrokes;
    char *pluginName;
    char *actionName;
} Mousegesture;


typedef struct _MousegesturesDisplay {
    int		    screenPrivateIndex;
    CompOption      opt[MOUSEGESTURES_DISPLAY_OPTION_NUM];
    HandleEventProc handleEvent;

    int grabIndex;
    int originalX;
    int originalY;
    int deltaX;
    int deltaY;

    char	 currentStroke;
    char	 lastStroke;
    char	 currentGesture[MOUSEGESTURES_MAX_STROKE];
    int		 nCurrentGesture;
    Mousegesture gesture[MOUSEGESTURES_MAX_GESTURES];
    int 	 nGesture;
} MousegesturesDisplay;

static void
mousegesturesDisplayActionInitiate (CompDisplay *d,
				    char        *pluginName,
				    char        *actionName)
{
    CompPlugin *p;
    CompOption *option = NULL;

    if (!strlen (pluginName) || !strlen (actionName))
	return;

    p = findActivePlugin (pluginName);
    if (p && p->vTable->getObjectOptions)
    {
	CompObject *object;
	object = compObjectFind (&core.base, COMP_OBJECT_TYPE_DISPLAY, NULL);
	if (object)
	{
	    CompOption *options;
	    int        nOptions;

	    options = (*p->vTable->getObjectOptions) (p, object, &nOptions);
	    option = compFindOption (options, nOptions, actionName, 0);
	}
    }
    else if (strcmp (pluginName, "exec") == 0)
    {
	return;
    }
    else
    {
	compLogMessage ("mousegestures", CompLogLevelWarn,
			"Could not find plugin called '%s', check that you " \
			"have it loaded and that you spelt it correctly in " \
			"the option.", pluginName);
	return;
    }

    if (option && option->value.action.initiate)
    {
        CompOption args[4];

	args[0].name    = "root";
	args[0].type    = CompOptionTypeInt;
	args[0].value.i = currentRoot;

	args[1].name    = "window";
	args[1].type    = CompOptionTypeInt;
	args[1].value.i = d->activeWindow;

	args[2].name    = "x";
	args[2].type    = CompOptionTypeInt;
	args[2].value.i = pointerX;

	args[3].name    = "y";
	args[3].type    = CompOptionTypeInt;
	args[3].value.i = pointerY;

	(*option->value.action.initiate)  (d, &option->value.action,
					   0, args, 4);
    }
    else
    {
	compLogMessage ("mousegestures", CompLogLevelWarn,
			"I found the plugin called '%s' but I could not" \
			" find the action called '%s', check that you spelt " \
			"it correctly in the option.", pluginName, actionName);
    }
}

static Bool
mousegesturesInitiate (CompDisplay     *d,
		       CompAction      *action,
		       CompActionState state,
		       CompOption      *option,
		       int             nOption)
{
    CompScreen *s;
    int        xid;

    MOUSEGESTURES_DISPLAY (d);

    xid = getIntOptionNamed (option, nOption, "root", 0);

    s = findScreenAtDisplay (d, xid);
    if (s)
    {
	md->originalX = pointerX;
	md->originalY = pointerY;
	md->deltaX    = 0;
	md->deltaY    = 0;

	md->currentStroke   = '\0';
	md->lastStroke      = '\0';
	memset (&md->currentGesture, 0, sizeof (md->currentGesture));
	md->nCurrentGesture = 0;

	md->grabIndex = pushScreenGrab (s, s->invisibleCursor, "mousegestures");

	if (state & CompActionStateInitButton)
	    action->state |= CompActionStateTermButton;

	return TRUE;
    }

    return FALSE;
}

static Bool
mousegesturesTerminate (CompDisplay     *d,
			CompAction      *action,
			CompActionState state,
			CompOption      *option,
			int             nOption)
{
    CompScreen   *s;
    int          ng, xid;
    Mousegesture *mg;

    MOUSEGESTURES_DISPLAY (d);

    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);

    if (!s)
	return FALSE;

    ng = md->nGesture;
    while (ng--)
    {
        mg = &md->gesture[ng];

	if (mg->nStrokes != md->nCurrentGesture)
	    continue;

	if (strncmp (md->currentGesture, mg->stroke, mg->nStrokes) != 0)
	    continue;

	if (!mg->pluginName || !mg->actionName)
	    continue;

	mousegesturesDisplayActionInitiate (d, mg->pluginName, mg->actionName);
    }

    removeScreenGrab (s, md->grabIndex, NULL);
    md->grabIndex = 0;

    action->state &= ~CompActionStateTermButton;

    return TRUE;
}

static void
mousegesturesHandleEvent (CompDisplay *d,
			  XEvent      *event)
{
    MOUSEGESTURES_DISPLAY (d);

    switch (event->type) {
    case MotionNotify:
	if (md->grabIndex)
	{
	    Bool detectDiagonal;
	    char currentStroke = '\0';
	    int  deltaX = 0, deltaY = 0, opt;

	    md->deltaX += (pointerX - lastPointerX);
	    md->deltaY += (pointerY - lastPointerY);

	    opt = MOUSEGESTURES_DISPLAY_OPTION_DETECT_DIAGONAL;
	    detectDiagonal = md->opt[opt].value.b;

	    if (abs (md->deltaX) > MOUSEGESTURE_MIN_DELTA)
	    {
		deltaX = md->deltaX;
		if (detectDiagonal &&
		    abs (md->deltaY) > MOUSEGESTURE_MAX_VARIANCE)
		{
		    deltaY = md->deltaY;
		}
	    }
	    else if (abs (md->deltaY) > MOUSEGESTURE_MIN_DELTA)
	    {
		deltaY = md->deltaY;
		if (detectDiagonal &&
		    abs (md->deltaX) > MOUSEGESTURE_MAX_VARIANCE)
		{
		    deltaX = md->deltaX;
		}
	    }

	    /* handle horizontal and vertical cases first */
	    if (!deltaY && deltaX < 0)
		currentStroke = M_LEFT;
	    else if (!deltaY && deltaX > 0)
		currentStroke = M_RIGHT;
	    else if (!deltaX && deltaY < 0)
		currentStroke = M_UP;
	    else if (!deltaX && deltaY > 0)
		currentStroke = M_DOWN;
	    else if (deltaX && deltaY)
	    {
		/* deltaX and deltaY != 0, thus diagonal */
		if (deltaX < 0)
		{
		    if (deltaY < 0)
			currentStroke = M_7;
		    else
			currentStroke = M_1;
		}
		else
		{
		    if (deltaY < 0)
			currentStroke = M_9;
		    else
			currentStroke = M_3;
		}
	    }

	    if (currentStroke && md->currentStroke != currentStroke)
	    {
		md->currentStroke = currentStroke;
		if (md->currentStroke != md->lastStroke)
		{
		    md->lastStroke = md->currentStroke;
		    if (md->nCurrentGesture < MOUSEGESTURES_MAX_STROKE)
		    {
			md->currentGesture[md->nCurrentGesture] =
			    md->currentStroke;
			md->nCurrentGesture++;
		    }
		}
		md->deltaX = 0;
		md->deltaY = 0;
	    }
	    else if (md->currentStroke && md->currentStroke == currentStroke)
	    {
		md->deltaX = 0;
		md->deltaY = 0;
	    }
	}
	break;
    }

    UNWRAP (md, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (md, d, handleEvent, mousegesturesHandleEvent);
}

static void
loadMouseGestures (CompDisplay *d)
{
    int             nGestures, nPlugins, nActions;
    CompOptionValue *gestureVal, *pluginVal, *actionVal;
    Mousegesture    *gesture;
    int             gNum = 0;

    MOUSEGESTURES_DISPLAY (d);

    nGestures = md->opt[MOUSEGESTURES_DISPLAY_OPTION_GESTURE].value.list.nValue;
    gestureVal = md->opt[MOUSEGESTURES_DISPLAY_OPTION_GESTURE].value.list.value;
    nPlugins = md->opt[MOUSEGESTURES_DISPLAY_OPTION_PLUGIN].value.list.nValue;
    pluginVal = md->opt[MOUSEGESTURES_DISPLAY_OPTION_PLUGIN].value.list.value;
    nActions = md->opt[MOUSEGESTURES_DISPLAY_OPTION_ACTION].value.list.nValue;
    actionVal = md->opt[MOUSEGESTURES_DISPLAY_OPTION_ACTION].value.list.value;

    if ((nGestures != nPlugins) || (nGestures != nActions))
	return;

    while (nGestures-- && (gNum < MOUSEGESTURES_MAX_GESTURES))
    {
	gesture = &md->gesture[gNum];
	strncpy (gesture->stroke, gestureVal->s, MOUSEGESTURES_MAX_STROKE - 1);
	gesture->nStrokes = strlen (gestureVal->s);

	if (gesture->pluginName)
	    free (gesture->pluginName);
	gesture->pluginName = strdup (pluginVal->s);

	if (gesture->actionName)
	    free (gesture->actionName);
	gesture->actionName = strdup (actionVal->s);

	gNum++;

	gestureVal++;
	pluginVal++;
	actionVal++;
    }

    md->nGesture = gNum;
}

static CompOption *
mousegesturesGetDisplayOptions (CompPlugin  *p,
				CompDisplay *d,
				int         *count)
{
    MOUSEGESTURES_DISPLAY (d);

    *count = NUM_OPTIONS (md);
    return md->opt;
}

static Bool
mousegesturesSetDisplayOption (CompPlugin      *p,
			       CompDisplay     *d,
			       const char      *name,
			       CompOptionValue *value)
{
    CompOption *o;
    int         index;

    MOUSEGESTURES_DISPLAY (d);

    o = compFindOption (md->opt, NUM_OPTIONS (md), name, &index);
    if (!o)
        return FALSE;

    switch (index) 
    {
        case MOUSEGESTURES_DISPLAY_OPTION_INITIATE_BUTTON:
            if (setDisplayAction (d, o, value))
	        return TRUE;
            break;
        default:
            if (compSetOption (o, value))
	    {
		loadMouseGestures (d);
	        return TRUE;
	    }
            break;
    }

    return FALSE;
}

static const CompMetadataOptionInfo mousegesturesDisplayOptionInfo[] = {
    { "initiate_button", "button", 0,
      mousegesturesInitiate, mousegesturesTerminate },
    { "detect_diagonal", "bool", 0, 0, 0 },
    { "gestures", "list", "<type>string</type>", 0, 0 },
    { "plugins", "list", "<type>string</type>", 0, 0 },
    { "actions", "list", "<type>string</type>", 0, 0 }
};

static Bool
mousegesturesInitDisplay (CompPlugin  *p,
			  CompDisplay *d)
{
    MousegesturesDisplay *md;

    md = malloc (sizeof (MousegesturesDisplay));
    if (!md)
        return FALSE;

    if (!compInitDisplayOptionsFromMetadata (d,
					     &mousegesturesMetadata,
					     mousegesturesDisplayOptionInfo,
					     md->opt,
					     MOUSEGESTURES_DISPLAY_OPTION_NUM))
    {
	free (md);
	return FALSE;
    }

    md->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (md->screenPrivateIndex < 0)
    {
	compFiniDisplayOptions (d, md->opt, MOUSEGESTURES_DISPLAY_OPTION_NUM);
        free (md);
        return FALSE;
    }

    md->grabIndex = 0;

    md->deltaX = 0;
    md->deltaY = 0;

    md->currentStroke = '\0';
    md->lastStroke = '\0';

    memset (&md->gesture, 0, sizeof (md->gesture));
    md->nGesture = 0;

    WRAP (md, d, handleEvent, mousegesturesHandleEvent);

    d->base.privates[displayPrivateIndex].ptr = md;

    return TRUE;
}

static void
mousegesturesFiniDisplay (CompPlugin *p, CompDisplay *d)
{
    int i;

    MOUSEGESTURES_DISPLAY (d);

    freeScreenPrivateIndex (d, md->screenPrivateIndex);

    UNWRAP (md, d, handleEvent);

    for (i = 0; i < MOUSEGESTURES_MAX_GESTURES; i++)
    {
	Mousegesture *gesture = &md->gesture[i];

	if (gesture->pluginName)
	    free (gesture->pluginName);

	if (gesture->actionName)
	    free (gesture->actionName);
    }

    compFiniDisplayOptions (d, md->opt, MOUSEGESTURES_DISPLAY_OPTION_NUM);

    free (md);
}

static Bool
mousegesturesInit (CompPlugin *p)
{
    if (!compInitPluginMetadataFromInfo (&mousegesturesMetadata,
					 p->vTable->name,
					 mousegesturesDisplayOptionInfo,
					 MOUSEGESTURES_DISPLAY_OPTION_NUM,
					 0, 0))
	return FALSE;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
    {
	compFiniMetadata (&mousegesturesMetadata);
	return FALSE;
    }

    compAddMetadataFromFile (&mousegesturesMetadata, p->vTable->name);

    return TRUE;
}
static CompBool
mousegesturesInitObject (CompPlugin *p,
			 CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) mousegesturesInitDisplay
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
mousegesturesFiniObject (CompPlugin *p,
			 CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) mousegesturesFiniDisplay
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static CompOption *
mousegesturesGetObjectOptions (CompPlugin *plugin,
			       CompObject *object,
			       int	  *count)
{
    static GetPluginObjectOptionsProc dispTab[] = {
	(GetPluginObjectOptionsProc) 0, /* GetCoreOptions */
	(GetPluginObjectOptionsProc) mousegesturesGetDisplayOptions
    };

    *count = 0;

    RETURN_DISPATCH (object, dispTab, ARRAY_SIZE (dispTab),
             (void *) count, (plugin, object, count));
}

static CompBool
mousegesturesSetObjectOption (CompPlugin      *plugin,
			      CompObject      *object,
			      const char      *name,
			      CompOptionValue *value)
{
    static SetPluginObjectOptionProc dispTab[] = {
	(SetPluginObjectOptionProc) 0, /* SetCoreOption */
	(SetPluginObjectOptionProc) mousegesturesSetDisplayOption
    };

    RETURN_DISPATCH (object, dispTab, ARRAY_SIZE (dispTab), FALSE,
		     (plugin, object, name, value));
}

static void
mousegesturesFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);

    compFiniMetadata (&mousegesturesMetadata);
}

static CompMetadata *
mousegesturesGetMetadata (CompPlugin *plugin)
{
    return &mousegesturesMetadata;
}

CompPluginVTable mousegesturesVTable = {
    "mousegestures",
    mousegesturesGetMetadata,
    mousegesturesInit,
    mousegesturesFini,
    mousegesturesInitObject,
    mousegesturesFiniObject,
    mousegesturesGetObjectOptions,
    mousegesturesSetObjectOption
};

CompPluginVTable *
getCompPluginInfo20070830 (void)
{
    return &mousegesturesVTable;
}

