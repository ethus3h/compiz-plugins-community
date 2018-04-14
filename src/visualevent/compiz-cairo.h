/*
 * Compiz Cairo helper functions
 *
 * Author : Guillaume Seguin
 * Email : guillaume@segu.in
 *
 * Copyright (c) 2007 Guillaume Seguin <guillaume@segu.in>
 *
 * Extending some Cairo bits from wall.c:
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

#include <compiz-core.h>

#include <cairo-xlib-xrender.h>
#include <cairo.h>

#include <pango/pangocairo.h>

typedef struct _CairoContext
{
    cairo_t	    *cr;

    CompTexture	    texture;
    Pixmap	    pixmap;
    cairo_surface_t *surface;

    int		    width;
    int		    height;
} CairoContext;

void
setupCairoContext (CompScreen *s,
		   CairoContext *context);

void
destroyCairoContext (CompScreen *s,
		     CairoContext *context);

void
drawCairoTexture (CompScreen *s,
		  CompOutput *output,
		  CairoContext *context,
		  int x,
		  int y);

void
cairoDrawRoundedRectangle (cairo_t *cr,
			   int x, int y, int width, int height,
			   int radius, int margin);

PangoLayout *
cairoPrepareText (cairo_t *cr, char *text, char *font,
		  int *width, int *height);

void
cairoDrawText (cairo_t *cr, char *text, char *font,
	       int x, int y, int *width, int *height);

void
cairoDrawRoundedText (cairo_t *cr, char *text, char *font,
		      int x, int y, int radius, int margin,
		      float *text_color, float *border_color);
