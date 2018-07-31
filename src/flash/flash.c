/**
*
* Compiz plugin Flash
*
* Copyright : (C) 2007 by Jean-Fran�is Souville & Charles Jeremy
* E-mail    : souville@ecole.ensicaen.fr , charles@ecole.ensicaen.fr
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


/**
 *    FLASH - macro names
 *    Flash - struct names
 *    flash - function names
 *    fd  - FlashDisplay variable name
 *    fs  - FlashScreen variable name
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <compiz-core.h>

#define OPTION_DISTANCE_MAX 1024

#define FLASH_DISPLAY_OPTION_INITIATE_BUTTON  0
#define FLASH_DISPLAY_OPTION_INITIATE_KEY     1
#define FLASH_DISPLAY_OPTION_DISTANCE_MAX     2
#define FLASH_DISPLAY_OPTION_ECART_MAX        3
#define FLASH_DISPLAY_OPTION_INTERVAL         4
#define FLASH_DISPLAY_OPTION_TIME             5
#define FLASH_DISPLAY_OPTION_COLOR_UP_LEFT    6
#define FLASH_DISPLAY_OPTION_COLOR_CENTER     7
#define FLASH_DISPLAY_OPTION_COLOR_DOWN_RIGHT 8
#define FLASH_DISPLAY_OPTION_NUM              9

#define FLASH_SCREEN_OPTION_WINDOW_TYPE  0
#define FLASH_SCREEN_OPTION_NUM          1

/*
 * Helpers
 *
 */
#define GET_FLASH_DISPLAY(d) \
	((FlashDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define FLASH_DISPLAY(d)  \
	FlashDisplay *fd = GET_FLASH_DISPLAY (d)

#define GET_FLASH_SCREEN(s, fd)  \
	((FlashScreen *) (s)->base.privates[(fd)->screenPrivateIndex].ptr)

#define FLASH_SCREEN(s)  \
	FlashScreen *fs = GET_FLASH_SCREEN (s, GET_FLASH_DISPLAY (s->display))

#define GET_FLASH_WINDOW(w, fs)  \
	((FlashWindow *) (w)->base.privates[(fs)->windowPrivateIndex].ptr)

#define FLASH_WINDOW(w)  \
	FlashWindow *fw = GET_FLASH_WINDOW  \
				(w, GET_FLASH_SCREEN  (w->screen,  \
				GET_FLASH_DISPLAY (w->screen->display)))

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))

#define POS_TOP 0
#define POS_BOTTOM 1
#define POS_LEFT 2
#define POS_RIGHT 3

/*
 * pointer to display list
 */
static int displayPrivateIndex;

static CompMetadata flashMetadata;

/*
 * FlashDisplay structure
 */
typedef struct _FlashDisplay {
    int		                screenPrivateIndex;
    CompOption                  opt[FLASH_DISPLAY_OPTION_NUM];
    HandleEventProc             handleEvent;
} FlashDisplay;

/*
 * FlashScreen structure
 */
typedef struct _FlashScreen {

    int                         windowPrivateIndex;
    CompOption                  opt[FLASH_SCREEN_OPTION_NUM];

    DonePaintScreenProc         donePaintScreen;
    PaintOutputProc             paintOutput;
    PreparePaintScreenProc      preparePaintScreen;

    int pointX, pointY;
    int winX,winY,winW,winH;

    double** CoordinateArray;

    int tailleTab;

    int *remainTime;

    int *interval;

    int** alea;

    int location;

    unsigned int WindowMask;

    Bool active;
    Bool *lightning;

    Bool existWindow;

    Bool isAnimated;
    Bool anotherPluginIsUsing;

} FlashScreen;


/*
 * flashSetScreenOption
 *
 */
static Bool
flashSetScreenOption (CompPlugin *p, CompScreen *s, const char *name, CompOptionValue *value)
{
    CompOption *o;
    int	        index;

    FLASH_SCREEN (s);

    o = compFindOption (fs->opt, NUM_OPTIONS (fs), name, &index);
    if (!o)
	    return FALSE;

    if (compSetOption (o, value))
	return TRUE;

    return FALSE;
}

