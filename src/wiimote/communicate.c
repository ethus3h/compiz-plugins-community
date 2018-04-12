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

/* Inter-Plugin Communication --------------------------------------------------- */

Bool
sendInfoToPlugin (CompDisplay *d, CompOption *argument, int nArgument, char *pluginName, char *actionName)
{
	Bool pluginExists = FALSE;
	CompOption *options, *option;
	CompPlugin *p;
	CompObject *o;
	int nOptions;

	p = findActivePlugin (pluginName);
	o = compObjectFind (&core.base, COMP_OBJECT_TYPE_DISPLAY, NULL);

	if (!o)
		return FALSE;

	if (!p || !p->vTable->getObjectOptions)
	{
		compLogMessage ("wiimote", CompLogLevelError,
		"Reporting plugin '%s' does not exist!", pluginName);
		return FALSE;
	}

	if (p && p->vTable->getObjectOptions)
	{
		options = (*p->vTable->getObjectOptions) (p, o, &nOptions);
		option = compFindOption (options, nOptions, actionName, 0);
		pluginExists = TRUE;
	}

	if (pluginExists)
	{
		if (option && option->value.action.initiate)
		{
			(*option->value.action.initiate)  (d,
		                           &option->value.action,
		                           0,
		                           argument,
		                           nArgument);
		}
		else
		{
			compLogMessage ("wiimote", CompLogLevelError,
			"Plugin '%s' does exist, but no option named '%s' was found!", pluginName, actionName);
			return FALSE;
		}
	}

	return TRUE;
}
