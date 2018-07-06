/*
 * Window capture plugin, by Thai Pangsakulyanont
 *
 * Based on neg plugin, Copyright (c) 2006 Darryll Truchan <moppsy@comcast.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <compiz-core.h>
#include "windowcapture_options.h"

#define GET_WINDOWCAPTURE_DISPLAY(d) \
	((WindowCaptureDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define WINDOWCAPTURE_DISPLAY(d) \
	WindowCaptureDisplay *fd = GET_WINDOWCAPTURE_DISPLAY(d)

#define GET_WINDOWCAPTURE_SCREEN(s, fd) \
	((WindowCaptureScreen *) (s)->base.privates[fd->screenPrivateIndex].ptr)
#define WINDOWCAPTURE_SCREEN(s) \
	WindowCaptureScreen *fs = GET_WINDOWCAPTURE_SCREEN(s, GET_WINDOWCAPTURE_DISPLAY(s->display))

int displayPrivateIndex;

typedef struct _WindowCaptureDisplay {
	int screenPrivateIndex;
} WindowCaptureDisplay;

typedef struct _WindowCaptureScreen {
	DrawWindowTextureProc drawWindowTexture;
	PaintOutputProc paintOutput;
	PaintWindowProc paintWindow;
} WindowCaptureScreen;

CompWindow *capturingWindow = NULL;

Bool windowCapturePaintOutput(CompScreen *s, const ScreenPaintAttrib *sAttrib, const CompTransform *transform, Region region, CompOutput *output, unsigned int mask);
Bool windowCapturePaintWindow(CompWindow *w, const WindowPaintAttrib *attrib, const CompTransform *transform, Region region, unsigned int mask);

void windowCaptureWrapScreenFunctions(WindowCaptureScreen *fs, CompScreen *s);
void windowCaptureUnwrapScreenFunctions(WindowCaptureScreen *fs, CompScreen *s);

Bool windowCapturePaintWindow(CompWindow *w, const WindowPaintAttrib *attrib, const CompTransform *transform, Region region, unsigned int mask) {
	CompScreen *s = w->screen;
	Bool status = FALSE;
	WINDOWCAPTURE_SCREEN (s);
	if (capturingWindow == NULL || capturingWindow == w) {
		UNWRAP (fs, s, paintWindow);
		status = (*s->paintWindow)(w, attrib, transform, region, mask);
		WRAP (fs, s, paintWindow, windowCapturePaintWindow);
	}
	return status;
}

GLubyte inRange(int x);
GLubyte inRange(int x) {
	if (x > 255) return 255;
	if (x < 0) return 0;
	return (GLubyte)x;
}

void colorBlend(GLubyte rb, GLubyte gb, GLubyte bb, GLubyte rw, GLubyte gw, GLubyte bw,	GLubyte *r, GLubyte *g, GLubyte *b, GLubyte *a);
void colorBlend(GLubyte rb, GLubyte gb, GLubyte bb, GLubyte rw, GLubyte gw, GLubyte bw,	GLubyte *r, GLubyte *g, GLubyte *b, GLubyte *a) {
	GLubyte tmp, alpha;
	if (rb > rw) { tmp = rb; rb = rw; rw = tmp; }
	if (gb > gw) { tmp = gb; gb = gw; gw = tmp; }
	if (bb > bw) { tmp = bb; bb = bw; bw = tmp; }
	alpha = 255 - (rw + gw + bw - rb - gb - bb) / 3;
	if (alpha < 1) {
		*r = *g = *b = *a = 0;
	} else if (alpha > 254) {
		*r = rb;
		*g = gb;
		*b = bb;
		*a = 255;
	} else {
		*r = inRange(255 * rb / alpha);
		*g = inRange(255 * gb / alpha);
		*b = inRange(255 * bb / alpha);
		*a = alpha;
	}
}

Bool windowCapturePaintOutput(CompScreen *s, const ScreenPaintAttrib *sAttrib, const CompTransform *transform, Region region, CompOutput *output, unsigned int mask) {
	Bool status = FALSE;
	WINDOWCAPTURE_SCREEN (s);
	
	if (capturingWindow != NULL && capturingWindow->screen == s) {

		GLint x1 = output->region.extents.x1;
		GLint x2 = output->region.extents.x2;
		GLint y1 = output->region.extents.y1;
		GLint y2 = output->region.extents.y2;
		GLint w = x2 - x1, h = y2 - y1;
		GLubyte *buffer = (GLubyte *)malloc(sizeof(GLubyte) * w * h * 4);
		GLubyte *buffer_black = (GLubyte *)malloc(sizeof(GLubyte) * w * h * 4);
		GLubyte *buffer_white = (GLubyte *)malloc(sizeof(GLubyte) * w * h * 4);

		if (buffer && buffer_black && buffer_white) {

			int i;
			for (i = 0; i <= 1; i ++) {

				// draw color
				CompTransform sTransform = *transform;
				transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);
				glPushMatrix ();
				glLoadMatrixf (sTransform.m);
				glColor4f ((float)i, (float)i, (float)i, 1.0f);
				glEnable (GL_BLEND);
				glBegin (GL_QUADS);
				glVertex2i (x1, y1);
				glVertex2i (x1, y2);
				glVertex2i (x2, y2);
				glVertex2i (x2, y1);
				glEnd ();
				glDisable (GL_BLEND);
				glColor4usv (defaultColor);
				glPopMatrix ();

				// draw
				UNWRAP (fs, s, paintOutput);
				status = (*s->paintOutput)(s, sAttrib, transform, region, output, mask);
				WRAP (fs, s, paintOutput, windowCapturePaintOutput);

				// snapshot
				glReadPixels (x1, y1, w, h, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *)(i == 0 ? buffer_black : buffer_white));
			
			}

			// blend together
			for (i = 0; i < w * h * 4; i += 4) {
				colorBlend (
					buffer_black[i], buffer_black[i + 1], buffer_black[i + 2],
					buffer_white[i], buffer_white[i + 1], buffer_white[i + 2],
					&buffer[i], &buffer[i + 1], &buffer[i + 2], &buffer[i + 3]
				);
			}
			
			// find border
			int x, y, left, top, right, bottom;
			Bool transparent;
			for (top = 0; top < h; top ++) {
				transparent = TRUE;
				for (x = 0; x < w; x ++) {
					if (buffer[(top * w + x) * 4 + 3] != 0) {
						transparent = FALSE; break;
					}
				}
				if (!transparent) break;
			}
			for (right = w - 1; right >= 0; right --) {
				transparent = TRUE;
				for (y = 0; y < h; y ++) {
					if (buffer[(y * w + right) * 4 + 3] != 0) {
						transparent = FALSE; break;
					}
				}
				if (!transparent) break;
			}
			for (bottom = h - 1; bottom >= 0; bottom --) {
				transparent = TRUE;
				for (x = 0; x < w; x ++) {
					if (buffer[(bottom * w + x) * 4 + 3] != 0) {
						transparent = FALSE; break;
					}
				}
				if (!transparent) break;
			}
			for (left = 0; left < w; left ++) {
				transparent = TRUE;
				for (y = 0; y < h; y ++) {
					if (buffer[(y * w + left) * 4 + 3] != 0) {
						transparent = FALSE; break;
					}
				}
				if (!transparent) break;
			}
			if (top < bottom && left < right && top > 0 && left > 0) {
				i = 0;
				for (y = top; y <= bottom; y ++) {
					for (x = left; x <= right; x ++) {
						buffer[i]     = buffer[(y * w + x) * 4];
						buffer[i + 1] = buffer[(y * w + x) * 4 + 1];
						buffer[i + 2] = buffer[(y * w + x) * 4 + 2];
						buffer[i + 3] = buffer[(y * w + x) * 4 + 3];
						i += 4;
					}
				}
				w = right - left + 1;
				h = bottom - top + 1;
			}

			writeImageToFile (s->display, "/tmp/", "windowcapture.png", "png", w, h, buffer);

		}

		capturingWindow = NULL;
		UNWRAP (fs, s, paintOutput);
		status = (*s->paintOutput)(s, sAttrib, transform, region, output, mask);
		WRAP (fs, s, paintOutput, windowCapturePaintOutput);

		if (buffer) free (buffer);
		if (buffer_white) free (buffer_white);
		if (buffer_black) free (buffer_black);

	} else {

		UNWRAP (fs, s, paintOutput);
		status = (*s->paintOutput)(s, sAttrib, transform, region, output, mask);
		WRAP (fs, s, paintOutput, windowCapturePaintOutput);

	}

	return status;
}

void windowCaptureWrapScreenFunctions(WindowCaptureScreen *fs, CompScreen *s) {
	WRAP (fs, s, paintOutput, windowCapturePaintOutput);
	WRAP (fs, s, paintWindow, windowCapturePaintWindow);
}
void windowCaptureUnwrapScreenFunctions(WindowCaptureScreen *fs, CompScreen *s) {
	UNWRAP (fs, s, paintOutput);
	UNWRAP (fs, s, paintWindow);
}

static Bool windowCaptureCapture(CompDisplay *d, CompAction *action, CompActionState state, CompOption *option, int nOption) {
	CompWindow *w;
	Window xid;
    xid = getIntOptionNamed(option, nOption, "window", 0);
    w = findWindowAtDisplay(d, xid);
	if (w) {
		capturingWindow = w;
		damageScreen (w->screen);
	}
	return TRUE;
}

static Bool windowCaptureInitDisplay(CompPlugin *p, CompDisplay *d) {
	WindowCaptureDisplay *fd = (WindowCaptureDisplay *)malloc(sizeof(WindowCaptureDisplay));
	if (!fd) {
		return FALSE;
	}
	fd->screenPrivateIndex = allocateScreenPrivateIndex(d);
	if (fd->screenPrivateIndex < 0) {
		free (fd);
		return FALSE;
	}
	d->base.privates[displayPrivateIndex].ptr = fd;
	windowcaptureSetWindowCaptureKeyInitiate (d, windowCaptureCapture);
	windowcaptureSetWindowCaptureButtonInitiate (d, windowCaptureCapture);
	return TRUE;
}

static void windowCaptureFiniDisplay(CompPlugin *p, CompDisplay *d) {
	WINDOWCAPTURE_DISPLAY (d);
	freeScreenPrivateIndex (d, fd->screenPrivateIndex);
	free (fd);
}

static Bool windowCaptureInitScreen(CompPlugin *p, CompScreen *s) {
	WindowCaptureScreen *fs = (WindowCaptureScreen *)malloc(sizeof(WindowCaptureScreen));
	if (!fs) {
		return FALSE;
	}
	WINDOWCAPTURE_DISPLAY (s->display);
	s->base.privates[fd->screenPrivateIndex].ptr = fs;
	windowCaptureWrapScreenFunctions (fs, s);
	return TRUE;
}

static void windowCaptureFiniScreen(CompPlugin *p, CompScreen *s) {
	WINDOWCAPTURE_SCREEN (s);
	windowCaptureUnwrapScreenFunctions (fs, s);
	free (fs);
}

static Bool windowCaptureInit(CompPlugin * p) {
	displayPrivateIndex = allocateDisplayPrivateIndex();
	if (displayPrivateIndex < 0)
		return FALSE;
	return TRUE;
}

static void windowCaptureFini(CompPlugin * p) {
	if (displayPrivateIndex < 0)
		freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompBool windowCaptureInitObject(CompPlugin *p, CompObject *o) {
    static InitPluginObjectProc dispTab[] = {
		0,
		(InitPluginObjectProc) windowCaptureInitDisplay,
		(InitPluginObjectProc) windowCaptureInitScreen,
		0
    };
    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void windowCaptureFiniObject(CompPlugin *p, CompObject *o) {
    static FiniPluginObjectProc dispTab[] = {
		0,
		(FiniPluginObjectProc) windowCaptureFiniDisplay,
		(FiniPluginObjectProc) windowCaptureFiniScreen,
		0
    };
    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable windowCaptureVTable = {
	"windowcapture",
	0,
	windowCaptureInit,
	windowCaptureFini,
	windowCaptureInitObject,
	windowCaptureFiniObject,
	0,
	0,
};

CompPluginVTable* getCompPluginInfo(void) {
	return &windowCaptureVTable;
}