/*
 * flashGetScreenOptions
 *
 */
static CompOption *
flashGetScreenOptions (CompPlugin *p, CompScreen *s, int *count)
{
    FLASH_SCREEN (s);
    *count = NUM_OPTIONS (fs);
    return fs->opt;
}

/*
 * flashInitiate
 *
 */
static Bool
flashInitiate (CompDisplay *d, CompAction *action, CompActionState state, CompOption *option, int nOption)
{
    CompScreen *s;
    int i;

    s = findScreenAtDisplay(d, getIntOptionNamed(option, nOption, "root", 0));
    if (s)
	{
	    FLASH_SCREEN(s);
	    FLASH_DISPLAY(s->display);
	    fs->active = !fs->active;

	    for(i=0 ; i<5 ; i++)
	    {
		fs->remainTime[i] = fd->opt[FLASH_DISPLAY_OPTION_TIME].value.i + fs->interval[i];
		fs->lightning[i] = FALSE;
	    }

	    if(!fs->active)
		damageScreen(s);
	}
    return FALSE;
}

static Bool
pointerIsInWindow (FlashScreen *fs)
{
    if(!fs->existWindow)
	return False;

    if( (fs->pointX < fs->winX + fs->winW) && (fs->pointX > fs->winX) && 
	(fs->pointY < fs->winY + fs->winH) && (fs->pointY > fs->winY) )
	return True;

    return False;
}

/*
 * getDigression : deplacer la destination
 *
 */
static int
getDigression (FlashDisplay *fd, int coord, int min, int max, int *alea)
{
        int ecart=fd->opt[FLASH_DISPLAY_OPTION_ECART_MAX].value.i;

	int interval = max - min;

	int tmp=alea[0] % interval;

	if(coord-min >= ecart)
	    min = coord - ecart;

	if(max-coord >= ecart)
	    max=coord + ecart;
	
	tmp = tmp * (max - min) / interval;
	
	return (max - tmp) - coord;
}

/*
 * getCoordTarget
 *
 */
static void
getWindowTarget (FlashDisplay *fd, int* distance, int* Tx, int* Ty, int Px, int Py, int Wx, int Wy, int Ww, int Wh, int *alea)
{
        int distanceMax=fd->opt[FLASH_DISPLAY_OPTION_DISTANCE_MAX].value.i;

	if(Px <= Wx) *Tx=Wx;
	else if(Px >= Wx+Ww) *Tx=Wx+Ww;
	else *Tx=Px;
	
	if(Py <= Wy) *Ty=Wy;
	else if(Py >= Wy+Wh) *Ty=Wy+Wh;
	else *Ty=Py;
	
	*distance = sqrt( (Px - *Tx)*(Px - *Tx) + (Py - *Ty)*(Py - *Ty) );
	
	if(*distance > 0)
		if(*Tx == Px)
			*Tx += getDigression(fd,Px,Wx,Wx+Ww, alea) * (*distance) / distanceMax + 1;
		if(*Ty == Py)
			*Ty += getDigression(fd,Py,Wy,Wy+Wh, alea) * (*distance) / distanceMax + 1;
}



/*
 * getScreenTarget
 *
 */
