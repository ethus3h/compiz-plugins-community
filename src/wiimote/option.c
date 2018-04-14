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

/* Option Initialisation --------------------------------------------------- */

void
reloadReportersForWiimote (CompDisplay *d,
			   CompWiimote *wiimote)
{
	CompListValue *cWiimoteNumber = wiimoteGetReportWiimoteNumber (d);
	CompListValue *cReportType = wiimoteGetReportType (d);
	CompListValue *cPluginName = wiimoteGetReportPluginName (d);
	CompListValue *cActionName = wiimoteGetReportActionName (d);
	CompListValue *cReportSensitivity = wiimoteGetReportSensitivity (d);

	CompListValue *cXArgument = wiimoteGetReportXArgument (d);
	CompListValue *cYArgument = wiimoteGetReportYArgument (d);
	CompListValue *cZArgument = wiimoteGetReportZArgument (d);

	CompListValue *cDataType = wiimoteGetReportDataType (d);

	int nReport;
	int iReport = 0;

	if ((cWiimoteNumber->nValue != cReportType->nValue) ||
            (cWiimoteNumber->nValue != cPluginName->nValue) ||
            (cWiimoteNumber->nValue != cActionName->nValue) ||
            (cWiimoteNumber->nValue != cXArgument->nValue)  ||
            (cWiimoteNumber->nValue != cYArgument->nValue)  ||
            (cWiimoteNumber->nValue != cZArgument->nValue)  ||
	    (cWiimoteNumber->nValue != cDataType->nValue)   ||
	    (cWiimoteNumber->nValue != cReportSensitivity->nValue - 1))
	{
	compLogMessage ("wiimote", CompLogLevelError,
		"Options (reporters) are not set correctly. Please revisit them and make sure each option in the list is set\n");
	return;
	}


	nReport = cWiimoteNumber->nValue;
	iReport = 0;
	wiimote->nReport = 0;

	while (nReport-- && iReport < MAX_REPORTS - 1)
	{
		if (cWiimoteNumber->value[iReport].i == wiimote->id)
		{
			/* And set them */
			wiimote->report[wiimote->nReport].set = TRUE;
			wiimote->report[wiimote->nReport].pluginName = strdup(cPluginName->value[iReport].s);
			wiimote->report[wiimote->nReport].actionName = strdup(cActionName->value[iReport].s);
			wiimote->report[wiimote->nReport].sensitivity = cReportSensitivity->value[iReport].i;

			wiimote->report[wiimote->nReport].xarg = strdup(cXArgument->value[iReport].s);
			wiimote->report[wiimote->nReport].yarg = strdup(cYArgument->value[iReport].s);
			wiimote->report[wiimote->nReport].zarg = strdup(cZArgument->value[iReport].s);

			wiimote->report[wiimote->nReport].type = cReportType->value[iReport].i;
			wiimote->report[wiimote->nReport].dataType = cDataType->value[iReport].i;

			wiimote->nReport++;
		}

		iReport++;
	}
}

void
reloadReportersForWiimoteNumber (CompDisplay *d,
				 int wiimoteNumber)
{
    CompWiimote *wiimote;
    WIIMOTE_DISPLAY (d);

    for (wiimote = ad->wiimotes; wiimote; wiimote = wiimote->next)
    {	
	if (wiimote->id == wiimoteNumber)
	{
	    reloadReportersForWiimote (d, wiimote);
	    break;
	}
    }
}

void
reloadGesturesForWiimote (CompDisplay *d,
			  CompWiimote *wiimote)
{
	CompListValue *cWiimoteNumber = wiimoteGetGestureWiimoteNumber (d);
	CompListValue *cGestureType = wiimoteGetGestureType (d);
	CompListValue *cPluginName = wiimoteGetGesturePluginName (d);
	CompListValue *cActionName = wiimoteGetGestureActionName (d);
	CompListValue *cGestureSensitivity = wiimoteGetGestureSensitivity (d);

	int nGesture;
	int iGesture = 0;

	if ((cWiimoteNumber->nValue != cGestureType->nValue) ||
            (cWiimoteNumber->nValue != cPluginName->nValue)  ||
            (cWiimoteNumber->nValue != cGestureSensitivity->nValue - 1)  ||
            (cWiimoteNumber->nValue != cActionName->nValue))
	{
	    compLogMessage ("wiimote", CompLogLevelError,
	    "Options (gestures) are not set correctly. Please revisit them and make sure each option in the list is set\n");
	return;
	}

	nGesture = cWiimoteNumber->nValue;
	iGesture = 0;
	wiimote->nGesture = 0;

	while (nGesture-- && iGesture < MAX_GESTURES)
	{
		if (cWiimoteNumber->value[iGesture].i == wiimote->id)
		{
			wiimote->gesture[wiimote->nGesture].set = TRUE;	
			wiimote->gesture[wiimote->nGesture].pluginName = strdup(cPluginName->value[iGesture].s);
			wiimote->gesture[wiimote->nGesture].actionName = strdup(cActionName->value[iGesture].s);
			wiimote->gesture[wiimote->nGesture].sensitivity = cGestureSensitivity->value[iGesture].i;				
			wiimote->gesture[wiimote->nGesture].type = cGestureType->value[iGesture].i;
			wiimote->nGesture++;
			
		}
	iGesture++;
	}
}

void
reloadGesturesForWiimoteNumber (CompDisplay *d,
				int wiimoteNumber)
{
    CompWiimote *wiimote;
    WIIMOTE_DISPLAY (d);

    for (wiimote = ad->wiimotes; wiimote; wiimote = wiimote->next)
    {	
	if (wiimote->id == wiimoteNumber)
	{
	    reloadGesturesForWiimote (d, wiimote);
	    break;
	}
    }
}

void
reloadOptionsForWiimote (CompDisplay *d,
			 CompWiimote *wiimote)
{
	CompListValue *cXCal = wiimoteGetXCalibrationMul (d);
	CompListValue *cYCal = wiimoteGetYCalibrationMul (d);
	CompListValue *cXAdj = wiimoteGetXAdjust (d);
	CompListValue *cYAdj = wiimoteGetYAdjust (d);

	if ((cXCal->nValue != cYCal->nValue) ||
            (cXCal->nValue != cXAdj->nValue)  ||
            (cXCal->nValue != cYAdj->nValue))
	{
	    compLogMessage ("wiimote", CompLogLevelError,
	    "Options (general) are not set correctly. Please revisit them and make sure each option in the list is set\n");
	return;
	}

	if (wiimote->id < cXCal->nValue)
	{
	    wiimote->irMulX = cXCal->value[wiimote->id].f;
	    wiimote->irMulY = cYCal->value[wiimote->id].f;
	    wiimote->irSubX = cXAdj->value[wiimote->id].f;
	    wiimote->irSubY = cYAdj->value[wiimote->id].f;
	}
}

void
reloadOptionsForWiimoteNumber (CompDisplay *d,
			       int wiimoteNumber)
{
    CompWiimote *wiimote;
    WIIMOTE_DISPLAY (d);

    for (wiimote = ad->wiimotes; wiimote; wiimote = wiimote->next)
    {	
	if (wiimote->id == wiimoteNumber)
	{
	    reloadOptionsForWiimote (d, wiimote);
	    break;
	}
    }
}


