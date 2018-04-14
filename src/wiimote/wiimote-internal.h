/**
 *
 * Compiz Nintendo(R) Wii(TM) Remote Interface Plugin
 *
 * Copyright : (C) 2008 by Sam Spilsbury
 * E-mail    : smspillaz@gmail.com
 *
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
 */

#include "compiz-wiimote.h"
#include "wiimote_options.h"

#define MAX_GESTURES 32
#define MAX_REPORTS  16
#define MAX_WIIMOTES 10

/* Core / Display / Screen structs */

typedef struct _WiimoteDisplay
{
    int screenPrivateIndex;
    int count;
    
    int nWiimote;
    CompWiimote *wiimotes;
    /* Hack */

    int firstRoot;

    Bool report;
    
    Bool CallbackSet;
    
    CompTimeoutHandle infoTimeoutHandle;
    CompTimeoutHandle gestureTimeoutHandle;
    CompTimeoutHandle checkingTimeoutHandle;
    
} WiimoteDisplay;

typedef struct _WiimoteScreen
{
    PaintOutputProc        paintOutput;
} WiimoteScreen;

#define GET_WIIMOTE_DISPLAY(d)                            \
    ((WiimoteDisplay *) (d)->base.privates[wiimoteDisplayPrivateIndex].ptr)
#define WIIMOTE_DISPLAY(d)                                \
    WiimoteDisplay *ad = GET_WIIMOTE_DISPLAY (d)
#define GET_WIIMOTE_SCREEN(s, ad)                         \
    ((WiimoteScreen *) (s)->base.privates[(ad)->screenPrivateIndex].ptr)
#define WIIMOTE_SCREEN(s)                                 \
    WiimoteScreen *as = GET_WIIMOTE_SCREEN (s, GET_WIIMOTE_DISPLAY (s->display))
#define GET_WIIMOTE_WINDOW(w, as) \
    ((WiimoteWindow *) (w)->base.privates[ (as)->windowPrivateIndex].ptr)
#define WIIMOTE_WINDOW(w) \
    WiimoteWindow *aw = GET_WIIMOTE_WINDOW (w,          \
			  GET_WIIMOTE_SCREEN  (w->screen, \
			  GET_WIIMOTE_DISPLAY (w->screen->display)))

extern int wiimoteDisplayPrivateIndex;
cwiid_mesg_callback_t wiimoteCWiiDCallback;
cwiid_err_t err;
CompDisplay *firstDisplay;

/* Function Prototypes */

/* util.c*/

int findMinIR (CompDisplay *, CompWiimote *, int, int, int, int); // No you can't have fminf
int findMaxIR (CompDisplay *, CompWiimote *, int, int, int, int); // Or fmaxf
Bool wiimoteChangeLights(void *Wiimote);
void set_led_state(cwiid_wiimote_t *wiimote, unsigned char led_state);
void set_rpt_mode(cwiid_wiimote_t *wiimote, unsigned char rpt_mode);

/* communicate.c */

Bool sendInfoToPlugin (CompDisplay *, CompOption *, int, char *, char *);

/* gesture.c */

void wiimoteProcessButtons (CompDisplay *, CompWiimote *, struct cwiid_btn_mesg *);
void wiimoteProcessNunchuckButtons (CompDisplay *, CompWiimote *, struct cwiid_nunchuk_mesg *);
Bool wiimoteCheckForGestures (void *vs);

/* connect.c */

Bool find_wiimote(bdaddr_t *bdaddr, int timeout);
void* connectWiimote (void *vd);
Bool checkConnected (void *vd);

/* report.c */

Bool sendReports(void *vs);

/* option.c */

void
reloadReportersForWiimote (CompDisplay *d,
			   CompWiimote *wiimote);

void
reloadReportersForWiimoteNumber (CompDisplay *d,
				 int wiimoteNumber);

void
reloadGesturesForWiimote (CompDisplay *d,
			  CompWiimote *wiimote);

void
reloadGesturesForWiimoteNumber (CompDisplay *d,
				int wiimoteNumber);

void
reloadOptionsForWiimote (CompDisplay *d,
			 CompWiimote *wiimote);

void
reloadOptionsForWiimoteNumber (CompDisplay *d,
			       int wiimoteNumber);

/* action.c */

Bool
wiimoteToggle (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState cstate,
		 CompOption      *option,
		 int             nOption);

Bool
wiimoteDisable (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState cstate,
		 CompOption      *option,
		 int             nOption);

Bool
wiimoteSendInfo (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState cstate,
		 CompOption      *option,
		 int             nOption);

/* wiimote.c */
CompWiimote *
wiimoteAddWiimote (CompDisplay *d);
void
wiimoteRemoveWiimote (CompDisplay *d,
		      CompWiimote *wiimote);