static void
getScreenTarget (CompScreen *s, int* distance, int* Tx, int* Ty, int Sx, int Sy, int Sw, int Sh, int *alea, int location)
{
	FLASH_SCREEN(s);
	FLASH_DISPLAY(s->display);
	
	int min=0;
	int max=0;
	int distanceMax=fd->opt[FLASH_DISPLAY_OPTION_DISTANCE_MAX].value.i;
	
	switch(location)
	{
	case POS_TOP:
		*Tx=fs->pointX;
		*Ty=Sy;
		min=Sx;
		max=Sx+Sw;
		if(!fs->existWindow)
		    break;

		if(fs->pointX < fs->winX && fs->pointY > fs->winY)
		    max=fs->winX;

		else if(fs->pointX > fs->winX + fs->winW && fs->pointY > fs->winY)
		    min=fs->winX+fs->winW;

		else if(fs->pointY >= fs->winY + fs->winH) 
		    *Ty=fs->pointY;
		break;
		
	case POS_BOTTOM:
		*Tx=fs->pointX;
		*Ty=Sy+Sh;
		min=Sx;
		max=Sx+Sw;
		if(!fs->existWindow)
		    break;
		
		if(fs->pointX < fs->winX && fs->pointY < fs->winY+fs->winH)
			max=fs->winX;

		else if(fs->pointX > fs->winX+fs->winW && fs->pointY < fs->winY+fs->winH)
			min=fs->winX+fs->winW;

		else if(fs->pointY <= fs->winY)
		    *Ty=fs->pointY;
		break;
		
	case POS_LEFT:
		*Ty=fs->pointY;
		*Tx=Sx;
		min=Sy;
		max=Sy+Sh;
		if(!fs->existWindow)
		    break;
		
		if(fs->pointY < fs->winY && fs->pointX > fs->winX)
			max=fs->winY;

		else if(fs->pointY > fs->winY+fs->winH && fs->pointX > fs->winX)
			min = fs->winY + fs->winH;

		else if(fs->pointX >= fs->winX + fs->winW)
		    *Tx=fs->pointX;
		break;
		
	case POS_RIGHT:
		*Ty=fs->pointY;
		*Tx=Sx+Sw;
		min=Sy;
		max=Sy+Sh;
		if(!fs->existWindow)
		    break;

		if(fs->pointY < fs->winY && fs->pointX < fs->winX+fs->winW)
			max=fs->winY;

		else if(fs->pointY > fs->winY+fs->winH && fs->pointX < fs->winX+fs->winW)
			min=fs->winY+fs->winH;

		else if(fs->pointX <= fs->winX)
		    *Tx=fs->pointX;
		break;
		
	default:
		break;
	}
	
	*distance = sqrt( (fs->pointX - *Tx)*(fs->pointX - *Tx) + (fs->pointY - *Ty)*(fs->pointY - *Ty) );
	
	if(*distance > 0)
		if(*Tx == fs->pointX)
		    *Tx += getDigression(fd,fs->pointX,min,max, alea) * (*distance) / distanceMax + 1;
		if(*Ty == fs->pointY)
		    *Ty += getDigression(fd,fs->pointY,min,max, alea) * (*distance) / distanceMax + 1;
}


/*
 * getWay : fonction du chemin semi-aleatoire
 *
 */
static int
getWay(int xy, int distance, int *alea)
{

	double pi = 4 * atan (1.0);
	int resultat = 0;
	int amplitude = distance * xy / (distance + xy);
	int i;
	int ms = alea[0] % 100;

	if(ms <= 50)
	    resultat += ms * amplitude * cos( xy * pi / (2*distance));
	else
	    resultat += -(ms - 50) * amplitude * cos( xy * pi / (2*distance));
	
	for(i=1;i<10;i++)
	{
	    resultat += (alea[i] % 100) * amplitude * cos( pow(2,i) * xy * pi * (alea[i] % 100) / (20 * distance) ) * (distance - xy) / (distance * 5 * (i + 1));
	}
	
	return (resultat / 100);
}


static void
getCoordinateArray (double** CoordinateArray ,int distance, int Tx, int Ty, int Px, int Py, int* tailleTab, int* alea, int numero)
{
    	int i;
	double Mx,My;
	int ecart;

	*tailleTab=distance;
		//courbe demarre au pointeur de la souris
	CoordinateArray[0][0]=0;
	CoordinateArray[0][1]=0;

	for(i=1;i<distance;i++)
	    {
		Mx=((double)i / (double)distance)*(Tx-Px);
		My=((double)i / (double)distance)*(Ty-Py);
		   			    
		ecart = getWay(i, distance,alea) + numero;

		CoordinateArray[i][0] = Mx + (double) ecart * (Ty-Py) / distance;
		CoordinateArray[i][1] = My + (double) ecart * (Tx-Px) / distance;
	    }
}

