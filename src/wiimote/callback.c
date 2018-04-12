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


/* Prototype cwiid_callback with cwiid_callback_t, define it with the actual
 * type - this will cause a compile error (rather than some undefined bizarre
 * behavior) if cwiid_callback_t changes */
/* cwiid_mesg_callback_t has undergone a few changes lately, hopefully this
 * will be the last.  Some programs need to know which messages were received
 * simultaneously (e.g. for correlating accelerometer and IR data), and the
 * sequence number mechanism used previously proved cumbersome, so we just
 * pass an array of messages, all of which were received at the same time.
 * The id is to distinguish between multiple wiimotes using the same callback.
 * */
 
/* Essentially what this does, is a small callback whenever cwiid_callback (The
 * prototype to cwiid_callback_t) is 'called' by cwiid_callback_t (All the time),
 * it will print out status reports. It is NOT a loop inside this plugin, but instead
 * is called with a number of informations about the Wii Remote at the time.
 */
 
/* Since we can't actually get a CompDisplay *d into the arguements
 * we kind of cheat and use the first display provided by CompCore *c.
 * Obviously this is going to become depracted as soon as David releases
 * some sort of system to have multiple diplays, but I assume that we
 * can move this information into the core structs when that happens.
 * (Assuming that there is only one core)
 */

void wiimoteCWiiDCallback(cwiid_wiimote_t *wiimote, int mesg_count,
                    union cwiid_mesg mesg[], struct timespec *timestamp)
{
	int i, j;
	float totalX = 0.0f, totalY = 0.0f;
	int count = 0;
	int id = cwiid_get_id (wiimote);
	struct cwiid_state state;
	CompWiimote *cmpwiimote;

	cwiid_get_state(wiimote, &state);
	//unsigned char led_state = 0;
	
	WIIMOTE_DISPLAY (firstDisplay);
	//cmpwiimote->nunchuck.connected = FALSE;

	for (cmpwiimote = ad->wiimotes; cmpwiimote; cmpwiimote = cmpwiimote->next)
	{
	    if (cmpwiimote->id == id)
		break;
	}

