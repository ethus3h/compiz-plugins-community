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

#define _GNU_SOURCE
#include "compiz-cairo.h"

#define PI 3.14159265359f

/* Cairo stuff -------------------------------------------------------------- */

/*
 * Setup a Cairo context and its surface, pixmap and texture
 */
void
setupCairoContext (CompScreen *s,
		   CairoContext *context)
{
    XRenderPictFormat *format;
    Screen *screen;

    screen = ScreenOfDisplay (s->display->display, s->screenNum);

    initTexture (s, &context->texture);

    format = XRenderFindStandardFormat (s->display->display,
					PictStandardARGB32);

    context->pixmap = XCreatePixmap (s->display->display, s->root,
				     context->width, context->height, 32);

    if (!bindPixmapToTexture (s, &context->texture, context->pixmap,
			      context->width, context->height, 32))
    compLogMessage ("visualevent", CompLogLevelError, 
		    "Could not create Cairo context.");

    context->surface =
	cairo_xlib_surface_create_with_xrender_format (s->display->display,
						       context->pixmap,
						       screen,
						       format,
						       context->width,
						       context->height);

    context->cr = cairo_create (context->surface);

    cairo_set_operator (context->cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (context->cr);
    cairo_set_operator (context->cr, CAIRO_OPERATOR_OVER);
}

/*
 * Destroy a Cairo context and its surface, pixmap and texture 
 */
void
destroyCairoContext (CompScreen *s,
		     CairoContext *context)
{
    if (context->cr)
	cairo_destroy (context->cr);

    if (context->surface)
	cairo_surface_destroy (context->surface);

    finiTexture (s, &context->texture);

    if (context->pixmap)
	XFreePixmap (s->display->display, context->pixmap);

    free (context);
}

/*
 * Draw a Cairo surface texture on output
 */
void
drawCairoTexture (CompScreen *s,
		  CompOutput *output,
		  CairoContext *context,
		  int x,
		  int y)
{
    CompMatrix matrix = context->texture.matrix;
    int x1, y1, x2, y2;

    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
    glEnable (GL_BLEND);

    enableTexture (s, &context->texture, COMP_TEXTURE_FILTER_GOOD);

    x1 = x + output->region.extents.x1;
    x2 = x1 + context->width;
    y1 = y + output->region.extents.y1;
    y2 = y1 + context->height;

    matrix.x0 -= x1 * matrix.xx;
    matrix.y0 -= y1 * matrix.yy;

    glBegin (GL_QUADS);
    glTexCoord2f (COMP_TEX_COORD_X (&matrix, x1),
		  COMP_TEX_COORD_Y (&matrix, y2));
    glVertex2i (x1, y2);
    glTexCoord2f (COMP_TEX_COORD_X (&matrix, x2),
		  COMP_TEX_COORD_Y (&matrix, y2));
    glVertex2i (x2, y2);
    glTexCoord2f (COMP_TEX_COORD_X (&matrix, x2),
		  COMP_TEX_COORD_Y (&matrix, y1));
    glVertex2i (x2, y1);
    glTexCoord2f (COMP_TEX_COORD_X (&matrix, x1),
		  COMP_TEX_COORD_Y (&matrix, y1));
    glVertex2i (x1, y1);
    glEnd ();

    disableTexture (s, &context->texture);

    glDisable (GL_BLEND);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
}

/* Helper drawing functions ------------------------------------------------- */

/*
 * Draw a rounded rectangle path
 */
void
cairoDrawRoundedRectangle (cairo_t *cr,
			   int x, int y, int width, int height,
			   int radius, int margin)
{
    int x0, y0, x1, y1;

    x0 = x + margin;
    y0 = y + margin;
    x1 = x + width - margin;
    y1 = y + height - margin;

    cairo_new_path (cr);
    cairo_arc (cr, x0 + radius, y1 - radius, radius, PI / 2, PI);
    cairo_line_to (cr, x0, y0 + radius);
    cairo_arc (cr, x0 + radius, y0 + radius, radius, PI, 3 * PI / 2);
    cairo_line_to (cr, x1 - radius, y0);
    cairo_arc (cr, x1 - radius, y0 + radius, radius, 3 * PI / 2, 2 * PI);
    cairo_line_to (cr, x1, y1 - radius);
    cairo_arc (cr, x1 - radius, y1 - radius, radius, 0, PI / 2);
    cairo_close_path (cr);
}

PangoLayout *
cairoPrepareText (cairo_t *cr, char *text, char *font,
		  int *width, int *height)
{
    PangoLayout *layout;
    char *markup = NULL;

    layout = pango_cairo_create_layout (cr);

    asprintf (&markup, "<span font_desc=\"%s\">%s</span>", font, text);
    pango_layout_set_markup (layout, markup, -1);
    free (markup);

    pango_layout_get_pixel_size (layout, width, height);

    return layout;
}

/*
 * Draw some text using Pango
 */
void
cairoDrawText (cairo_t *cr, char *text, char *font,
	       int x, int y, int *width, int *height)
{
    PangoLayout	 *layout;

    layout = cairoPrepareText (cr, text, font, width, height);

    cairo_save (cr);

    cairo_move_to (cr, x, y);
    pango_cairo_show_layout (cr, layout);

    cairo_restore (cr);
}

/*
 * Draw some text inside a rounded rectangle
 */
void
cairoDrawRoundedText (cairo_t *cr, char *text, char *font,
		      int x, int y, int radius, int margin,
		      float *text_color, float *border_color)
{
    PangoLayout	 *layout;
    int width, height;

    cairo_save (cr);

    layout = cairoPrepareText (cr, text, font, &width, &height);

    cairoDrawRoundedRectangle (cr, x, y, width + 20, height + 20,
			       radius, margin);

    if (border_color)
    {
	cairo_fill_preserve (cr);
	cairo_set_source_rgba (cr,
			       border_color[0], border_color[1],
			       border_color[2], border_color[3]);
	cairo_stroke (cr);
    }
    else
	cairo_fill (cr);

    cairo_set_source_rgba (cr,
			   text_color[0], text_color[1],
			   text_color[2], text_color[3]);

    cairo_move_to (cr, x + 10, y + 10);
    pango_cairo_show_layout (cr, layout);

    cairo_restore (cr);
}
