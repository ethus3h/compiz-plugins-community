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

#include "wiimote-internal.h"

/* Wii Remote Connection / Handling --------------------------------------------------- */

/* Allocate a callback function */

static Bool
allocateCallbackFunction (cwiid_wiimote_t *wiimote, int nWiimote)
{
	if (nWiimote > MAX_WIIMOTES)
		return FALSE;

	if (cwiid_set_mesg_callback (wiimote, wiimoteCWiiDCallback))
	{
		fprintf(stderr, "Could not set mesg callback\n");
		return FALSE;
	}
	return TRUE;
}

/* Interface to find a Wii Remote
 * so that cwiid_open won't crash
 * when we can't find one */

Bool find_wiimote(bdaddr_t *bdaddr, int timeout)
{
	struct cwiid_bdinfo *bdinfo;
	int bdinfo_count;

	if (timeout == -1) {
		while ((bdinfo_count = cwiid_get_bdinfo_array(-1, 2, 1, &bdinfo, 0))
		       == 0);
		if (bdinfo_count == -1) {
			return FALSE;
		}
	}
	else {
		bdinfo_count = cwiid_get_bdinfo_array(-1, timeout, 1, &bdinfo, 0);
		if (bdinfo_count == -1) {
			return FALSE;
		}
		else if (bdinfo_count == 0) {
			return FALSE;
		}
	}

	free(bdinfo);
	return TRUE;
}

/* Second thread.
 * This is the actual bluetooth connection
 * phase for the Wii Remote. Because
 * control is not returned to compiz while
 * connection is taking place, the desktop
 * will appear to 'lock up' - which
 * is certainly not what we want, especially
 * if we want to do fancy stuff like display
 * messages. When a wiimote is successfully
 * connected to the host via bluetooth, a 
 * variable 'connected' is set to indicate that
 * the CompWiimote can be used.
 */

void* connectWiimote (void *vd)
{
        bdaddr_t bdaddr;	/* bluetooth device address */
        CompDisplay *d = (CompDisplay *) vd;
        bdaddr = *BDADDR_ANY;
        CompScreen *s;
	CompWiimote *cmpwiimote;
        cwiid_wiimote_t *wiimote;
        
        WIIMOTE_DISPLAY (d);
        
        if (!find_wiimote(&bdaddr, 2))
        {
	        compLogMessage ("wiimote", CompLogLevelError,
			"Wii Remote not found");

	        for (s = d->screens; s; s = s->next)
	        {
	           // WIIMOTE_SCREEN (s);
	            CompOption arg[4];
		    int nArg = 0;

                    arg[nArg].name = "window";
                    arg[nArg].type = CompOptionTypeInt;
                    arg[nArg].value.i = d->activeWindow;
                    nArg++;

                    arg[nArg].name = "root";
                    arg[nArg].type = CompOptionTypeInt;
                    arg[nArg].value.i = s->root;
                    nArg++;

                    arg[nArg].name = "string";
                    arg[nArg].type = CompOptionTypeString;
                    arg[nArg].value.s = "Wii Remote not found";
                    nArg++;

                    arg[nArg].name = "timeout";
                    arg[nArg].type = CompOptionTypeInt;
                    arg[nArg].value.i = 500;

		    sendInfoToPlugin (d, arg, nArg,
		    "prompt",
		    "display_text");
	        }
	        compRemoveTimeout(ad->checkingTimeoutHandle);
	    }
	    else
	    {
	        for (s = d->screens; s; s = s->next)
	        {
	            CompOption arg[4];
		    int nArg = 0;

                    arg[nArg].name = "window";
                    arg[nArg].type = CompOptionTypeInt;
                    arg[nArg].value.i = d->activeWindow;
                    nArg++;

                    arg[nArg].name = "root";
                    arg[nArg].type = CompOptionTypeInt;
                    arg[nArg].value.i = s->root;
                    nArg++;

                    arg[nArg].name = "string";
                    arg[nArg].type = CompOptionTypeString;
                    arg[nArg].value.s = "Wii Remote(s) found. Continue to\n"\
	                         " hold (1) and (2) to finalize connection sequence";
                    nArg++;

                    arg[nArg].name = "timeout";
                    arg[nArg].type = CompOptionTypeInt;
                    arg[nArg].value.i = 500;

		    sendInfoToPlugin (d, arg, nArg,
		    "prompt",
		    "display_text");
	        }
        	if (!(wiimote = cwiid_open(&bdaddr, 0))) {
		        compLogMessage ("wiimote", CompLogLevelError,
			"Could not connect to Wii Remote");

		        return NULL;
	        }

		/* Should be safe to assume that the most recent wiimote is the last one on the list */
		for (cmpwiimote = ad->wiimotes; cmpwiimote->next; cmpwiimote = cmpwiimote->next) ;

	        cmpwiimote->wiimote = wiimote;
	        cmpwiimote->connected = TRUE;
	        for (s = d->screens; s; s = s->next)
	        {
	            CompOption arg[4];
		    int nArg = 0;

                    arg[nArg].name = "window";
                    arg[nArg].type = CompOptionTypeInt;
                    arg[nArg].value.i = d->activeWindow;
                    nArg++;

                    arg[nArg].name = "root";
                    arg[nArg].type = CompOptionTypeInt;
                    arg[nArg].value.i = s->root;
                    nArg++;

                    arg[nArg].name = "string";
                    arg[nArg].type = CompOptionTypeString;
                    arg[nArg].value.s = "Wii Remote successfully connected";
                    nArg++;

                    arg[nArg].name = "timeout";
                    arg[nArg].type = CompOptionTypeInt;
                    arg[nArg].value.i = 500;

		    sendInfoToPlugin (d, arg, nArg,
		    "prompt",
		    "display_text");
	        }
	    }
	    return NULL; // This is required for pthread
}