/*
 * flashPreparePaintOutput
 *
 */
static void
flashPreparePaintOutput (CompScreen *s, int msSinceLastPaint)
{
    FLASH_SCREEN (s);
    FLASH_DISPLAY(s->display);

	CompWindow *w;
	Window root, child;
	int winx, winy;
	int i,j;
	unsigned int pmask;


	if(fs->active)
	{

	    //Get informations about active window
		w = findWindowAtDisplay(s->display, s->display->activeWindow);		
		if(w)
		{

		    if(matchEval (&fs->opt[FLASH_SCREEN_OPTION_WINDOW_TYPE].value.match, w))
				fs->existWindow=TRUE;
		    else
				fs->existWindow=FALSE;
		
		    fs->winW=w->width + w->input.left + w->input.right;
		    fs->winH=w->height + w->input.top + w->input.bottom;
		    fs->winX=w->attrib.x - w->input.left;
		    fs->winY=w->attrib.y - w->input.top;
		}
		else
		{
		    fs->existWindow=FALSE;
		    fs->winW=0;
		    fs->winH=0;
		    fs->winX=0;
		    fs->winY=0;	
		    fs->isAnimated = FALSE;
		}
		

		//test if an other plugin is used at this time.
		if(otherScreenGrabExist(s, NULL))
		    fs->anotherPluginIsUsing = TRUE;
		else
		    fs->anotherPluginIsUsing = FALSE;
	
		//get position of pointer
		XQueryPointer(s->display->display, s->root, &root, &child, &(fs->pointX), &(fs->pointY), &winx, &winy, &pmask);

		int time = fd->opt[FLASH_DISPLAY_OPTION_TIME].value.i;	
	
		for(i = 0 ; i < 5 ; i++)
		{
		    fs->remainTime[i] -= msSinceLastPaint;

		    if(fs->remainTime[i] < 0)
		    {
			fs->lightning[i] = FALSE;
			fs->remainTime[i] = fs->interval[i] + time;
			
			srand(fs->alea[i][1] * (fs->pointX+1) * (fs->pointY+1) / (fs->interval[i] + 1) );
			for(j=0;j<10;j++)
			    fs->alea[i][j]=(int) rand();
		    }
		    else
		    {
			if( (fs->remainTime[i]) < time )
			    fs->lightning[i] = TRUE;
		    }
		}
	}

	UNWRAP (fs, s, preparePaintScreen);
	(*s->preparePaintScreen) (s, msSinceLastPaint);
	WRAP (fs, s, preparePaintScreen, flashPreparePaintOutput);
}

/*
 * flashDonePaintScreen
 *
 */
