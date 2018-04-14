/**
 * Compiz Wiimote Plugin. Use the Nintendo Wii Remove with Compiz.
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
 * This plugin provides an interface that collects values from cwiid and stored them
 * in display stucts. It should be possible to extend this plugin quite easily.
 * 
 * Please note any major changes to the code in this header with who you
 * are and what you did. 
 *
 * TODO:
 * - ???
 */
#include "wiimote-internal.h"

static int corePrivateIndex; 
int wiimoteDisplayPrivateIndex;
int functionsPrivateIndex;

CompWiimote *
wiimoteForIter (CompDisplay *d,
		int         iter)
{
    WIIMOTE_DISPLAY (d);

    CompWiimote *wiimote;

    for (wiimote = ad->wiimotes; wiimote; wiimote = wiimote->next)
    {
	if (wiimote->id == iter)
	    return wiimote;
    }
    return NULL;
}

/* Wrappable Functions --------------------------------------------------- */

static void
wiimoteDisplayOptionChanged (CompDisplay             *d,
			       CompOption              *opt,
			       WiimoteDisplayOptions num)
{
    CompWiimote *wiimote;
    WIIMOTE_DISPLAY (d);
    switch (num)
    {   

	case WiimoteDisplayOptionXCalibrationMul:
	case WiimoteDisplayOptionYCalibrationMul:
	case WiimoteDisplayOptionXAdjust:
	case WiimoteDisplayOptionYAdjust:
        {
		for (wiimote = ad->wiimotes; wiimote; wiimote = wiimote->next)
		    reloadOptionsForWiimote (d, wiimote);
		break;
	}
	case WiimoteDisplayOptionGestureWiimoteNumber:
	case WiimoteDisplayOptionGestureType:
	case WiimoteDisplayOptionGesturePluginName:
	case WiimoteDisplayOptionGestureActionName:
	case WiimoteDisplayOptionGestureSensitivity:
	{
		for (wiimote = ad->wiimotes; wiimote; wiimote = wiimote->next)
		    reloadGesturesForWiimote (d, wiimote);
		break;
	}
	case WiimoteDisplayOptionReportWiimoteNumber:
	case WiimoteDisplayOptionReportType:
	case WiimoteDisplayOptionReportPluginName:
	case WiimoteDisplayOptionReportActionName:
	case WiimoteDisplayOptionReportSensitivity:
	{
		for (wiimote = ad->wiimotes; wiimote; wiimote = wiimote->next)
		    reloadReportersForWiimote (d, wiimote);
		break;
	}
	default:
		break;
    } 
}

/* Memory Management ----------------------------------------------------- */

CompWiimote *
wiimoteAddWiimote (CompDisplay *d)
{
    WIIMOTE_DISPLAY (d);
    CompWiimote *wiimote;
    
    if (!ad->wiimotes)
    {
        ad->wiimotes = calloc (1, sizeof (CompWiimote));
        if (!ad->wiimotes)
            return NULL;
	ad->wiimotes->next = NULL;
	wiimote = ad->wiimotes;
    }
    else
    {
    	for (wiimote = ad->wiimotes; wiimote->next; wiimote = wiimote->next) ;
    	
    	wiimote->next = calloc (1 , sizeof (CompWiimote));
	if (!wiimote->next)
	    return NULL;
	wiimote->next->next = NULL;
	wiimote = wiimote->next;
    }
    
    wiimote->connected = FALSE;
    wiimote->initiated = FALSE;
    wiimote->initSet = FALSE;

    return wiimote;
}

void
wiimoteRemoveWiimote (CompDisplay *d,
		      CompWiimote *wiimote)
{
    WIIMOTE_DISPLAY (d);
    CompWiimote *run;
    
    if (wiimote == ad->wiimotes)
    {
        if (ad->wiimotes->next)
            ad->wiimotes = ad->wiimotes->next;
        else
            ad->wiimotes = NULL;
            
        /* Let CWiiD handle some of the other memory management */
        
        free (wiimote);
    }
    else
    {
	for (run = ad->wiimotes; run; run = run->next)
	{
	    if (run == wiimote)
	    {
		if (run->next)
		    run = run->next;
		else
		    run = NULL;

		free (wiimote);
		break;
	    }
	}
    }
}

static WiimoteBaseFunctions wiimoteFunctions =
{
    .wiimoteForIter = wiimoteForIter
};

/* Core Initialization --------------------------------------------------- */

/* Use initCore() to find the first display (c->displays) */

static Bool
wiimoteInitCore (CompPlugin *p,
  	     CompCore   *c)
{
    if (!checkPluginABI ("core", CORE_ABIVERSION))
        return FALSE;

    wiimoteDisplayPrivateIndex = allocateDisplayPrivateIndex ();
    if (wiimoteDisplayPrivateIndex < 0)
    {
        return FALSE;
    }

    functionsPrivateIndex = allocateDisplayPrivateIndex ();
    if (functionsPrivateIndex < 0)
    {
	freeDisplayPrivateIndex (wiimoteDisplayPrivateIndex);
	return FALSE;
    }
    
    firstDisplay = c->displays;
    
    return TRUE;
}