/* Second Thread Poller.
 * This is a poller interface
 * to ensure that the Wii Remote is connected
 * before we do anything that requires initialisation.
 * When the wii remote is connected it does things
 * such as setup and then returns FALSE
 * so that it is not called again.
 */

Bool checkConnected (void *vd)
{
        CompDisplay *d = vd;
	CompWiimote *cmpwiimote;
        WIIMOTE_DISPLAY (d);
	struct cwiid_state state;	/* wiimote state */
	unsigned char mesg = 0; /* Mesg Flag */
	unsigned char rpt_mode = 0; /* Reporting Flag */
	unsigned char rumble = 0; /* Rumble Flag */
	int led_state = 0; /* LED State Flag */

        /* FIXME:
         * Remove me as all I do
         * is bring down host programs
         * not good for compiz!
         */

	cwiid_set_err(err); /* Error Handling */

	/* Should be safe to assume that the most recent wiimote is the last one on the list */
	for (cmpwiimote = ad->wiimotes; cmpwiimote->next; cmpwiimote = cmpwiimote->next) ;

        if (cmpwiimote->connected)
        {
	    /* Connect to the wiimote */
	    compLogMessage ("wiimote", CompLogLevelInfo,
			"Wii Remote Connection started");

            if (allocateCallbackFunction
            (cmpwiimote->wiimote, ad->nWiimote)) // Allocate a c
            {
	            compLogMessage ("wiimote", CompLogLevelInfo,
			"Wii Remote is now connected");


	            if (cwiid_set_rumble(cmpwiimote->wiimote, rumble)) {
		        compLogMessage ("wiimote", CompLogLevelError,
			"Couldn't set rumble");
	            }
	            
	            /* Toggle Accellerometor Reporting */
	            
                    toggle_bit(rpt_mode, CWIID_RPT_ACC);
	            set_rpt_mode(cmpwiimote->wiimote, rpt_mode);
	            
	            /* Toggle Button Reporting */
	            
	            toggle_bit(rpt_mode, CWIID_RPT_BTN);
	            set_rpt_mode(cmpwiimote->wiimote, rpt_mode);
	            
	            
	            /* CWIID_RPT_EXT is actually
	             * CWIID_RPT_NUNCHUK | CWIID_RPT_CLASSIC */
	            
	            /* Toggle Extension Reporting */ 
	            
	            toggle_bit(rpt_mode, CWIID_RPT_EXT);
	            set_rpt_mode(cmpwiimote->wiimote, rpt_mode);
	            
	            
	            /* libwiimote picks the highest quality IR mode available with the
	             * other options selected (not including as-yet-undeciphered
	             * interleaved mode */
	             
	            /* Toggle IR reporting */
	             
	            toggle_bit(rpt_mode, CWIID_RPT_IR);
	            set_rpt_mode(cmpwiimote->wiimote, rpt_mode);
	            
	            /* Toggle Status (Battery etc) reporting */
	            
	            toggle_bit(rpt_mode, CWIID_RPT_STATUS);
	            set_rpt_mode(cmpwiimote->wiimote, rpt_mode);
	            
	            /* Check to see if there is a mesg reporting
	               handler (Should be wiimoteCWiiDCallbackx, where X is
	               the Wii Remote iter
	             */
		    /* FIXME !mesg is here unconditionally true */
	            if (!mesg) {
	                if (cwiid_enable(cmpwiimote->wiimote, CWIID_FLAG_MESG_IFC)) {
				compLogMessage ("wiimote", CompLogLevelError,
				"Couldn't set messages. Wii remote will connect, but reporting and other things won't work!");
	                }
	                else {
	                    mesg = 1;
	                }
	            }
	            else {
		    if (cwiid_disable(cmpwiimote->wiimote, CWIID_FLAG_MESG_IFC)) {
			compLogMessage ("wiimote", CompLogLevelError,
			"Couldn't disable messages");
	            }
		        else {
			        mesg = 0;
		        }
	            }
	            
	            /* Request Status */
	
	            if (cwiid_request_status(cmpwiimote->wiimote)) {
		        compLogMessage ("wiimote", CompLogLevelError,
			"Error requesting status. Battery info will not work!");
	            }
	            
	            /* Request State */
	            
	            if (cwiid_get_state(cmpwiimote->wiimote, &state)) {
		        compLogMessage ("wiimote", CompLogLevelError,
			"Error requesting state. Calibration will not work!");
	            }
	            
	            /* TODO:
	             * Toggle certain LED's for certain
	             * Wii Remote 'Types'
	             */
	            
	            toggle_bit(led_state, CWIID_LED1_ON);
	            set_led_state(cmpwiimote->wiimote, led_state);
	            
	            /* Do a fancy thing with the LED's. Doesn't
	             * quite work right yet :(
	             */

		    cmpwiimote->count = 0;

	            cmpwiimote->lightsTimeoutHandle =
		    compAddTimeout (650, 650 * 1.2 /* I'll do the math later */, wiimoteChangeLights, cmpwiimote);
	            
	            /* Set some variables using the state interface
	             * FIXME: For some reason, over-querying the 
	             * state interface can result in a Mesg Pipe
	             * overflow which in turn causes a cwiid_err.
	             * This is no good because it brings down compiz!
	             * Of course, we only poll the value once, but it 
	             * _should_ be fixed in CWiiD
	             */
	             
	            cmpwiimote->acc.initAccX = state.acc[CWIID_X];
	            cmpwiimote->acc.initAccY = state.acc[CWIID_Y];

		    /* Set the id */

		    cmpwiimote->id = cwiid_get_id (cmpwiimote->wiimote);

		    reloadOptionsForWiimote (d, cmpwiimote);
		    reloadGesturesForWiimote (d, cmpwiimote);
		    reloadReportersForWiimote (d, cmpwiimote);             
	            
	            /* Increment the Wiimote iter to
	             * allow another CompWiimote to initialise
	             */
	            
	            ad->nWiimote++;
	        }
	        else
	        {
		        compLogMessage ("wiimote", CompLogLevelError,
			"Maximum number of Wii Remotes reached. If you want to enable more,"\
			"change the macro MAX_WIIMOTES in wiimote.c. This will be fixed in "\
			"the future with actual memory allocation");
	        }
	        
	        /* No more checking */
	        return FALSE;
	  }
	    
	  return TRUE;
}