static void
flashDonePaintScreen (CompScreen *s)
{
    FLASH_SCREEN (s);

    if(fs->active)
		damageScreen(s);

    UNWRAP (fs, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (fs, s, donePaintScreen, flashDonePaintScreen);
}

/*
 * flashPaintOutput
 *
 */
static Bool
flashPaintOutput (CompScreen * s, const ScreenPaintAttrib * sAttrib,
				  const CompTransform    *transform,
				  Region region, CompOutput *output, unsigned int mask)
{
    Bool status;
    FLASH_SCREEN (s);
    FLASH_DISPLAY(s->display);

    UNWRAP (fs, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform ,region, output, mask);
    WRAP (fs, s, paintOutput, flashPaintOutput);

    int x1, y1;
    x1 = fs->pointX;
    y1 = fs->pointY;

    int x2, y2;
    int distanceToWindow;
    int i,j;

    if(!fs->active || pointerIsInWindow(fs) || fs->anotherPluginIsUsing)
	return status;

    glPushMatrix();  //sauve la matrice courante
    glLoadIdentity();//remplace la matrice courante par la matrice identite
    prepareXCoords(s, output, -DEFAULT_Z_CAMERA);

    glEnable(GL_BLEND);//active le melange des couleurs (glBlendFunc)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//permet les effets de transparence

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 

    glTranslatef(x1, y1, 0);

    glLineWidth(2);
	
    //lightning between pointer and active window
    if(fs->lightning[0])
	{
	    getWindowTarget(fd, &distanceToWindow, &x2, &y2, x1, y1, fs->winX, fs->winY, fs->winW, fs->winH, fs->alea[0]);

	    if(fs->existWindow && distanceToWindow < fd->opt[FLASH_DISPLAY_OPTION_DISTANCE_MAX].value.i && !fs->isAnimated)
		{
		    //changement de la frequence d'apparition en fonction de la distance
		    fs->interval[0] = (fd->opt[FLASH_DISPLAY_OPTION_INTERVAL].value.i + distanceToWindow) * (fs->alea[0][1]%400) / 200;// * 2 / fd->opt[FLASH_DISPLAY_OPTION_DISTANCE_MAX].value.i;
		
		    //affichage du trait du haut ou a gauche
		    glBegin(GL_LINE_STRIP);
		    glColor4f(
			      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_UP_LEFT].value.c[0] / 0xffff,
			      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_UP_LEFT].value.c[1] / 0xffff,
			      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_UP_LEFT].value.c[2] / 0xffff,
			      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_UP_LEFT].value.c[3] / 0xffff);
		    //remplissage du tableau de coordonnees
		    getCoordinateArray(fs->CoordinateArray, distanceToWindow, x2, y2, x1, y1, &(fs->tailleTab), fs->alea[0], -2);
		    for(i=0;i<fs->tailleTab;i++)
			glVertex2dv(fs->CoordinateArray[i]);
		    glEnd();
		
		    //affichage du trait central
		    glBegin(GL_LINE_STRIP);
		    glColor4f(
			      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_CENTER].value.c[0] / 0xffff,
			      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_CENTER].value.c[1] / 0xffff,
			      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_CENTER].value.c[2] / 0xffff,
			      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_CENTER].value.c[3] / 0xffff);
		    //remplissage du tableau de coordonnees
		    getCoordinateArray(fs->CoordinateArray, distanceToWindow, x2, y2, x1, y1, &(fs->tailleTab), fs->alea[0], 0);
		    for(i=0;i<fs->tailleTab;i++)
			glVertex2dv(fs->CoordinateArray[i]);
		    glEnd();
		
		    //affichage du trait du bas ou a droite
		    glBegin(GL_LINE_STRIP);	    
		    glColor4f(
			      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_DOWN_RIGHT].value.c[0] / 0xffff,
			      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_DOWN_RIGHT].value.c[1] / 0xffff,
			      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_DOWN_RIGHT].value.c[2] / 0xffff,
			      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_DOWN_RIGHT].value.c[3] / 0xffff);
		    //remplissage du tableau de coordonnees
		    getCoordinateArray(fs->CoordinateArray, distanceToWindow, x2, y2, x1, y1, &(fs->tailleTab), fs->alea[0], 2);
		    for(i=0;i<fs->tailleTab;i++)
			glVertex2dv(fs->CoordinateArray[i]);	
		    glEnd();
		}
	}

    for(i=1;i<5;i++)
	{
	    if(fs->lightning[i])
		{
		    getScreenTarget(s, &distanceToWindow, &x2, &y2, s->x, s->y, s->width, s->height, fs->alea[i], i-1);

		    if(distanceToWindow < fd->opt[FLASH_DISPLAY_OPTION_DISTANCE_MAX].value.i )
			{
			    //changement de la frequence d'apparition en fonction de la distance
			    fs->interval[i] = fd->opt[FLASH_DISPLAY_OPTION_INTERVAL].value.i + distanceToWindow * (fs->alea[i][1]%600) / 200; // / fd->opt[FLASH_DISPLAY_OPTION_DISTANCE_MAX].value.i;
		
			    //affichage du trait du haut ou a gauche
			    glBegin(GL_LINE_STRIP);
			    glColor4f(
				      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_UP_LEFT].value.c[0] / 0xffff,
				      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_UP_LEFT].value.c[1] / 0xffff,
				      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_UP_LEFT].value.c[2] / 0xffff,
				      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_UP_LEFT].value.c[3] / 0xffff);
			    //remplissage du tableau de coordonnees
			    getCoordinateArray(fs->CoordinateArray, distanceToWindow, x2, y2, x1, y1, &(fs->tailleTab), fs->alea[i], -2);
			    for(j=0;j<fs->tailleTab;j++)
				glVertex2dv(fs->CoordinateArray[j]);
			    glEnd();
		
			    //affichage du trait central
			    glBegin(GL_LINE_STRIP);
			    glColor4f(
				      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_CENTER].value.c[0] / 0xffff,
				      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_CENTER].value.c[1] / 0xffff,
				      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_CENTER].value.c[2] / 0xffff,
				      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_CENTER].value.c[3] / 0xffff);
			    //remplissage du tableau de coordonnees
			    getCoordinateArray(fs->CoordinateArray, distanceToWindow, x2, y2, x1, y1, &(fs->tailleTab), fs->alea[i], 0);
			    for(j=0;j<fs->tailleTab;j++)
				glVertex2dv(fs->CoordinateArray[j]);
			    glEnd();
		
			    //affichage du trait du bas ou a droite
			    glBegin(GL_LINE_STRIP);	    
			    glColor4f(
				      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_DOWN_RIGHT].value.c[0] / 0xffff,
				      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_DOWN_RIGHT].value.c[1] / 0xffff,
				      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_DOWN_RIGHT].value.c[2] / 0xffff,
				      (float)fd->opt[FLASH_DISPLAY_OPTION_COLOR_DOWN_RIGHT].value.c[3] / 0xffff);
			    //remplissage du tableau de coordonnees
			    getCoordinateArray(fs->CoordinateArray, distanceToWindow, x2, y2, x1, y1, &(fs->tailleTab), fs->alea[i], 2);
			    for(j=0;j<fs->tailleTab;j++)
				glVertex2dv(fs->CoordinateArray[j]);	
			    glEnd();
			}
		}
	}
    glTranslatef(-x1, -y1, 0);

    glColor4usv(defaultColor);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glPopMatrix();

    return status;
}


