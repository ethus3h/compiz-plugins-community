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

#define PI 3.1415926
#define PI2 3.14159265358979323846
#define DEG2RAD2(DEG) ((DEG)*((PI2)/(180.0)))

static int displayPrivateIndex;

typedef struct _WiimoteDisplay
{
    int screenPrivateIndex;
    
    CompTimeoutHandle removeTextHandle;
    
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

#define GET_PROMPT_DISPLAY(d)                            \
    ((WiimoteDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define PROMPT_DISPLAY(d)                                \
    WiimoteDisplay *ad = GET_PROMPT_DISPLAY (d)
#define GET_PROMPT_SCREEN(s, ad)                         \
    ((WiimoteScreen *) (s)->base.privates[(ad)->screenPrivateIndex].ptr)
#define PROMPT_SCREEN(s)                                 \
    WiimoteScreen *as = GET_PROMPT_SCREEN (s, GET_PROMPT_DISPLAY (s->display))
#define GET_PROMPT_WINDOW(w, as) \
    ((WiimoteWindow *) (w)->base.privates[ (as)->windowPrivateIndex].ptr)
#define PROMPT_WINDOW(w) \
    WiimoteWindow *aw = GET_PROMPT_WINDOW (w,          \
			  GET_PROMPT_SCREEN  (w->screen, \
			  GET_PROMPT_DISPLAY (w->screen->display)))

/* Prototyping */

static void 
promptFreeTitle (CompScreen *s)
{
    PROMPT_SCREEN(s);

    if (!as->textPixmap)
	return;

    releasePixmapFromTexture (s, &as->textTexture);
    initTexture (s, &as->textTexture);
    XFreePixmap (s->display->display, as->textPixmap);
    as->textPixmap = None;
    damageScreen (s);
}

static void 
promptRenderTitle (CompScreen *s, char *stringData)
{
    CompTextAttrib tA;
    int            stride;
    void           *data;

    PROMPT_SCREEN (s);

    //ringFreeWindowTitle (s);

    int ox1, ox2, oy1, oy2;
    getCurrentOutputExtents (s, &ox1, &oy1, &ox2, &oy2);

    /* 75% of the output device as maximum width */
    tA.maxWidth = (ox2 - ox1) * 3 / 4;
    tA.maxHeight = 100;
    tA.size = promptGetTitleFontSize (s);
    tA.color[0] = promptGetTitleFontColorRed (s);
    tA.color[1] = promptGetTitleFontColorGreen (s);
    tA.color[2] = promptGetTitleFontColorBlue (s);
    tA.color[3] = promptGetTitleFontColorAlpha (s);

    tA.bgHMargin = 10.0f;
    tA.bgVMargin = 10.0f;
    tA.bgColor[0] = promptGetTitleBackColorRed (s);
    tA.bgColor[1] = promptGetTitleBackColorGreen (s);
    tA.bgColor[2] = promptGetTitleBackColorBlue (s);
    tA.bgColor[3] = promptGetTitleBackColorAlpha (s);
    tA.family = "Sans";

    initTexture (s, &as->textTexture);

    if ((*s->display->fileToImage) (s->display, "TextToPixmap", (char *)&tA,
			 	    &as->textWidth, &as->textHeight,
				    &stride, &data))
    {
	as->textPixmap = (Pixmap)data;
	bindPixmapToTexture (s, &as->textTexture, as->textPixmap,
			     as->textWidth, as->textHeight, 32);
    }
    else 
    {
	as->textPixmap = None;
	as->textWidth  = 0;
	as->textHeight = 0;
    }
}

/* Stolen from ring.c */

static void
promptDrawTitle (CompScreen *s)
{
    PROMPT_SCREEN(s);
    GLboolean wasBlend;
    GLint oldBlendSrc, oldBlendDst;

    float width = as->textWidth;
    float height = as->textHeight;

    int ox1, ox2, oy1, oy2;
    getCurrentOutputExtents (s, &ox1, &oy1, &ox2, &oy2);

    float x = ox1 + ((ox2 - ox1) / 2) - (as->textWidth / 2);
    float y = oy1 + ((oy2 - oy1) / 2) + (height / 2);;

    x = floor (x);
    y = floor (y);

    glGetIntegerv (GL_BLEND_SRC, &oldBlendSrc);
    glGetIntegerv (GL_BLEND_DST, &oldBlendDst);
    wasBlend = glIsEnabled (GL_BLEND);

    if (!wasBlend)
	glEnable (GL_BLEND);
    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);

    glPushMatrix ();
/*
    glTranslatef (x, y - height, 0.0f);
    glRectf (0.0f, height, width, 0.0f);
    glRectf (0.0f, 0.0f, width, -border);
    glRectf (0.0f, height + border, width, height);
    glRectf (-border, height, 0.0f, 0.0f);
    glRectf (width, height, width + border, 0.0f);
    glTranslatef (-border, -border, 0.0f);

#define CORNER(a,b) \
    for (k = a; k < b; k++) \
    {\
	float rad = k * (PI / 180.0f);\
	glVertex2f (0.0f, 0.0f);\
	glVertex2f (cos (rad) * border, sin (rad) * border);\
	glVertex2f (cos ((k - 1) * (PI / 180.0f)) * border, \
		    sin ((k - 1) * (PI / 180.0f)) * border);\
    }
    int k;

    glTranslatef (border, border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (180, 270) glEnd ();
    glTranslatef (-border, -border, 0.0f);

    glTranslatef (width + border, border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (270, 360) glEnd ();
    glTranslatef (-(width + border), -border, 0.0f);

    glTranslatef (border, height + border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (90, 180) glEnd ();
    glTranslatef (-border, -(height + border), 0.0f);

    glTranslatef (width + border, height + border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (0, 90) glEnd ();
    glTranslatef (-(width + border), -(height + border), 0.0f);

    glPopMatrix ();

#undef CORNER

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f (1.0, 1.0, 1.0, 1.0);*/

    enableTexture (s, &as->textTexture,COMP_TEXTURE_FILTER_GOOD);

    CompMatrix *m = &as->textTexture.matrix;

    glBegin (GL_QUADS);

    glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m ,0));
    glVertex2f (x, y - height);
    glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m, height));
    glVertex2f (x, y);
    glTexCoord2f (COMP_TEX_COORD_X (m, width), COMP_TEX_COORD_Y (m, height));
    glVertex2f (x + width, y);
    glTexCoord2f (COMP_TEX_COORD_X (m, width), COMP_TEX_COORD_Y (m, 0));
    glVertex2f (x + width, y - height);

    glEnd ();

    disableTexture (s, &as->textTexture);
    glColor4usv (defaultColor);

    if (!wasBlend)
	glDisable (GL_BLEND);
    glBlendFunc (oldBlendSrc, oldBlendDst);
}

