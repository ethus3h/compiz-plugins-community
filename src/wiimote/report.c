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

/* Value Reporting --------------------------------------------------- */

Bool sendReports(void *vs)
{
	CompScreen *s = vs;
	CompWiimote *wiimote;
	WIIMOTE_DISPLAY (s->display);

	for (wiimote = ad->wiimotes; wiimote; wiimote = wiimote->next)
	{
		int j;

		if (wiimote->connected)
		{
			for (j = 0; j < wiimote->nReport; j++)
			{
			        float report[3] = { 0 };
				int sens = wiimote->report[j].sensitivity;

				switch (wiimote->report[j].type)
				{
					case WiimoteReportTypeNone:
					case WiimoteReportTypeLength:
					default:
						break;
					case WiimoteReportTypeIR:
						report[0] = wiimote->irMidX - (CWIID_IR_X_MAX / 4);
						report[1] = wiimote->irMidY - (CWIID_IR_Y_MAX / 4);
						report[2] = wiimote->irDistance;
						break;
					case WiimoteReportTypeAccellerometer:
						report[0] = (float) ( (int) (wiimote->acc.accDX / sens));
						report[1] = (float) ( (int) (wiimote->acc.accDY / sens));
						report[2] = (float) ( (int) (wiimote->acc.accDZ / sens));
						break;
					case WiimoteReportTypeNunchuckAccellerometer:
						report[0] = (float) ( (int) (wiimote->nunchuck.accDX / sens));
						report[1] = (float) ( (int) (wiimote->nunchuck.accDY / sens));
						report[2] = (float) ( (int) (wiimote->nunchuck.accDZ / sens));
						break;
					case WiimoteReportTypeNunchuckStick:
						report[0] = (float) ( (int) (wiimote->nunchuck.stickDX / sens));
						report[1] = (float) ( (int) (wiimote->nunchuck.stickDY / sens));
						report[2] = 0.0f;
						break;
				}

				CompOption argument[5];
				int nArgument = 0;

				argument[nArgument].name = "window";
				argument[nArgument].type = CompOptionTypeInt;
				argument[nArgument].value.i = s->display->activeWindow;
				nArgument++;

				argument[nArgument].name = "root";
				argument[nArgument].type = CompOptionTypeInt;
				argument[nArgument].value.i = s->root;
				nArgument++;

				if (wiimote->report[j].xarg)
					argument[nArgument].name = wiimote->report[j].xarg;
				else
					argument[nArgument].name = "x";
				
				if (wiimote->report[j].dataType == 0)
				{
					argument[nArgument].type = CompOptionTypeFloat;
					argument[nArgument].value.f = report[0];
				}
				else if (wiimote->report[j].dataType == 1)
				{
					argument[nArgument].type = CompOptionTypeInt;
					argument[nArgument].value.i = (int) report[0];
				}
				nArgument++;

				if (wiimote->report[j].yarg)
					argument[nArgument].name = wiimote->report[j].yarg;
				else
					argument[nArgument].name = "y";

				if (wiimote->report[j].dataType == 0)
				{
					argument[nArgument].type = CompOptionTypeFloat;
					argument[nArgument].value.f = report[1];
				}
				else if (wiimote->report[j].dataType == 1)
				{
					argument[nArgument].type = CompOptionTypeInt;
					argument[nArgument].value.i = (int) report[1];
				}
				nArgument++;

				if (wiimote->report[j].yarg)
					argument[nArgument].name = wiimote->report[j].yarg;
				else
					argument[nArgument].name = "z";

				if (wiimote->report[j].dataType == 0)
				{
					argument[nArgument].type = CompOptionTypeFloat;
					argument[nArgument].value.f = report[2];
				}
				else if (wiimote->report[j].dataType == 1)
				{
					argument[nArgument].type = CompOptionTypeInt;
					argument[nArgument].value.i = (int) report[2];
				}
				nArgument++;

				if (wiimote->report[j].pluginName && wiimote->report[j].actionName){
				sendInfoToPlugin (s->display, argument, nArgument,
						  wiimote->report[j].pluginName,
						  wiimote->report[j].actionName);
				}
			}
		}
		
	}

	return TRUE;

}