/*
 * flashSetDisplayOption
 *
 */
static Bool
flashSetDisplayOption (CompPlugin *p, CompDisplay *d, const char *name, CompOptionValue *value)
{
    CompOption *o;
    int         index;

    FLASH_DISPLAY (d);

    o = compFindOption (fd->opt, NUM_OPTIONS (fd), name, &index);
    if (!o)
        return FALSE;

    switch (index) 
    {

    case FLASH_DISPLAY_OPTION_INITIATE_BUTTON:
    case FLASH_DISPLAY_OPTION_INITIATE_KEY:
	if (setDisplayAction (d, o, value))
	    return TRUE;
	break;
    case FLASH_DISPLAY_OPTION_DISTANCE_MAX:
    case FLASH_DISPLAY_OPTION_ECART_MAX:
    case FLASH_DISPLAY_OPTION_INTERVAL:
    case FLASH_DISPLAY_OPTION_TIME:
	if (compSetIntOption(o, value))
	    return TRUE;
	break;
    case FLASH_DISPLAY_OPTION_COLOR_UP_LEFT:
    case FLASH_DISPLAY_OPTION_COLOR_CENTER:
    case FLASH_DISPLAY_OPTION_COLOR_DOWN_RIGHT:
	if (compSetColorOption(o, value))
	    return TRUE;
	break;
    default:
	break;
    }

    return FALSE;
}

 /*
 * flashGetDisplayOptions
 *
 */
static CompOption *
flashGetDisplayOptions (CompPlugin *p, CompDisplay *d, int *count)
{
    FLASH_DISPLAY (d);

    *count = NUM_OPTIONS (fd);
    return fd->opt;
}

