/**
 * Compiz Prompt Plugin. Have other plugins display messages on screen.
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
 *
 * TODO:
 * - Handle Multiple text requests via the use of a struct system
 *   and memory allocation
 * - Allow for offsets of text placement
 * - Integration with compLogMessage?
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <string.h>
#include <math.h>

#include <compiz-core.h>
#include <compiz-text.h>

#include <unistd.h>

#include "prompt_options.h"

static int displayPrivateIndex;

typedef struct _PromptDisplay
{
    int screenPrivateIndex;
    
    CompTimeoutHandle removeTextHandle;

    TextFunc *textFunc;
} PromptDisplay;

typedef struct _PromptScreen
{
    PaintOutputProc paintOutput;

    /* text display support */
    CompTextData *textData;
} PromptScreen;

#define GET_PROMPT_DISPLAY(d)                            \
    ((PromptDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define PROMPT_DISPLAY(d)                                \
    PromptDisplay *pd = GET_PROMPT_DISPLAY (d)
#define GET_PROMPT_SCREEN(s, pd)                         \
    ((PromptScreen *) (s)->base.privates[(pd)->screenPrivateIndex].ptr)
#define PROMPT_SCREEN(s)                                 \
    PromptScreen *ps = GET_PROMPT_SCREEN (s, GET_PROMPT_DISPLAY (s->display))

/* Prototyping */

static void 
promptFreeTitle (CompScreen *s)
{
    PROMPT_SCREEN(s);
    PROMPT_DISPLAY (s->display);

    if (!ps->textData)
	return;

    (pd->textFunc->finiTextData) (s, ps->textData);
    ps->textData = NULL;

    damageScreen (s);
}

static void 
promptRenderTitle (CompScreen *s,
		   char       *stringData)
{
    CompTextAttrib tA;
    int            ox1, ox2, oy1, oy2;

    PROMPT_SCREEN (s);
    PROMPT_DISPLAY (s->display);

    promptFreeTitle (s);

    getCurrentOutputExtents (s, &ox1, &oy1, &ox2, &oy2);

    /* 75% of the output device as maximum width */
    tA.maxWidth = (ox2 - ox1) * 3 / 4;
    tA.maxHeight = 100;

    tA.family = "Sans";
    tA.size = promptGetTitleFontSize (s);
    tA.color[0] = promptGetTitleFontColorRed (s);
    tA.color[1] = promptGetTitleFontColorGreen (s);
    tA.color[2] = promptGetTitleFontColorBlue (s);
    tA.color[3] = promptGetTitleFontColorAlpha (s);
   
    tA.flags = CompTextFlagWithBackground | CompTextFlagEllipsized;

    tA.bgHMargin = 10.0f;
    tA.bgVMargin = 10.0f;
    tA.bgColor[0] = promptGetTitleBackColorRed (s);
    tA.bgColor[1] = promptGetTitleBackColorGreen (s);
    tA.bgColor[2] = promptGetTitleBackColorBlue (s);
    tA.bgColor[3] = promptGetTitleBackColorAlpha (s);

    ps->textData = (pd->textFunc->renderText) (s, stringData, &tA);
}

/* Stolen from ring.c */

static void
promptDrawTitle (CompScreen *s)
{
    int   ox1, ox2, oy1, oy2;
    float x, y, width, height;

    PROMPT_SCREEN (s);
    PROMPT_DISPLAY (s->display);

    width = ps->textData->width;
    height = ps->textData->height;

    getCurrentOutputExtents (s, &ox1, &oy1, &ox2, &oy2);

    x = floor (ox1 + ((ox2 - ox1) / 2) - (width / 2));
    y = floor (oy1 + ((oy2 - oy1) / 2) + (height / 2));

    (pd->textFunc->drawText) (s, ps->textData, x, y, 0.7f);
}

static Bool
promptRemoveTitle (void *vs)
{
    CompScreen *s = (CompScreen *) vs;

    PROMPT_DISPLAY (s->display);

    promptFreeTitle (s);

    pd->removeTextHandle = 0;

    return FALSE;
}

/* Paint title on screen if neccessary */
 
static Bool
promptPaintOutput (CompScreen		   *s,
		   const ScreenPaintAttrib *sAttrib,
		   const CompTransform	   *transform,
		   Region		   region,
		   CompOutput		   *output,
		   unsigned int		   mask)
{
    Bool status;

    PROMPT_SCREEN (s);    
    
    UNWRAP (ps, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
    WRAP (ps, s, paintOutput, promptPaintOutput);

    if (ps->textData)
    {
	CompTransform sTransform = *transform;

	transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);
	
	glPushMatrix ();
	glLoadMatrixf (sTransform.m);
        promptDrawTitle (s);
	glPopMatrix ();
    }

    return status;
} 

/* Callable Actions */

static Bool
promptDisplayText (CompDisplay     *d,
		   CompAction      *action,
		   CompActionState state,
		   CompOption      *option,
		   int             nOption)
{
    Window     xid;
    CompScreen *s;

    xid = getIntOptionNamed (option, nOption, "root", 0);
    s   = findScreenAtDisplay (d, xid);
    if (s)
    {
	char *stringData;
	int  timeout;

	stringData = getStringOptionNamed (option, nOption, "string",
					   "This message was not"
					   "sent correctly");

	timeout = getIntOptionNamed (option, nOption, "timeout", 5000);

	/* Create Message */
	promptRenderTitle (s, stringData);

	if (timeout > 0)
	{
	    PROMPT_DISPLAY (s->display);

	    if (pd->removeTextHandle)
		compRemoveTimeout (pd->removeTextHandle);
	    pd->removeTextHandle = compAddTimeout (timeout, timeout * 1.2,
						   promptRemoveTitle, s);
	}

	damageScreen (s);
    }
	
    return TRUE;
}

/* Option Initialisation */

static Bool
promptInitScreen (CompPlugin *p,
		  CompScreen *s)
{
    PromptScreen *ps;

    PROMPT_DISPLAY (s->display);

    ps = malloc (sizeof (PromptScreen));
    if (!ps)
	return FALSE;

    s->base.privates[pd->screenPrivateIndex].ptr = ps;

    ps->textData = NULL;

    WRAP (ps, s, paintOutput, promptPaintOutput); 

    return TRUE;
}

static void
promptFiniScreen (CompPlugin *p,
		  CompScreen *s)
{
    PROMPT_SCREEN (s);

    UNWRAP (ps, s, paintOutput);

    free (ps);
}

static Bool
promptInitDisplay (CompPlugin  *p,
		   CompDisplay *d)
{
    PromptDisplay *pd;
    int           index;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;
	
    if (!checkPluginABI ("text", TEXT_ABIVERSION) ||
	!getPluginDisplayIndex (d, "text", &index))
    {
	compLogMessage ("prompt", CompLogLevelWarn,
			"No compatible text plugin found. "
			"Prompt will now unload");
	return FALSE;
    }

    pd = malloc (sizeof (PromptDisplay));
    if (!pd)
	return FALSE;

    pd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (pd->screenPrivateIndex < 0)
    {
	free (pd);
	return FALSE;
    }

    d->base.privates[displayPrivateIndex].ptr = pd;
    pd->textFunc = d->base.privates[index].ptr;

    promptSetDisplayTextInitiate (d, promptDisplayText);

    return TRUE;
}

static void
promptFiniDisplay (CompPlugin  *p,
		   CompDisplay *d)
{
    PROMPT_DISPLAY (d);

    if (pd->removeTextHandle)
	compRemoveTimeout (pd->removeTextHandle);

    freeScreenPrivateIndex (d, pd->screenPrivateIndex);
    free (pd);
}

static CompBool
promptInitObject (CompPlugin *p,
		  CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0,
	(InitPluginObjectProc) promptInitDisplay,
	(InitPluginObjectProc) promptInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
promptFiniObject (CompPlugin *p,
		  CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0,
	(FiniPluginObjectProc) promptFiniDisplay,
	(FiniPluginObjectProc) promptFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
promptInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
promptFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}

CompPluginVTable promptVTable = {
    "prompt",
    0,
    promptInit,
    promptFini,
    promptInitObject,
    promptFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &promptVTable;
}
