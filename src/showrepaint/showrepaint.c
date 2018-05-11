/*
 * Copyright © 2017 Scott Moreau
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Based on work by Dennis Kasprzyk
 */

#include <compiz-core.h>

#include "showrepaint_options.h"

static int ShowrepaintDisplayPrivateIndex;

typedef struct _ShowrepaintDisplay
{
    int screenPrivateIndex;
} ShowrepaintDisplay;

typedef struct _ShowrepaintScreen
{
    PaintOutputProc	paintOutput;
} ShowrepaintScreen;

#define SHOWREPAINT_DISPLAY(d) PLUGIN_DISPLAY(d, Showrepaint, s)
#define SHOWREPAINT_SCREEN(s) PLUGIN_SCREEN(s, Showrepaint, s)


static Bool
showrepaintPaintOutput (CompScreen              *s,
                 const ScreenPaintAttrib *sAttrib,
                 const CompTransform     *transform,
                 Region                  region,
                 CompOutput              *output,
                 unsigned int            mask)
{
	Bool status;
	BoxPtr pBox;
	int    nBox;
	unsigned short color[4];

	SHOWREPAINT_SCREEN (s);

	UNWRAP (ss, s, paintOutput);
	status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
	WRAP (ss, s, paintOutput, showrepaintPaintOutput);

	glViewport (0, 0, s->width, s->height);

	glPushMatrix ();

	glTranslatef (-0.5f, -0.5f, -DEFAULT_Z_CAMERA);
	glScalef (1.0f  / s->width, -1.0f / s->height, 1.0f);
	glTranslatef (0, -s->height, 0.0f);

	glEnable (GL_BLEND); 

	color[3] = 50 * 0xffff / 100;
	color[0] = (rand () & 7) * color[3] / 8;
	color[1] = (rand () & 7) * color[3] / 8;
	color[2] = (rand () & 7) * color[3] / 8;

    glColor4usv (color);

	pBox = region->rects;
	nBox = region->numRects;

	glBegin (GL_TRIANGLES);

	while (nBox--)
	{
		glVertex2i (pBox->x1, pBox->y1);
		glVertex2i (pBox->x1, pBox->y2);
		glVertex2i (pBox->x2, pBox->y2);
		glVertex2i (pBox->x2, pBox->y2);
		glVertex2i (pBox->x2, pBox->y1);
		glVertex2i (pBox->x1, pBox->y1);
		
		pBox++;
	}

	glEnd ();
	
	glDisable (GL_BLEND);

	glColor4usv (defaultColor);

	glPopMatrix ();

	return status;
}

static Bool
showrepaintInitDisplay (CompPlugin *p,
					CompDisplay *d)
{
	ShowrepaintDisplay * sd;

	if (!checkPluginABI ("core", CORE_ABIVERSION))
		return FALSE;

	sd = malloc(sizeof (ShowrepaintDisplay));
	if (!sd)
		return FALSE;

	sd->screenPrivateIndex = allocateScreenPrivateIndex (d);
	if (sd->screenPrivateIndex < 0)
	{
		free(sd);
		return FALSE;
	}

    d->base.privates[ShowrepaintDisplayPrivateIndex].ptr = sd;

    return TRUE;

}

static void
showrepaintFiniDisplay (CompPlugin *p,
					CompDisplay *d)
{
	SHOWREPAINT_DISPLAY (d);

	freeScreenPrivateIndex (d, sd->screenPrivateIndex);

	free (sd);
}

static Bool
showrepaintInitScreen (CompPlugin *p,
				 CompScreen *s)
{
	ShowrepaintScreen *ss;

    SHOWREPAINT_DISPLAY (s->display);

	ss = malloc(sizeof (ShowrepaintScreen));
	if (!ss)
		return FALSE;

    s->base.privates[sd->screenPrivateIndex].ptr = ss;

	WRAP (ss, s, paintOutput, showrepaintPaintOutput);

	return TRUE;

}

static void
showrepaintFiniScreen (CompPlugin *p,
					CompScreen *s)
{
    SHOWREPAINT_SCREEN (s);

    UNWRAP (ss, s, paintOutput);

    free (ss);
}


static CompBool
showrepaintInitObject (CompPlugin *p,
					CompObject *o)
{
	static InitPluginObjectProc dispTab[] = {
		(InitPluginObjectProc) 0, /* InitCore */
		(InitPluginObjectProc) showrepaintInitDisplay,
		(InitPluginObjectProc) showrepaintInitScreen
	};

	RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
showrepaintFiniObject (CompPlugin *p,
					CompObject *o)
{
	static FiniPluginObjectProc dispTab[] = {
		(FiniPluginObjectProc) 0, /* FiniCore */
		(FiniPluginObjectProc) showrepaintFiniDisplay,
		(FiniPluginObjectProc) showrepaintFiniScreen
	};

	DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
showrepaintInit (CompPlugin *p)
{
	ShowrepaintDisplayPrivateIndex = allocateDisplayPrivateIndex ();

	if (ShowrepaintDisplayPrivateIndex < 0)
		return FALSE;

	return TRUE;
}

static void
showrepaintFini (CompPlugin *p)
{
	freeDisplayPrivateIndex (ShowrepaintDisplayPrivateIndex);
}

static CompPluginVTable showrepaintVTable=
{
    "showrepaint",
    0,
    showrepaintInit,
    showrepaintFini,
    showrepaintInitObject,
    showrepaintFiniObject,
    0,
    0
};

CompPluginVTable* getCompPluginInfo (void)
{
    return &showrepaintVTable;
}