static const CompMetadataOptionInfo flashDisplayOptionInfo[] = {
    { "initiate_button", "button", 0, flashInitiate, 0 },
    { "initiate_key", "key", 0, flashInitiate, 0 },
    { "distance_max", "int", 0, 0, 0 },
    { "ecart_max", "int", 0, 0, 0 },
    { "interval", "int", 0, 0, 0 },
    { "time", "int", 0, 0, 0 },
    { "flash_color_up_left", "color", 0, 0, 0 },
    { "flash_color_center", "color", 0, 0, 0 },
    { "flash_color_down_right", "color", 0, 0, 0 }
};

/*
 * flashInitDisplay
 *
 */
static Bool
flashInitDisplay (CompPlugin *p, CompDisplay *d)
{
    FlashDisplay *fd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;


    fd = malloc (sizeof (FlashDisplay));
    if (!fd)
        return FALSE;

    if (!compInitDisplayOptionsFromMetadata (d,
					     &flashMetadata,
					     flashDisplayOptionInfo,
					     fd->opt,
					     FLASH_DISPLAY_OPTION_NUM))
    {
	free (fd);
	return FALSE;
    }


    fd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (fd->screenPrivateIndex < 0)
    {
	compFiniDisplayOptions (d, fd->opt, FLASH_DISPLAY_OPTION_NUM);
        free (fd);
        return FALSE;
    }

    d->base.privates[displayPrivateIndex].ptr = fd;

    return TRUE;
}

/*
 * flashFiniDisplay
 *
 */
static void
flashFiniDisplay (CompPlugin *p, CompDisplay *d)
{
    FLASH_DISPLAY (d);

    freeScreenPrivateIndex (d, fd->screenPrivateIndex);

    free (fd);
}

static const CompMetadataOptionInfo flashScreenOptionInfo[] = {
    { "window_types", "match", 0, 0, 0 }
};

/*
 * flashInitScreen
 *
 */
static Bool
flashInitScreen (CompPlugin *p, CompScreen *s)
{
    FlashScreen *fs;
    int i,j;

    FLASH_DISPLAY (s->display);

    fs = malloc (sizeof (FlashScreen));
    if (!fs)
        return FALSE;

    if (!compInitScreenOptionsFromMetadata (s,
					    &flashMetadata,
					    flashScreenOptionInfo,
					    fs->opt,
					    FLASH_SCREEN_OPTION_NUM))
    {
	free (fs);
	return FALSE;
    }

    fs->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (fs->windowPrivateIndex < 0)
    {
        free (fs);
        return FALSE;
    }

    fs->active = FALSE;

    fs->lightning = (Bool *) malloc(5 * sizeof(Bool));
    fs->interval = (int *) malloc(5 * sizeof(int));
    fs->remainTime = (int *) malloc(5 * sizeof(int));

    fs->alea=(int**)malloc(5 * sizeof(int *));

    fs->CoordinateArray= (double **)malloc(OPTION_DISTANCE_MAX * sizeof(double *));

    for(i=0; i<OPTION_DISTANCE_MAX; i++)
	fs->CoordinateArray[i]=(double*)malloc(2*sizeof(double));

    for(i=0;i<5;i++)
    {
	fs->lightning[i] = FALSE;
	
	fs->alea[i]=(int*)malloc(10 * sizeof(int));
	
	fs->interval[i] = fd->opt[FLASH_DISPLAY_OPTION_INTERVAL].value.i;
	fs->remainTime[i] = fd->opt[FLASH_DISPLAY_OPTION_TIME].value.i + fs->interval[i];
	
	srand(1);
	for(j=0;j<10;j++)
	    fs->alea[i][j]=(int) rand();

    }

    addScreenAction (s, &fd->opt[FLASH_DISPLAY_OPTION_INITIATE_BUTTON].value.action);
    addScreenAction (s, &fd->opt[FLASH_DISPLAY_OPTION_INITIATE_KEY].value.action);

    WRAP (fs, s, donePaintScreen,          flashDonePaintScreen);
    WRAP (fs, s, paintOutput,              flashPaintOutput);
    WRAP (fs, s, preparePaintScreen,       flashPreparePaintOutput);

    s->base.privates[fd->screenPrivateIndex].ptr = fs;

    return TRUE;
}

