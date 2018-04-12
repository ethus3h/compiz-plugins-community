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

/* Utility Functions --------------------------------------------------- */

/* Error Handling */
void err(cwiid_wiimote_t *wiimote, const char *s, va_list ap)
{
	if (wiimote) printf("%d:", cwiid_get_id(wiimote));
	else printf("-1:");
	vprintf(s, ap);
	printf("\n");
}

/* Set LED Wrapper */
void set_led_state(cwiid_wiimote_t *wiimote, unsigned char led_state)
{
	if (cwiid_set_led(wiimote, led_state)) {
		fprintf(stderr, "Error setting LEDs \n");
	}
}

/* Set Reporting Mode Wrapper*/
void set_rpt_mode(cwiid_wiimote_t *wiimote, unsigned char rpt_mode)
{
	if (cwiid_set_rpt_mode(wiimote, rpt_mode)) {
		fprintf(stderr, "Error setting report mode\n");
	}
}

/* Wii Remote Light Show */

Bool wiimoteChangeLights(void *closure)
{
    CompWiimote *cwiimote = (CompWiimote *) closure;
    cwiid_wiimote_t *wiimote = cwiimote->wiimote;
    int led_state = 0;
    cwiimote->count++;
    switch (cwiimote->count)
    {
        case 1:
            {
                toggle_bit(led_state, CWIID_LED1_ON);
	            set_led_state(wiimote, led_state);
	        }
	        break;
	    case 2:
            {
                toggle_bit(led_state, CWIID_LED2_ON);
	            set_led_state(wiimote, led_state);
	        }
	        break;
	    case 3:
            {
                toggle_bit(led_state, CWIID_LED3_ON);
	            set_led_state(wiimote, led_state);
	        }
	        break;
	    case 4:
            {
                toggle_bit(led_state, CWIID_LED4_ON);
	            set_led_state(wiimote, led_state);
	        }
	        break;
	    case 5:
            {
                toggle_bit(led_state, CWIID_LED3_ON);
	            set_led_state(wiimote, led_state);
	        }
	        break;
	    case 6:
            {
                toggle_bit(led_state, CWIID_LED2_ON);
	            set_led_state(wiimote, led_state);
	        }
	        break;
	    case 7:
            {
                toggle_bit(led_state, CWIID_LED1_ON);
	            set_led_state(wiimote, led_state);
	            cwiimote->count = 1;
	        }
	        break;
	}

		
	return TRUE;
}

/* Utility */
int findMinIR (CompDisplay *d, CompWiimote *wiimote, int ir1, int ir2, int ir3, int ir4)
{
    
    int min = ir1;
    
    if ((ir2 < min) && wiimote->ir[1].valid)
        min = ir2;
    if ((ir3 < min) && wiimote->ir[2].valid)
        min = ir3;
    if ((ir4 < min) && wiimote->ir[3].valid)
        min = ir4;
        
    return min;
}

int findMaxIR (CompDisplay *d, CompWiimote *wiimote, int ir1, int ir2, int ir3, int ir4)
{
    
    int max = ir1;
    
    if ((ir2 > max) && wiimote->ir[1].valid)
        max = ir2;
    if ((ir3 > max) && wiimote->ir[2].valid)
        max = ir3;
    if ((ir4 > max) && wiimote->ir[3].valid)
        max = ir4;
        
    return max;
}