	if (wiimote)
	{
	for (i=0; i < mesg_count; i++)
	{
		switch (mesg[i].type) {
		case CWIID_MESG_STATUS:
			switch (mesg[i].status_mesg.ext_type) {
			case CWIID_EXT_NONE:
				break;
			case CWIID_EXT_NUNCHUK:
				cmpwiimote->nunchuck.connected = TRUE;
				cmpwiimote->nunchuck.initAccX =
				state.ext.nunchuk.acc[CWIID_X];
				cmpwiimote->nunchuck.initAccY =
				state.ext.nunchuk.acc[CWIID_Y];
				cmpwiimote->nunchuck.initAccZ =
				state.ext.nunchuk.acc[CWIID_Z];
				cmpwiimote->nunchuck.initStickX =
				state.ext.nunchuk.stick[CWIID_X];
				cmpwiimote->nunchuck.initStickY =
				state.ext.nunchuk.stick[CWIID_Y];
				wiimoteProcessNunchuckButtons(firstDisplay, cmpwiimote, (struct cwiid_nunchuk_mesg *) &mesg[i]); // Just pass the pointer methinks
				break;
			case CWIID_EXT_CLASSIC:
				break;
			default:
				break;
			}
			printf("\n");
			break;
		case CWIID_MESG_BTN:
			wiimoteProcessButtons(firstDisplay, cmpwiimote, (struct cwiid_btn_mesg *) &mesg[i]); // Just pass the pointer
			break;
		case CWIID_MESG_ACC:
			cmpwiimote->acc.accDX = ((cmpwiimote->acc.accX) - (cmpwiimote->acc.initAccX));
			cmpwiimote->acc.accDY = ((cmpwiimote->acc.accY) - (cmpwiimote->acc.initAccY));
			cmpwiimote->acc.accX = mesg[i].acc_mesg.acc[CWIID_X];
			cmpwiimote->acc.accY = mesg[i].acc_mesg.acc[CWIID_Y];
			cmpwiimote->acc.accZ = mesg[i].acc_mesg.acc[CWIID_Z];
			break;
		case CWIID_MESG_IR:
			for (j = 0; j < CWIID_IR_SRC_COUNT; j++) {
				if (mesg[i].ir_mesg.src[j].valid) {
					cmpwiimote->ir[j].valid = TRUE;
					cmpwiimote->ir[j].x =  mesg[i].ir_mesg.src[j].pos[CWIID_X];
					cmpwiimote->ir[j].y =  mesg[i].ir_mesg.src[j].pos[CWIID_Y];
					cmpwiimote->ir[j].size = mesg[i].ir_mesg.src[j].size;
					totalX += mesg[i].ir_mesg.src[j].pos[CWIID_X];
					totalY += mesg[i].ir_mesg.src[j].pos[CWIID_Y];
					count++;
				}
				else
				{
				    cmpwiimote->ir[j].valid = FALSE;
				}
			}
			break;
		case CWIID_MESG_NUNCHUK:
			cmpwiimote->nunchuck.connected = TRUE;
			cmpwiimote->nunchuck.accDX = ((cmpwiimote->nunchuck.accX) - (cmpwiimote->nunchuck.initAccX));
			cmpwiimote->nunchuck.accDY = ((cmpwiimote->nunchuck.accY) - (cmpwiimote->nunchuck.initAccY));
			cmpwiimote->nunchuck.accX = mesg[i].nunchuk_mesg.acc[CWIID_X];
			cmpwiimote->nunchuck.accY = mesg[i].nunchuk_mesg.acc[CWIID_Y];
			cmpwiimote->nunchuck.accZ = mesg[i].nunchuk_mesg.acc[CWIID_Z];
			cmpwiimote->nunchuck.stickDX = ((cmpwiimote->nunchuck.stickX) - (cmpwiimote->nunchuck.initStickX));
			cmpwiimote->nunchuck.stickDY = ((cmpwiimote->nunchuck.stickY) - (cmpwiimote->nunchuck.initStickY));
			cmpwiimote->nunchuck.stickX = mesg[i].nunchuk_mesg.stick[CWIID_X];
			cmpwiimote->nunchuck.stickY = mesg[i].nunchuk_mesg.stick[CWIID_Y];
			break;
		case CWIID_MESG_CLASSIC:
			printf("Classic Report: btns=%.4X l_stick=(%d,%d) r_stick=(%d,%d) "
			       "l=%d r=%d\n", mesg[i].classic_mesg.buttons,
			       mesg[i].classic_mesg.l_stick[CWIID_X],
			       mesg[i].classic_mesg.l_stick[CWIID_Y],
			       mesg[i].classic_mesg.r_stick[CWIID_X],
			       mesg[i].classic_mesg.r_stick[CWIID_Y],
			       mesg[i].classic_mesg.l, mesg[i].classic_mesg.r);
			break;
		case CWIID_MESG_ERROR:
			if (cwiid_close(wiimote)) {
				fprintf(stderr, "Error on wiimote disconnect\n");
				exit(-1);
			}
			exit(0);
			break;
		default:
			break;
		}
	}
	    
	/* Find the minimum and maximum points */
	   
	int min, max;
	float xRange = 0.0f;
        if (cmpwiimote->ir[1].valid)
        {
            cmpwiimote->irMidX = (totalX / count);
            cmpwiimote->irMidY = (totalY / count);
        }
        else
        {
            cmpwiimote->irMidX = 1.0f;
            cmpwiimote->irMidY = 1.0f;
        }
        if (cmpwiimote->ir[0].valid && cmpwiimote->ir[1].valid)
        {
            min = findMinIR (firstDisplay, cmpwiimote,
					 cmpwiimote->ir[0].x,
					 cmpwiimote->ir[1].x,
					 cmpwiimote->ir[2].x,
					 cmpwiimote->ir[3].x);

            max = findMaxIR (firstDisplay, cmpwiimote,
					 cmpwiimote->ir[0].x,
					 cmpwiimote->ir[1].x,
					 cmpwiimote->ir[2].x,
					 cmpwiimote->ir[3].x);
            
            xRange = max - min;
        }
        else
        {
            xRange = 1.0;
        }
        cmpwiimote->irDistance = (xRange / 100);
	}
        
}