/*
 * flashFiniScreen
 *
 */
static void
flashFiniScreen (CompPlugin *p, CompScreen *s)
{
    FLASH_SCREEN (s);
    FLASH_DISPLAY(s->display);

    freeWindowPrivateIndex (s, fs->windowPrivateIndex);

    UNWRAP (fs, s, donePaintScreen);
    UNWRAP (fs, s, paintOutput);
    UNWRAP (fs, s, preparePaintScreen);

    removeScreenAction(s, &fd->opt[FLASH_DISPLAY_OPTION_INITIATE_BUTTON].value.action);
    removeScreenAction(s, &fd->opt[FLASH_DISPLAY_OPTION_INITIATE_KEY].value.action);

    compFiniScreenOptions (s, fs->opt, FLASH_SCREEN_OPTION_NUM);

    free (fs);
}

/*
 * flashInit 
 *
 */
static Bool
flashInit (CompPlugin *p)
{
    if (!compInitPluginMetadataFromInfo (&flashMetadata,
					 p->vTable->name,
					 flashDisplayOptionInfo,
					 FLASH_DISPLAY_OPTION_NUM,
					 flashScreenOptionInfo,
					 FLASH_SCREEN_OPTION_NUM))
	return FALSE;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
    {
	compFiniMetadata (&flashMetadata);
	return FALSE;
    }

    compAddMetadataFromFile (&flashMetadata, p->vTable->name);

    return TRUE;
}

/*
 * flashFini
 *
 */
static void
flashFini (CompPlugin *p)
{
    if (displayPrivateIndex >= 0)
        freeDisplayPrivateIndex (displayPrivateIndex);

    compFiniMetadata (&flashMetadata);
}

static CompBool
flashInitObject (CompPlugin *p,
		     CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) flashInitDisplay,
	(InitPluginObjectProc) flashInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
flashFiniObject (CompPlugin *p,
		     CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) flashFiniDisplay,
	(FiniPluginObjectProc) flashFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static CompOption* flashGetObjectOptions (CompPlugin *plugin, 
    CompObject *object, int *count) {
    
    static GetPluginObjectOptionsProc dispTab[] = {
    (GetPluginObjectOptionsProc) 0, // GetCoreOptions
    (GetPluginObjectOptionsProc) flashGetDisplayOptions,
    (GetPluginObjectOptionsProc) flashGetScreenOptions
    };

    *count = 0;

    RETURN_DISPATCH (object, dispTab, ARRAY_SIZE (dispTab),
             (void *) count, (plugin, object, count));
}


static CompBool flashSetObjectOption (CompPlugin *plugin, CompObject *object, 
	const char *name, CompOptionValue *value) {
    
    static SetPluginObjectOptionProc dispTab[] = {
	(SetPluginObjectOptionProc) 0, /* SetCoreOption */
	(SetPluginObjectOptionProc) flashSetDisplayOption,
	(SetPluginObjectOptionProc) flashSetScreenOption
    };
    
    RETURN_DISPATCH (object, dispTab, ARRAY_SIZE (dispTab), 
	FALSE, (plugin, object, name, value));
}

static CompMetadata *
flashGetMetadata (CompPlugin *plugin)
{
    return &flashMetadata;
}

/*
 * flashVTable
 *
 */
CompPluginVTable flashVTable = {
    "flash",
    flashGetMetadata,
    flashInit,
    flashFini,
    flashInitObject,
    flashFiniObject,
    flashGetObjectOptions,
    flashSetObjectOption
};

/*
 * getCompPluginInfo
 *
 */
CompPluginVTable *
getCompPluginInfo20070830 (void)
{
    return &flashVTable;
}
