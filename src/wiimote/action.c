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

/* Callable Actions --------------------------------------------------- */

Bool
wiimoteToggle (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState cstate,
		 CompOption      *option,
		 int             nOption)
{
	int rc;
	CompWiimote *wiimote;

	WIIMOTE_DISPLAY (d);
	
	/* Create second thread attributes */
      
        pthread_attr_t secondThreadAttr;
	pthread_attr_init(&secondThreadAttr);
	pthread_attr_setdetachstate(&secondThreadAttr, PTHREAD_CREATE_JOINABLE);
	    
	CompWindow *w;
    	w = findWindowAtDisplay (d, getIntOptionNamed (option, nOption,
    							       "window", 0));
	if (!w) 
	    return TRUE;

	CompScreen *s;
    	s = findScreenAtDisplay (d, getIntOptionNamed (option, nOption,
    							       "root", 0));
	if (s)
	{
		ad->firstRoot = s->root;

		/* Create Message */
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
		arg[nArg].value.s = "Put your Wii Remote in a neutral position and \n"\
				    "hold down the 1 and 2 Buttons now";
		nArg++;

		arg[nArg].name = "timeout";
		arg[nArg].type = CompOptionTypeInt;
		arg[nArg].value.i = 2000;

		sendInfoToPlugin (d, arg, nArg,
		"prompt",
		"display_text");
		compLogMessage ("wiimote", CompLogLevelInfo,
		"Hold down the 1 and 2 Buttons on your Nintendo Wii Remote now");

		/* Initialize memory to save wii remove data */
		
		wiimote = wiimoteAddWiimote (d);
		if (wiimote)
		{

		    /* Create the second thread. */
		    rc = pthread_create(&wiimote->connectWiimote, &secondThreadAttr, connectWiimote, (void *)d);
		
		    /* Clean up second thread's attribute. */
		    pthread_attr_destroy(&secondThreadAttr);
		    if (!rc)
		    {
			/* So the thread succeeded!
			 * Start Checking! */
			ad->checkingTimeoutHandle = compAddTimeout (10, 10 * 1.2, checkConnected, d);
		    }
		}
	}
    return TRUE;
}

Bool
wiimoteDisable (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState cstate,
		 CompOption      *option,
		 int             nOption)
{

	int rc;
	CompWiimote *wiimote;
	WIIMOTE_DISPLAY (d);

	for (wiimote = ad->wiimotes; wiimote->next; wiimote = wiimote->next) ;

	/* nWiimote -1 because we increment nWiimote
	 * so we need to find the most recent (Which is -1)
	 */


	if (ad->nWiimote < 1)
		return FALSE;

	wiimote->initiated = FALSE;
	wiimote->connected = FALSE;

	/* Wait for the second thread to return control to itself */

	rc = pthread_join(wiimote->connectWiimote, NULL);
	if (rc)
		compLogMessage ("wiimote", CompLogLevelError,
		"Threading error occurred.\n");

	/* Remove timeout handles */

	if (ad->infoTimeoutHandle)
		compRemoveTimeout (ad->infoTimeoutHandle);

	if (ad->gestureTimeoutHandle)
		compRemoveTimeout (ad->gestureTimeoutHandle);
	
	/* Remove the lights timeout handle */

	if (wiimote->lightsTimeoutHandle)
	compRemoveTimeout (wiimote->lightsTimeoutHandle);

	/* Tell the user that we are closing the connection */

	compLogMessage ("wiimote", CompLogLevelInfo,
	"Closing connection to most recently connected Wii Remote\n");
	if (cwiid_close(wiimote->wiimote)) {
		compLogMessage ("wiimote", CompLogLevelError,
		"Error closing that connection\n");
		return -1;
	}

	/* Decrement Wiimote Iter */

	wiimoteRemoveWiimote (d, wiimote);

	ad->nWiimote--;

	return TRUE;
}



Bool
wiimoteSendInfo (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState cstate,
		 CompOption      *option,
		 int             nOption)
{
    WIIMOTE_DISPLAY (d);
    
    Window     root;
    CompScreen *s;

    ad->report = !ad->report;

    if (!ad->report)
    {
	if (ad->infoTimeoutHandle)
		compRemoveTimeout (ad->infoTimeoutHandle);
	return FALSE;
    }

    root = getIntOptionNamed (option, nOption, "root", 0);
    s    = findScreenAtDisplay (d, root);
    
    if (s)
    {
        /* Add a timeout to send info to another
         * plugin. The preferable way would be for
         * other plugins to get the values when they
         * need them, but this is just a workaround
         * for plugins that support the action system
         */

        ad->infoTimeoutHandle = compAddTimeout(wiimoteGetPollInterval (s->display) ,
                                               wiimoteGetPollInterval (s->display) * 1.2, sendReports, s);
	ad->gestureTimeoutHandle = compAddTimeout(wiimoteGetGestureTimeout (s->display) ,
                                                  wiimoteGetGestureTimeout (s->display) * 1.2, wiimoteCheckForGestures, s);
    }
    return TRUE;
}