static void
wiimoteFiniCore (CompPlugin *p,
  	     CompCore   *c)
{
    freeDisplayPrivateIndex (wiimoteDisplayPrivateIndex);
    freeDisplayPrivateIndex (functionsPrivateIndex);
}

/* Display Initialization --------------------------------------------------- */

static Bool
wiimoteInitDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    WiimoteDisplay *ad;
    CompOption     *abi, *index;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;
	
    compLogMessage ("wiimote", CompLogLevelInfo,
		"Please note that if you do not have the prompt plugin loaded, you won't see instructions on screen");

    ad = malloc (sizeof (WiimoteDisplay));
    if (!ad)
	return FALSE;

    ad->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (ad->screenPrivateIndex < 0)
    {
	free (ad);
	return FALSE;
    }

    d->base.privates[wiimoteDisplayPrivateIndex].ptr = ad;
    d->base.privates[functionsPrivateIndex].ptr = &wiimoteFunctions;

    abi = wiimoteGetAbiOption (d);
    abi->value.i = WIIMOTE_ABIVERSION;

    index = wiimoteGetIndexOption (d);
    index->value.i = functionsPrivateIndex;
    
    wiimoteSetToggleKeyInitiate (d, wiimoteToggle);
    wiimoteSetDisableKeyInitiate (d, wiimoteDisable);
    wiimoteSetReportKeyInitiate (d, wiimoteSendInfo);

    ad->nWiimote = 0;
    ad->wiimotes = NULL;

    wiimoteSetXCalibrationMulNotify (d, wiimoteDisplayOptionChanged);
    wiimoteSetYCalibrationMulNotify (d, wiimoteDisplayOptionChanged);
    wiimoteSetXAdjustNotify (d, wiimoteDisplayOptionChanged);
    wiimoteSetYAdjustNotify (d, wiimoteDisplayOptionChanged);

    wiimoteSetGestureWiimoteNumberNotify (d, wiimoteDisplayOptionChanged);
    wiimoteSetGestureTypeNotify (d, wiimoteDisplayOptionChanged);
    wiimoteSetGesturePluginNameNotify (d, wiimoteDisplayOptionChanged);
    wiimoteSetGestureActionNameNotify (d, wiimoteDisplayOptionChanged);
    wiimoteSetGestureSensitivityNotify (d, wiimoteDisplayOptionChanged);

    wiimoteSetReportWiimoteNumberNotify (d, wiimoteDisplayOptionChanged);
    wiimoteSetReportTypeNotify (d, wiimoteDisplayOptionChanged);
    wiimoteSetReportPluginNameNotify (d, wiimoteDisplayOptionChanged);
    wiimoteSetReportActionNameNotify (d, wiimoteDisplayOptionChanged);
    wiimoteSetReportSensitivityNotify (d, wiimoteDisplayOptionChanged);
    wiimoteSetReportDataTypeNotify (d, wiimoteDisplayOptionChanged);

    wiimoteSetReportXArgumentNotify (d, wiimoteDisplayOptionChanged);
    wiimoteSetReportYArgumentNotify (d, wiimoteDisplayOptionChanged);
    wiimoteSetReportZArgumentNotify (d, wiimoteDisplayOptionChanged);
        
    return TRUE;
}

static void
wiimoteFiniDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    WIIMOTE_DISPLAY (d);

    freeScreenPrivateIndex (d, ad->screenPrivateIndex);
    free (ad);
}

/* Screen Initialization --------------------------------------------------- */

static Bool
wiimoteInitScreen (CompPlugin *p,
		     CompScreen *s)
{
    WiimoteScreen *as;

    WIIMOTE_DISPLAY (s->display);

    as = malloc (sizeof (WiimoteScreen));
    if (!as)
	return FALSE;

    s->base.privates[ad->screenPrivateIndex].ptr = as;

    return TRUE;
}

static void
wiimoteFiniScreen (CompPlugin *p,
		     CompScreen *s)
{
    WIIMOTE_SCREEN (s);

    free (as);
}

/* Object Initialization --------------------------------------------------- */

static CompBool
wiimoteInitObject (CompPlugin *p,
		     CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) wiimoteInitCore,
	(InitPluginObjectProc) wiimoteInitDisplay,
	(InitPluginObjectProc) wiimoteInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
wiimoteFiniObject (CompPlugin *p,
		     CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) wiimoteFiniCore,
	(FiniPluginObjectProc) wiimoteFiniDisplay,
	(FiniPluginObjectProc) wiimoteFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

/* Plugin Initialization --------------------------------------------------- */

static Bool
wiimoteInit (CompPlugin *p)
{
    fprintf(stderr, " in init\n");
    corePrivateIndex = allocateCorePrivateIndex ();
    if (corePrivateIndex < 0)
	return FALSE;
    return TRUE;
}

static void
wiimoteFini (CompPlugin *p)
{
    freeCorePrivateIndex (corePrivateIndex);
}

CompPluginVTable wiimoteVTable = {
    "wiimote",
    0,
    wiimoteInit,
    wiimoteFini,
    wiimoteInitObject,
    wiimoteFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &wiimoteVTable;
}