static Bool
promptRemoveTitle (void *vs)
{
    CompScreen *s = (CompScreen *) vs;

    PROMPT_SCREEN (s);
    
    as->title = FALSE;

    promptFreeTitle (s);

    return FALSE;
}

/* Paint title on screen if neccessary */
 
static Bool
promptPaintOutput (CompScreen		  *s,
		 const ScreenPaintAttrib *sAttrib,
		 const CompTransform	  *transform,
		 Region		          region,
		 CompOutput		  *output,
		 unsigned int		  mask)
{
    Bool status;

    PROMPT_SCREEN (s);    
    CompTransform sTransform = *transform;
    
    mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
    
    UNWRAP (as, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
    WRAP (as, s, paintOutput, promptPaintOutput);

	transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);
	
	glPushMatrix ();
	glLoadMatrixf (sTransform.m);
	//glPopMatrix ();
    

    if (as->title)
    {
        promptDrawTitle (s);

    }
    glPopMatrix ();

    return status;
} 

/* Callable Actions */

static Bool
promptDisplayText (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState cstate,
		 CompOption      *option,
		 int             nOption)
{
	char *stringData;
	int timeout;
	Bool infinite = FALSE;

	CompWindow *w;
    	w = findWindowAtDisplay (d, getIntOptionNamed (option, nOption,
    							       "window", 0));
	if (!w) 
	    return TRUE;

	CompScreen *s;
    	s = findScreenAtDisplay (d, getIntOptionNamed (option, nOption,
    							       "root", 0));
	if (!s)
	    return TRUE;

        PROMPT_SCREEN (w->screen);

    stringData = strdup(getStringOptionNamed (option, nOption, "string", "This message was not sent correctly"));
	timeout = getIntOptionNamed(option, nOption, "timeout", 5000);
	if (timeout < 0)
		infinite = TRUE;

    /* Create Message */
    as->title = TRUE;
	promptRenderTitle (w->screen, stringData);
	if (!infinite)
		compAddTimeout (timeout, timeout * 1.2, promptRemoveTitle, s);
    damageScreen (s);
	
    return TRUE;
}

/* Option Initialisation */

static Bool
promptInitScreen (CompPlugin *p,
		     CompScreen *s)
{
    WiimoteScreen *as;

    PROMPT_DISPLAY (s->display);

    as = malloc (sizeof (WiimoteScreen));
    if (!as)
	return FALSE;

    s->base.privates[ad->screenPrivateIndex].ptr = as;

    initTexture (s, &as->textTexture);

    as->textWidth = 0;
    as->textHeight = 0;
    
    as->textPixmap = None;

    as->title = FALSE;
    
    WRAP (as, s, paintOutput, promptPaintOutput); 

    return TRUE;
}

static void
promptFiniScreen (CompPlugin *p,
		     CompScreen *s)
{
    PROMPT_SCREEN (s);

    UNWRAP (as, s, paintOutput);

    free (as);
}

static Bool
promptInitDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    WiimoteDisplay *ad;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    ad = malloc (sizeof (WiimoteDisplay));
    if (!ad)
	return FALSE;

    ad->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (ad->screenPrivateIndex < 0)
    {
	free (ad);
	return FALSE;
    }

    d->base.privates[displayPrivateIndex].ptr = ad;

    promptSetDisplayTextInitiate (d, promptDisplayText);

    return TRUE;
}

static void
promptFiniDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    PROMPT_DISPLAY (d);

    freeScreenPrivateIndex (d, ad->screenPrivateIndex);
    free (ad);
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
