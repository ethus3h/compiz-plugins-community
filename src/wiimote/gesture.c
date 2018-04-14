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

/* Gesturing and gesture checking --------------------------------------------- */

#define CHECK_BUTTON(a,b,c,e) \
	if (mesg->buttons & a) \
	{ \
		if (!b) \
			sendGesture(d, wiimote, c); \
		b = TRUE; \
	} \
	else \
	{ \
		if (b) \
			sendGesture(d, wiimote, e); \
		b = FALSE; \
	} \


static Bool sendGesture(CompDisplay *d, CompWiimote *wiimote, int type)
{
	WIIMOTE_DISPLAY (d);
	CompOption argument[2];
	int nArgument = 0;
	int i;

	if (wiimote->connected)
	{
		for (i = 0; i < wiimote->nGesture; i++)
		{
			if (wiimote->gesture[i].type == type) // Need to fix this
			{
				argument[nArgument].name = "window";
				argument[nArgument].type = CompOptionTypeInt;
				argument[nArgument].value.i = d->activeWindow;
				nArgument++;

				argument[nArgument].name = "root";
				argument[nArgument].type = CompOptionTypeInt;
				argument[nArgument].value.i = ad->firstRoot;
				nArgument++;

				sendInfoToPlugin (d, argument, nArgument,
						  wiimote->gesture[i].pluginName,
						  wiimote->gesture[i].actionName);
			}
		}
	}

	return TRUE;
}

void
wiimoteProcessButtons(CompDisplay *d, CompWiimote *wiimote, struct cwiid_btn_mesg *mesg)
{

	CHECK_BUTTON( CWIID_BTN_A, wiimote->buttons.A, 1, 2);
	CHECK_BUTTON( CWIID_BTN_B, wiimote->buttons.B, 3, 4);
	CHECK_BUTTON( CWIID_BTN_UP, wiimote->buttons.Up, 5, 6);
	CHECK_BUTTON( CWIID_BTN_DOWN, wiimote->buttons.Down, 7, 8);
	CHECK_BUTTON( CWIID_BTN_LEFT, wiimote->buttons.Left, 9, 10);
	CHECK_BUTTON( CWIID_BTN_RIGHT, wiimote->buttons.Right, 11, 12);
	CHECK_BUTTON( CWIID_BTN_PLUS, wiimote->buttons.Plus, 13, 14);
	CHECK_BUTTON( CWIID_BTN_MINUS, wiimote->buttons.Minus, 15, 16);
	CHECK_BUTTON( CWIID_BTN_HOME, wiimote->buttons.Home, 17, 18);
	CHECK_BUTTON( CWIID_BTN_1, wiimote->buttons.One, 19, 20);
	CHECK_BUTTON( CWIID_BTN_2, wiimote->buttons.Two, 21, 22);

}

void
wiimoteProcessNunchuckButtons (CompDisplay *d, CompWiimote *wiimote, struct cwiid_nunchuk_mesg *mesg)
{

	CHECK_BUTTON ( CWIID_NUNCHUK_BTN_C, wiimote->nunchuck.buttons.C, 23, 24);
	CHECK_BUTTON ( CWIID_NUNCHUK_BTN_Z, wiimote->nunchuck.buttons.Z, 25, 26);
}

/* I would rather use #define, but it's bugged up, so I have to compromise */

static void
wiimoteCheckGesture (CompScreen *s, CompWiimote *wiimote, float diff, float init, float *old, float value, int posGest, int negGest)
{
	if (*old <= value - diff && value <= init)
		sendGesture (s->display, wiimote, negGest);
	if (*old >= value + diff && value >= init)
		sendGesture (s->display, wiimote, posGest);

	*old = value;
}

Bool wiimoteCheckForGestures (void *vs)
{
	CompScreen *s = (CompScreen *) vs;
	CompWiimote *wiimote;
	float sens = wiimoteGetGestureSens (s->display);	

	WIIMOTE_DISPLAY (s->display);

        for (wiimote = ad->wiimotes; wiimote; wiimote = wiimote->next)
	{
		if (wiimote->connected)
		{
			/* Check Accellerometer */

			wiimoteCheckGesture (s, wiimote, sens, wiimote->acc.initAccY,
					      &wiimote->acc.oldAccY,
					      wiimote->acc.accY, 27, 28);
			wiimoteCheckGesture (s, wiimote, sens, wiimote->acc.initAccX,
					      &wiimote->acc.oldAccX,
					      wiimote->acc.accX, 29, 30);

			/* Check Nunchuk */
			if (wiimote->nunchuck.connected)
			{
				wiimoteCheckGesture ( s, wiimote, sens, wiimote->nunchuck.initAccY,
					              &wiimote->nunchuck.oldAccY,
						      wiimote->nunchuck.accY, 31, 32 );
				wiimoteCheckGesture ( s, wiimote, sens, wiimote->nunchuck.initAccX,
					              &wiimote->nunchuck.oldAccX,
						      wiimote->nunchuck.accX, 33, 34 );
				wiimoteCheckGesture ( s, wiimote, sens, wiimote->nunchuck.initStickY,
						      &wiimote->nunchuck.oldStickY,
						      wiimote->nunchuck.stickY, 35, 36 );
				wiimoteCheckGesture ( s, wiimote, sens, wiimote->nunchuck.initStickX,
						      &wiimote->nunchuck.oldStickX,
						      wiimote->nunchuck.stickX, 37, 38 );
			}
		}
	}
	return TRUE;
}
