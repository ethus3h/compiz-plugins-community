/*
 * Compiz cube rubik plugin
 *
 * rubik.c
 *
 * This plugin allows the cube to transform as a Rubik's cube.
 *
 * Copyright : (C) 2008 by David Mikos
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

/* NOTE: at the moment this plugin includes some dirty hacks to
 * achieve the Rubik's cube effect. The code is somewhat messy.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>



#include "rubik-internal.h"
#include "rubik_options.h"


int rubikDisplayPrivateIndex;

int cubeDisplayPrivateIndex;



static void
initRubik (CompScreen *s)
{

    RUBIK_SCREEN (s);

    rs->tempTransform = malloc (sizeof(CompTransform));
    vStrips = rubikGetNumberStrips(s);
    hStrips = rubikGetNumberStrips(s);
	
    rs->th    = malloc (vStrips*sizeof(float));
    rs->oldTh = malloc (vStrips*sizeof(float));

    rs->psi   = malloc (hStrips*sizeof(float));
    rs->oldPsi= malloc (hStrips*sizeof(float));

    
    int i;
    
    for (i=0; i<vStrips; i++)
    { 
    	rs->th[i] = 0.0;
    	rs->oldTh[i] = rs->th[i];
    }

    for (i=0; i<hStrips; i++)
    { 
    	rs->psi[i] = 0.0;
    	rs->oldPsi[i] = rs->psi[i];
    }

    currentVStrip = NRAND(vStrips);
    currentHStrip = NRAND(hStrips);
    currentStripCounter = 0;
    currentStripDirection = 1;
    rotationAxis = NRAND(1);

    rs->w = NULL;

    initializeWorldVariables(s);

    rs->faces = malloc(6*sizeof(faceRec));
    
    for (i=0; i<6; i++)
    {
	rs->faces[i].square = malloc(hStrips*vStrips*sizeof(squareRec));
    }

    //sides
    setSpecifiedColor(rs->faces[0].color, RED);
    setSpecifiedColor(rs->faces[1].color, YELLOW);
    setSpecifiedColor(rs->faces[2].color, BLUE);
    setSpecifiedColor(rs->faces[3].color, GREEN);

    //top and bottom
    setSpecifiedColor(rs->faces[4].color, WHITE);
    setSpecifiedColor(rs->faces[5].color, ORANGE);

    initFaces(s);
}

void
initializeWorldVariables(CompScreen *s)
{
    RUBIK_SCREEN (s);
    CUBE_SCREEN (s);
    
    rs->speedFactor = rubikGetSpeedFactor(s);

    rs->hsize = s->hsize * cs->nOutput;

    rs->distance = cs->distance;
    rs->radius = cs->distance/sinf(PI*(0.5f-1.0f/rs->hsize));
}

void
initFaces (CompScreen *s)
{
    RUBIK_SCREEN(s);
    int i, j, k;

    for (k=0; k<6; k++)
    {
	for (j=0; j< vStrips; j++)
	{
	    for (i=0; i< hStrips; i++)
	    {
		int index=j*hStrips+i;

		rs->faces[k].square[index].side = k;
		rs->faces[k].square[index].x = i;//NRAND(vStrips);//i;
		rs->faces[k].square[index].y = j;//NRAND(hStrips);//j;

		rs->faces[k].square[index].psi = 0;
	    }
	}
    }
}


static void
freeRubik (CompScreen *s)
{
    RUBIK_SCREEN (s);

    if (rs->tempTransform)
	free(rs->tempTransform);

    if (rs->th)
	free(rs->th);

    if (rs->oldTh)
	free(rs->oldTh);

    if (rs->psi)
	free(rs->psi);

    if (rs->oldPsi)
	free(rs->oldPsi);

    if (rs->faces)
    {
	int i;
	for (i=0; i<6; i++)
	{
	    if (rs->faces[i].square)
		free(rs->faces[i].square);
	}

	free(rs->faces);
    }

    if (rs->w)
	free(rs->w);
}

static void
updateRubik (CompScreen *s)
{
	freeRubik (s);
	initRubik (s);
}

static void
rubikScreenOptionChange (CompScreen *s,
		CompOption            *opt,
		RubikScreenOptions num)
{
	updateRubik (s);
}


static void
rubikSpeedFactorOptionChange (CompScreen *s,
		CompOption            *opt,
		RubikScreenOptions num)
{
    	RUBIK_SCREEN (s);
	rs->speedFactor = rubikGetSpeedFactor(s);
}

static void
rubikClearTargetOutput (CompScreen *s,
		float      xRotate,
		float      vRotate)
{
	RUBIK_SCREEN (s);
	CUBE_SCREEN (s);

	UNWRAP (rs, cs, clearTargetOutput);
	(*cs->clearTargetOutput) (s, xRotate, vRotate);
	WRAP (rs, cs, clearTargetOutput, rubikClearTargetOutput);

	glClear (GL_DEPTH_BUFFER_BIT);
}


/* Cut down version of moveScreenViewport in cube.c
 * 
 * This code should be avoided in the future.
 * It's here temporarily so that windowMoveNotify, etc are not called.
 */

static void rubikMoveScreenViewport (CompScreen *, int);

static void
rubikMoveScreenViewport (CompScreen *s,
                         int	    tx)
{
    //return;
    
    CompWindow *w;
    int         wx, wy;

    tx = s->x - tx;
    tx = MOD (tx, s->hsize);
    tx -= s->x;

    if (!tx)
	return;

    s->x += tx;

    tx *= -s->width;

    for (w = s->windows; w; w = w->next)
    {
	if (windowOnAllViewports (w))
	    continue;

	getWindowMovementForOffset (w, tx, 0, &wx, &wy);

	if (w->saveMask & CWX)
	    w->saveWc.x += wx;

	if (wx)
	{
	    w->attrib.x += wx;

	    w->matrix = w->texture->matrix;
	    w->matrix.x0 -= (w->attrib.x * w->matrix.xx);
	    w->matrix.y0 -= (w->attrib.y * w->matrix.yy);

	    w->invisible = WINDOW_INVISIBLE (w);
	}
    }
}


static void
rubikPaintInside (CompScreen *s,
                  const ScreenPaintAttrib *sAttrib,
                  const CompTransform     *transform,
                  CompOutput              *output,
                  int                     size)
{
    RUBIK_SCREEN (s);
    CUBE_SCREEN (s);

    int i, j;
    
    if (rs->hsize!=s->hsize) updateRubik (s);


    ScreenPaintAttrib sA = *sAttrib;
    CompTransform mT = *transform;

    sA.yRotate += cs->invert * (360.0f / size) *
    (cs->xRotations - (s->x * cs->nOutput));

    (*s->applyScreenTransform) (s, &sA, output, &mT);

    glPushMatrix();

    glLoadMatrixf (mT.m);

    glTranslatef (cs->outputXOffset, -cs->outputYOffset, 0.0f);

    glScalef (cs->outputXScale, cs->outputYScale, 1.0f);

    Bool enabledCull = FALSE;

    glPushAttrib (GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);


    glEnable (GL_BLEND);
    
 
    
    if (glIsEnabled (GL_CULL_FACE))
    {
	enabledCull = TRUE;
    }

    int cull;

    glGetIntegerv (GL_CULL_FACE_MODE, &cull);
    glEnable (GL_CULL_FACE);

    glCullFace (~cull & (GL_FRONT | GL_BACK));

    glPushMatrix();


    glEnable (GL_NORMALIZE);
    //glEnable (GL_LIGHTING);
    //glEnable (GL_LIGHT1);
    //glEnable (GL_LIGHT0);

    if (rs->initiated) {

	Bool coloredSides = rubikGetColoredSides(s); 

	CompWindow *w;
	//CompScreen *screen;

	int screenX = s->x;

	int viewport;



	
	
	
	
	
	float xRot, vRot, progress;
	(*cs->getRotation) (s, &(xRot), &(vRot), &progress);
	
	xRot = fmodf( xRot-cs->invert * (360.0f / s->hsize) *
	              ((s->x* cs->nOutput)), 360 );
	//printf ("%f", xRot);
	//printf ("  %f\n", vRot);

	int fv[6];
	float fvd[6];

	float temp;

	fv[0]  = 0;
	fvd[0] = cos(xRot*toRadians)*cos(vRot*toRadians);
	
	//find farthest viewport (bubble sort)
	for (viewport = 1; viewport<6; viewport++) {
	    if (viewport<4)
	    {
		xRot = fmodf( xRot-cs->invert * (360.0f / s->hsize) *
		              ((-1* cs->nOutput)), 360 );
		temp = cos(xRot*toRadians)*cos(vRot*toRadians);
	    }
	    else if (viewport==4)
		temp = sin(vRot*toRadians);
	    else
		temp = -sin(vRot*toRadians);
	    
	   // printf (", %f", xRot);

	    int vp;


	    fv[viewport]  = viewport;
	    fvd[viewport] = temp;
	       
	    for (vp = 0; vp<viewport; vp++)
	    {
		if (temp<fvd[vp])
		{
		    for (i=viewport; i>vp; i--)
		    {
			float tfvd = fvd[i];
			fvd[i] = fvd[i-1];
			fvd[i-1] = tfvd;
			
			int tfv = fv[i]; 
			fv[i] = fv[i-1];
			fv[i-1] = tfv;
		    }
		    
		    break;
		}
	    }
	}
	//printf ("\n");
	//printf ("%i, %i, %i, %i\n", fv[0], fv[1], fv[2], fv[3]);
	//printf ("%f, %f, %f, %f\n\n", fvd[0], fvd[1], fvd[2], fvd[3]);

	
	
	int numWindows=1;
	
	if (!coloredSides) {
	    rubikMoveScreenViewport (s, screenX);

	    for (viewport = 0; viewport<4; viewport++) {
		rubikMoveScreenViewport (s, -1);

		int tempNumWindows = 0;
		if (!coloredSides) {
		    for (w = s->windows; w; w = w->next) {
			if (w->destroyed) continue;
			if (w->hidden) continue;
			if (w->invisible) continue;
			//if (!w->desktop && !rubikGetPaintWindowContents(s)) continue;

			tempNumWindows++;
		    }
		}
		if (tempNumWindows > numWindows) numWindows = tempNumWindows;
	    }

	    rubikMoveScreenViewport (s, -screenX);

	}

	if (!coloredSides)
	   rubikMoveScreenViewport (s, screenX);

	for (j=0; j < 6; j++) {
	    
	    i = fv[j];

		int winCounter = 0, screenCounter = 0;

	
	for (viewport = 0; viewport<4; viewport++) {
	    if (screenCounter>0 && coloredSides)
		break;

	    if (screenCounter>0 && !coloredSides)
	    {
		//rubikMoveScreenViewport (s, viewport-1);
		//rubikMoveScreenViewport (s, -viewport);
	    }

	    //if (viewport>0) continue;

	    screenCounter++;
	    winCounter = 0;



	    for (w = (cs->invert>0 ? s->windows : s->reverseWindows); w;
	    w = (cs->invert>0 ? w->next : w->prev)) {

		int attrX = 0, attrY = 0; //window x, y attributes

		if (!coloredSides) {

		    attrX = (w)->attrib.x;
		    attrY = (w)->attrib.y;


		    int tx = s->x +viewport-s->x;
		    tx = MOD (tx, s->hsize);
		    tx -= s->x;

		    int ty = 0;

		    //s->x += tx;

		    tx *= -s->width;

		    int wx, wy;

		    if (!windowOnAllViewports (w) && viewport>0)
		    {

			getWindowMovementForOffset (w, tx, ty, &wx, &wy);

			tx=wx;
			//ty=wy;

			//if (w->saveMask & CWX)
			//    w->saveWc.x += wx;

			//if (w->saveMask & CWY)
			//    w->saveWc.y += wy;


			//XOffsetRegion (w->region, dx, dy);

			attrX = (w)->attrib.x+tx;

			//w->matrix = w->texture->matrix;
			//w->matrix.x0 -= (w->attrib.x * w->matrix.xx);
			//w->matrix.y0 -= (w->attrib.y * w->matrix.yy);


#define RUBIK_WINDOW_INVISIBLE(w)			       \
    ((w)->attrib.map_state != IsViewable		    || \
	    (!(w)->damaged)					    || \
	    attrX	   + (w)->width  + (w)->output.right  <= 0  || \
	    (w)->attrib.y + (w)->height + (w)->output.bottom <= 0  || \
	    attrX	   - (w)->output.left >= (w)->screen->width || \
	    (w)->attrib.y - (w)->output.top >= (w)->screen->height)


			w->invisible = RUBIK_WINDOW_INVISIBLE (w);
		    }			    

		    if (w->destroyed) continue;
		    if (w->hidden) continue;
		    if (w->invisible) continue;
		    //if (!w->desktop && !rubikGetPaintWindowContents(s)) continue;

		    if (w->type & CompWindowTypeDesktopMask)
		    {
			glEnable(GL_COLOR_MATERIAL);
			glColor4us (0xffff, 0xffff, 0xffff, rs->desktopOpacity);

			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		    }
		    else
		    {
			glEnable(GL_COLOR_MATERIAL);
			glColor4us (0xffff, 0xffff, 0xffff, 0xffff);

			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		    }
		    enableTexture (s, w->texture, 
		                   COMP_TEXTURE_FILTER_GOOD); 

		    //glEnable (w->texture->target);	
		}
		else {
		    if (winCounter>0)
			break;
		}
		winCounter++;

		glEnable (GL_NORMALIZE);
		glDisable (GL_COLOR_MATERIAL);
		glDisable (GL_CULL_FACE);
		glEnable (GL_TEXTURE_2D);
		glEnable (GL_DEPTH_TEST);

		if (coloredSides) {
		    glEnable (GL_COLOR_MATERIAL);
		    glDisable (GL_TEXTURE_2D);
		}

		// draw texture 

		float width  = ((float) w->width) /((float) s->width);
		float height = ((float) w->height)/((float) s->height);

		if (coloredSides) {
		    width  = 1;
		    height = 1;
		}
		int texWidth = 1;//(rs->w[0].width)/vStrips;
		int texHeight = 1;//(rs->w[0].height)/vStrips;


		int hs, vs;


		    if (coloredSides)
			glColor4fv (rs->faces[i].color);


		    for (vs=0; vs<vStrips; vs++) {

			for (hs=0; hs<hStrips; hs++) {

			    int index = hs*hStrips+vs;
			    squareRec * square = &(rs->faces[i].square[index]);

			    if (coloredSides)
				glColor4fv (rs->faces[square->side].color);
			    else if (!rubikGetDesktopCaps(s) && square->side>=4)
				continue;
			    else {
				//float winX = WIN_REAL_X(w) + WIN_REAL_W(w)/2.0+((screen->x+screenCounter-1)%rs->hsize)*w->screen->width;
				//float screenW = w->screen->width;

				//printf ("%i, %i\n", ((int) (winX/screenW)), w->screen->x);
				//if (((int) (winX/screenW))!=square->side)

				if (windowOnAllViewports (w) && rubikGetDesktopCaps(s) && square->side>=4) {
				    if (viewport!=screenX)
					continue;
				}
				else if (viewport != square->side)
				    continue;
			    }

			    glPushMatrix();

			    if (cs->invert<0) {
				glRotatef (180,0,0,1);
				glRotatef (180,1,0,0);
				glScalef(-1.0f, 1.0f, 1.0f);
			    }

			    if(rotationAxis==0) {
				if (i<4) {
				    if (hs == currentHStrip)
					glRotatef (currentStripCounter*currentStripDirection,0,1,0);
				}
				else if (i==4) {
				    if (currentHStrip==0)
					glRotatef (currentStripCounter*currentStripDirection,0,1,0);
				}
				else if (i==5) {
				    if (currentHStrip==hStrips-1)
					glRotatef (currentStripCounter*currentStripDirection,0,1,0);
				}
			    }
			    else if(rotationAxis==1) {
				if (i==3) {
				    if (currentHStrip==0)
					glRotatef (-currentStripCounter*currentStripDirection,1,0,0);
				}
				else if (i==1) {
				    if (currentHStrip==hStrips-1)
					glRotatef (-currentStripCounter*currentStripDirection,1,0,0);
				}
				else if (i==2){
				    if (vStrips-1-vs == currentHStrip)
					glRotatef (-currentStripCounter*currentStripDirection,1,0,0);
				}
				else {
				    if (vs == currentHStrip)
					glRotatef (-currentStripCounter*currentStripDirection,1,0,0);
				}
			    }
			    else if(rotationAxis==2) {
				if (i==0) {
				    if (currentHStrip==0)
					glRotatef (-currentStripCounter*currentStripDirection,0,0,1);
				}
				else if (i==1){
				    if (vs == currentHStrip)
					glRotatef (-currentStripCounter*currentStripDirection,0,0,1);
				}
				else if (i==2) {
				    if (currentHStrip==hStrips-1)
					glRotatef (-currentStripCounter*currentStripDirection,0,0,1);
				}
				else if (i==3){
				    if (vStrips-1-vs == currentHStrip)
					glRotatef (-currentStripCounter*currentStripDirection,0,0,1);
				}
				else if (i==4){
				    if (hStrips-1-hs == currentHStrip)
					glRotatef (-currentStripCounter*currentStripDirection,0,0,1);
				}
				else if (i==5){
				    if (hs == currentHStrip)
					glRotatef (-currentStripCounter*currentStripDirection,0,0,1);
				}

			    }

			    if (i<4)
				glRotatef (90*i, 0, 1, 0);
			    else if (i==4)
				glRotatef (-90, 1, 0, 0);
			    else if (i==5) {
				glRotatef (90, 1, 0, 0);
				//glRotatef (180, 0, 0, 1);
			    }



			    /*if (i==0)
							glRotatef (face->side[index]*90, 0, 1, 0);
						if (i==2)
							glRotatef (face->side[index]*90, 0, 1, 0);
						if (i==1)
							glRotatef (face->side[index]*90, 0, 1, 0);
						if (i==3)
							glRotatef (face->side[index]*90, 0, 1, 0);
			     */
			    glTranslatef (0,0,0.5-0.00005*(numWindows+1-winCounter)); //allow for layers


			    glTranslatef (-0.5,0,0);
			    glTranslatef (0,0.5,0);

			    glTranslatef (((float) vs+0.5 )/((float) hStrips),0,0);
			    glTranslatef (0,-((float) hs+0.5 )/((float) vStrips),0);

			    glRotatef (90*square->psi, 0, 0, 1);

			    //glRotatef (90*NRAND(4), 0, 0, 1);
			    glTranslatef (-((float) vs+0.5 )/((float) hStrips),0,0);
			    glTranslatef (0,((float) hs+0.5 )/((float) vStrips),0);

			    if (!coloredSides) {
				glTranslatef (-((float) attrX)/((float) s->width),0,0);
				glTranslatef (0,-((float) attrY)/((float) s->height),0);
			    }


			    float h1 = ((float) square->y   )/((float) hStrips);	
			    float h2 = ((float) square->y+1 )/((float) hStrips);

			    float v1 = ((float) square->x   )/((float) vStrips);	
			    float v2 = ((float) square->x+1 )/((float) vStrips);	

			    float y = 0;
			    if (!coloredSides)
				y = ((float) attrY)/((float) s->height);

			    float x = 0;
			    if (!coloredSides)
				x = ((float) attrX)/((float) s->width);


			    float wh1 = MAX (y, h1);
			    float wh2 = MIN (y+height, h2);

			    float wv1 = MAX (x, v1);
			    float wv2 = MIN (x+width, v2);

			    if (y>h1)
				h1=y;
			    if (y+height<h2)
				h2=y+height;

			    if (x>v1)
				v1=x;
			    if (x+width<v2)
				v2=x+width;


			    if (wh1<wh2 && wv1<wv2) {

				wh1-= y;
				wh2-= y;

				h1 = wh1/height;
				h2 = wh2/height;

				wh1+=((float) hs-square->y   )/((float) hStrips);	
				wh2+=((float) hs-square->y   )/((float) hStrips);	


				v1 = (wv1-x)/width;
				v2 = (wv2-x)/width;

				wv1+= x;
				wv2+= x;



				wv1+=((float) vs-square->x   )/((float) vStrips);	
				wv2+=((float) vs-square->x   )/((float) vStrips);	

				//glColor4us (0xffff, 0xffff, 0xffff, cs->desktopOpacity);
				//glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				//glEnable (GL_COLOR_MATERIAL);
				//glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

				glBegin(GL_QUADS);

				glTexCoord2f (v1*texWidth, h1*texHeight);
				glVertex3f( wv1, -wh1,0 );	// Top Left Of The Texture and Quad

				glTexCoord2f (v2*texWidth, h1*texHeight);  
				glVertex3f( wv2, -wh1,0);	// Top Right Of The Texture and Quad    

				glTexCoord2f (v2*texWidth, h2*texHeight);  
				glVertex3f( wv2, -wh2,0);	// Bot Right Of The Texture and Quad    

				glTexCoord2f ( v1*texWidth, h2*texHeight); 
				glVertex3f( wv1,-wh2,0);

				glEnd();
			    }

			    glPopMatrix();
			}
		    }

		

		if (!coloredSides) {
		    disableTexture (s, w->texture);
		}
	    }
	}
	}
	
	if (!coloredSides)
	    rubikMoveScreenViewport (s, -screenX);
    }

    glPopMatrix();

    /*glDisable (GL_LIGHT1);
	glDisable (GL_NORMALIZE);

	if (!s->lighting)
		glDisable (GL_LIGHTING);*/

    glDisable (GL_DEPTH_TEST);

    if (enabledCull)
	glDisable (GL_CULL_FACE);

    glPopMatrix();

    
    glColor3usv (defaultColor);
    glDisable (GL_BLEND);
    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    screenTexEnvMode (s, GL_REPLACE);

    
    glPopAttrib();



    rs->damage = TRUE;

    UNWRAP (rs, cs, paintInside);
    (*cs->paintInside) (s, sAttrib, transform, output, size);
    WRAP (rs, cs, paintInside, rubikPaintInside);
}

static void
rubikPreparePaintScreen (CompScreen *s,
		int        ms)
{
    RUBIK_SCREEN (s);
    CUBE_SCREEN  (s);

    //rotationAxis=2; //TESTING
    //currentStripDirection=-1; //TESTING

    if (rs->hsize!=4) rs->initiated = FALSE;

    int i;
    if (rs->initiated) {
	i= currentHStrip;
	float increment = rs->speedFactor; 
	rs->psi[i]  += currentStripDirection*increment;
	currentStripCounter +=increment;

	if (currentStripCounter>90) {
	    rs->psi[i]=rs->oldPsi[i]+currentStripDirection*90;
	    currentStripCounter -=90;


	    int j,k;
	    if (rotationAxis==0) {
		for (k=0; k<vStrips; k++) {
		    if (currentStripDirection==1) {
			squareRec tempSquare = (rs->faces[4-1].square[i*hStrips+k]);

			for (j=4-1; j>0; j--) {  
			    rs->faces[j].square[i*hStrips+k] = rs->faces[j-1].square[i*hStrips+k];
			}

			rs->faces[0].square[i*hStrips+k] = tempSquare;


			if (i==0) {

			    if (k==0)
				rotateClockwise ((rs->faces[4].square));
			}
			if (i==hStrips-1) {
			    if (k==0)
				rotateAnticlockwise ((rs->faces[5].square));
			}

		    }
		    else {
			squareRec tempSquare = (rs->faces[0].square[i*hStrips+k]);

			for (j=0; j<4-1; j++) {  
			    rs->faces[j].square[i*hStrips+k] = rs->faces[j+1].square[i*hStrips+k];
			}

			rs->faces[4-1].square[i*hStrips+k] = tempSquare;


			if (i==0) {
			    if (k==0)
				rotateAnticlockwise ((rs->faces[4].square));
			}
			if (i==hStrips-1) {
			    if (k==0)
				rotateClockwise ((rs->faces[5].square));
			}

		    }


		}

	    }
	    else if (rotationAxis==1) {
		for (k=0; k<vStrips; k++) {
		    if (currentStripDirection==1) {

			squareRec tempSquare = (rs->faces[4].square[k*hStrips+i]);

			rs->faces[4].square[k*hStrips+i] = rs->faces[0].square[k*hStrips+i];
			rs->faces[0].square[k*hStrips+i] = rs->faces[5].square[k*hStrips+i];
			rs->faces[5].square[k*hStrips+i] = rs->faces[2].square[(vStrips-1-k)*hStrips+(vStrips-1-i)];
			rs->faces[2].square[(vStrips-1-k)*hStrips+(vStrips-1-i)] = tempSquare;

			rs->faces[5].square[k*hStrips+i].psi = (rs->faces[5].square[k*hStrips+i].psi+2)%4; 
			rs->faces[2].square[(vStrips-1-k)*hStrips+(vStrips-1-i)].psi = (rs->faces[2].square[(vStrips-1-k)*hStrips+(vStrips-1-i)].psi+2)%4;


			if (i==hStrips-1) {

			    if (k==0)
				rotateAnticlockwise ((rs->faces[1].square));
			}
			if (i==0) {
			    if (k==0)
				rotateClockwise ((rs->faces[3].square));
			}
		    }
		    else {

			squareRec tempSquare = (rs->faces[4].square[k*hStrips+i]);

			rs->faces[4].square[k*hStrips+i] = rs->faces[2].square[(vStrips-1-k)*hStrips+(vStrips-1-i)];
			rs->faces[2].square[(vStrips-1-k)*hStrips+(vStrips-1-i)] = rs->faces[5].square[k*hStrips+i];
			rs->faces[5].square[k*hStrips+i] = rs->faces[0].square[k*hStrips+i]; 
			rs->faces[0].square[k*hStrips+i] = tempSquare;

			rs->faces[2].square[(vStrips-1-k)*hStrips+(vStrips-1-i)].psi = (rs->faces[2].square[(vStrips-1-k)*hStrips+(vStrips-1-i)].psi+2)%4;
			rs->faces[4].square[k*hStrips+i].psi = (rs->faces[4].square[k*hStrips+i].psi+2)%4;



			if (i==0) {
			    if (k==0)
				rotateAnticlockwise ((rs->faces[3].square));
			}
			if (i==hStrips-1) {
			    if (k==0)
				rotateClockwise ((rs->faces[1].square));
			}
		    }

		}

	    }
	    else if (rotationAxis==2) {
		for (k=0; k<vStrips; k++) {
		    if (currentStripDirection==1) {
			squareRec tempSquare = (rs->faces[4].square[(vStrips-1-i)*hStrips+k]);

			rs->faces[4].square[(vStrips-1-i)*hStrips+k] = rs->faces[3].square[(vStrips-1-k)*hStrips+(vStrips-1-i)];
			rs->faces[3].square[(vStrips-1-k)*hStrips+(vStrips-1-i)]=rs->faces[5].square[i*hStrips+(vStrips-1-k)];
			rs->faces[5].square[i*hStrips+(vStrips-1-k)]=rs->faces[1].square[k*hStrips+i];
			rs->faces[1].square[k*hStrips+i]=tempSquare;


			rs->faces[4].square[(vStrips-1-i)*hStrips+k].psi = (rs->faces[4].square[(vStrips-1-i)*hStrips+k].psi+3)%4;
			rs->faces[3].square[(vStrips-1-k)*hStrips+(vStrips-1-i)].psi = (rs->faces[3].square[(vStrips-1-k)*hStrips+(vStrips-1-i)].psi+3)%4;
			rs->faces[5].square[i*hStrips+(vStrips-1-k)].psi = (rs->faces[5].square[i*hStrips+(vStrips-1-k)].psi+3)%4;
			rs->faces[1].square[k*hStrips+i].psi = (rs->faces[1].square[k*hStrips+i].psi+3)%4;

			if (i==0) {
			    if (k==0)
				rotateAnticlockwise ((rs->faces[0].square));
			}
			if (i==hStrips-1) {
			    if (k==0)
				rotateClockwise ((rs->faces[2].square));
			}
		    }
		    else {
			squareRec tempSquare = (rs->faces[4].square[(vStrips-1-i)*hStrips+k]);

			rs->faces[4].square[(vStrips-1-i)*hStrips+k] = rs->faces[1].square[k*hStrips+i];
			rs->faces[1].square[k*hStrips+i] = rs->faces[5].square[i*hStrips+(vStrips-1-k)]; 
			rs->faces[5].square[i*hStrips+(vStrips-1-k)] = rs->faces[3].square[(vStrips-1-k)*hStrips+(vStrips-1-i)];
			rs->faces[3].square[(vStrips-1-k)*hStrips+(vStrips-1-i)]= tempSquare;


			rs->faces[4].square[(vStrips-1-i)*hStrips+k].psi = (rs->faces[4].square[(vStrips-1-i)*hStrips+k].psi+1)%4;
			rs->faces[3].square[(vStrips-1-k)*hStrips+(vStrips-1-i)].psi = (rs->faces[3].square[(vStrips-1-k)*hStrips+(vStrips-1-i)].psi+1)%4;
			rs->faces[5].square[i*hStrips+(vStrips-1-k)].psi = (rs->faces[5].square[i*hStrips+(vStrips-1-k)].psi+1)%4;
			rs->faces[1].square[k*hStrips+i].psi = (rs->faces[1].square[k*hStrips+i].psi+1)%4;


			if (i==0) {
			    if (k==0)
				rotateClockwise ((rs->faces[0].square));
			}
			if (i==hStrips-1) {
			    if (k==0)
				rotateAnticlockwise ((rs->faces[2].square));
			}
		    }
		}
	    }

	    currentHStrip = NRAND(hStrips);
	    currentStripDirection = NRAND(2)*2-1;
	    rotationAxis = NRAND(3);
	    rs->oldPsi[i]= rs->psi[i];
	}
	rs->psi[i] = fmodf(rs->psi[i], 360);
    }

    UNWRAP (rs, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, ms);
    WRAP (rs, s, preparePaintScreen, rubikPreparePaintScreen);

    if (rs->initiated && cs->rotationState!=RotationNone) {
	cs->toOpacity = 0;
	cs->desktopOpacity = 0;
    }
}

static Bool RubikDamageWindowRect(CompWindow *w, Bool initial, BoxPtr rect){

	Bool status = TRUE;
	RUBIK_SCREEN(w->screen);
	CUBE_SCREEN (w->screen);
	//RUBIK_WINDOW(w);

	if (w->damaged || (rs->initiated && cs->rotationState!=RotationNone))
		damageScreen(w->screen);

	UNWRAP(rs, w->screen, damageWindowRect);
	status |= (*w->screen->damageWindowRect)(w, initial, rect);
	//(*w->screen->damageWindowRect)(w, initial, &rw->rect);
	WRAP(rs, w->screen, damageWindowRect, RubikDamageWindowRect);

	// true if damaged something
	return status;
}

void rubikGetRotation( CompScreen *s, float *x, float *v )
{
	//RUBIK_SCREEN (s);
    
}

static void
RubikDisableOutputClipping (CompScreen 	      *s)
{

	RUBIK_SCREEN(s);
	
	UNWRAP (rs, s, disableOutputClipping);
	(*s->disableOutputClipping) (s);
	WRAP (rs, s, disableOutputClipping, RubikDisableOutputClipping);
}

static void 
toggleRubikEffect (CompScreen *s)
{
	
	RUBIK_SCREEN(s);
	CUBE_SCREEN (s);
	
	rs->initiated = !(rs->initiated);
	
	if (rs->initiated) {
		currentVStrip = NRAND(vStrips);
		currentStripCounter =0;
		currentStripDirection = NRAND(2)*2-1;
		rotationAxis = NRAND(3);

		rs->desktopOpacity = cs->toOpacity;
		cs->toOpacity = 0;
		cs->desktopOpacity = 0;

		//cs->rotationState = RotationManual;
		//WRAP( rs, cs, getRotation, rubikGetRotation );
	}
	else {
		initFaces (s);
		cs->desktopOpacity = rs->desktopOpacity; 
		
		//cs->rotationState = RotationNone;
		
		//UNWRAP (rs, cs, getRotation);
	}
}

static Bool
rubikPrepareToggle (CompDisplay *d,
			CompAction *action,
			CompActionState state,
			CompOption *option,
			int nOption)
{
	CompScreen *s;
	Window     xid;

	xid = getIntOptionNamed (option, nOption, "root", 0);

	s = findScreenAtDisplay (d, xid);

    	if (s){
		toggleRubikEffect (s);
		return TRUE;
	}
	return FALSE;
}

static void
rubikDonePaintScreen (CompScreen * s)
{
	RUBIK_SCREEN (s);

	if (rs->damage)
	{
		damageScreen (s);
		rs->damage = FALSE;
	}

	UNWRAP (rs, s, donePaintScreen);
	(*s->donePaintScreen) (s);
	WRAP (rs, s, donePaintScreen, rubikDonePaintScreen);
}

static Bool RubikPaintOutput(CompScreen *s, const ScreenPaintAttrib *sAttrib, 
	const CompTransform *transform, Region region, CompOutput *output, unsigned int mask){

    Bool wasCulled, status;
	wasCulled = glIsEnabled(GL_CULL_FACE);

    RUBIK_SCREEN(s);

    if(rs->initiated) {
    	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
		//mask |= PAINT_SCREEN_REGION_MASK;
    }

    UNWRAP(rs, s, paintOutput);
    status = (*s->paintOutput)(s, sAttrib, transform, region, output, mask);
    WRAP(rs, s, paintOutput, RubikPaintOutput);

    return status;
}

static Bool
RubikDrawWindow (CompWindow           *w,
		const CompTransform  *transform,
		const FragmentAttrib *fragment,
		Region	             region,
		unsigned int	     mask)
{
    Bool       status;
    CompScreen *s = w->screen;
    
	FragmentAttrib fA = *fragment;

    RUBIK_SCREEN (s);
    CUBE_SCREEN (s);

    if (rs->initiated) {
    	if (cs->rotationState!=RotationNone)
    	    fA.opacity = 0;
    }
    
    if (rs->initiated) {
    	mask |= PAINT_WINDOW_TRANSFORMED_MASK;
		//mask |= PAINT_SCREEN_REGION_MASK;
    }

   	UNWRAP (rs, s, drawWindow);
	status = (*s->drawWindow) (w, transform, &fA, region, mask);
	WRAP (rs, s, drawWindow, RubikDrawWindow);

    return status;
}


static void
RubikAddWindowGeometry(CompWindow * w,
		      CompMatrix * matrix,
		      int nMatrix, Region region, Region clip)
{
    //RUBIK_WINDOW(w);
    RUBIK_SCREEN(w->screen);
    CUBE_SCREEN (w->screen);

    long i;

	UNWRAP(rs, w->screen, addWindowGeometry);
    
    if (rs->initiated) {
    	float winX = WIN_REAL_X(w) + WIN_REAL_W(w)/2.0+w->screen->x*w->screen->width;
    	float screenW = w->screen->width;
    	//printf ("\nwin: %f\n",winX);
    	//printf ("screen: %f\n",screenW);

    	rs->oldClip = malloc (clip->numRects*sizeof(BOX));
		memcpy (rs->oldClip, (clip->rects), clip->numRects*sizeof(BOX));

		
    	if (!w->desktop && !w->hidden) {

    		//for (i=0; i<region->numRects; i++) {
    		//	region->rects[i].x1 = 0;
    		//	region->rects[i].x2 = 1280;
    		//}

    		for (i=0; i<clip->numRects; i++) {
    			clip->rects[i].x1 = 1280*((int) (winX/screenW)-w->screen->x);
    			clip->rects[i].x2 = 1280*((int) (winX/screenW)+1-w->screen->x);
    		}
        	if (cs->rotationState!=RotationNone) {
        		for (i=0; i<clip->numRects; i++) {
        			clip->rects[i].x1 = 0;
        			clip->rects[i].x2 = 0;
        		}
        	}
    		
    	}
    	else if (w->desktop && cs->rotationState!=RotationNone) {
    		for (i=0; i<clip->numRects; i++) {
    			clip->rects[i].x1 = 0;
    			clip->rects[i].x2 = 0;
    		}
    	}
       	(*w->screen->addWindowGeometry) (w, matrix, nMatrix, region, clip);

		memcpy ((clip->rects), rs->oldClip, clip->numRects*sizeof(BOX));
    	
		if (rs->oldClip)
			free(rs->oldClip);

    }
    else {
    	(*w->screen->addWindowGeometry) (w, matrix, nMatrix, region, clip);
    }
    
	WRAP(rs, w->screen, addWindowGeometry, RubikAddWindowGeometry);
}

static void
RubikPaintTransformedOutput (CompScreen              *s,
			    const ScreenPaintAttrib *sAttrib,
			    const CompTransform     *transform,
			    Region                  region,
			    CompOutput              *output,
			    unsigned int            mask)
{
    RUBIK_SCREEN (s);

    UNWRAP (rs, s, paintTransformedOutput);

    (*s->paintTransformedOutput) (s, sAttrib, transform, region,
				      output, mask);
    
    WRAP (rs, s, paintTransformedOutput, RubikPaintTransformedOutput);
}

static Bool RubikPaintWindow(CompWindow *w, const WindowPaintAttrib *attrib, 
	const CompTransform *transform, Region region, unsigned int mask)
{
    RUBIK_SCREEN(w->screen);

    UNWRAP(rs, w->screen, paintWindow);
    Bool status = (*w->screen->paintWindow)(w, attrib, transform, region, mask);
    WRAP(rs, w->screen, paintWindow, RubikPaintWindow);

    return status;
}



static Bool
rubikInitDisplay (CompPlugin  *p,
		CompDisplay *d)
{
	RubikDisplay *rd;

	if (!checkPluginABI ("core", CORE_ABIVERSION) ||
			!checkPluginABI ("cube", CUBE_ABIVERSION))
		return FALSE;

	if (!getPluginDisplayIndex (d, "cube", &cubeDisplayPrivateIndex))
		return FALSE;

	rd = malloc (sizeof (RubikDisplay));

	if (!rd)
		return FALSE;

	rd->screenPrivateIndex = allocateScreenPrivateIndex (d);

	if (rd->screenPrivateIndex < 0)
	{
		free (rd);
		return FALSE;
	}

	rubikSetToggleKeyInitiate    (d, rubikPrepareToggle);
	rubikSetToggleButtonInitiate (d, rubikPrepareToggle);
	
	d->base.privates[rubikDisplayPrivateIndex].ptr = rd;

	return TRUE;
}

static void
rubikFiniDisplay (CompPlugin  *p,
		CompDisplay *d)
{
	RUBIK_DISPLAY (d);

	freeScreenPrivateIndex (d, rd->screenPrivateIndex);
	free (rd);
}

static Bool
rubikInitScreen (CompPlugin *p,
		CompScreen *s)
{
	static const float ambient[]  = { 0.3, 0.3, 0.3, 1.0 };
	static const float diffuse[]  = { 1.0, 1.0, 1.0, 1.0 };
	static const float position[] = { 0.0, 1.0, 0.0, 0.0 };

	RubikScreen *rs;

	RUBIK_DISPLAY (s->display);
	CUBE_SCREEN (s);

	rs = malloc (sizeof (RubikScreen) );

	if (!rs)
		return FALSE;

    if( (rs->windowPrivateIndex = allocateWindowPrivateIndex(s)) < 0){
    	free(rs);
    	return FALSE;
    }

    s->base.privates[rd->screenPrivateIndex].ptr = rs;

    rs->damage = FALSE;

    glLightfv (GL_LIGHT1, GL_AMBIENT, ambient);
    glLightfv (GL_LIGHT1, GL_DIFFUSE, diffuse);
    glLightfv (GL_LIGHT1, GL_POSITION, position);

    initRubik (s);


    rubikSetSpeedFactorNotify (s, rubikSpeedFactorOptionChange);
    rubikSetNumberStripsNotify (s, rubikScreenOptionChange);

    rs->initiated = FALSE;


    WRAP (rs, s, donePaintScreen, rubikDonePaintScreen);
    WRAP (rs, s, preparePaintScreen, rubikPreparePaintScreen);
    WRAP (rs, cs, clearTargetOutput, rubikClearTargetOutput);
    WRAP (rs, cs, paintInside, rubikPaintInside);

    //WRAP (rs, s, paintWindow, RubikPaintWindow);
    WRAP (rs, s, paintOutput, RubikPaintOutput);
    WRAP (rs, s, drawWindow, RubikDrawWindow);
    WRAP (rs, s, paintTransformedOutput, RubikPaintTransformedOutput);
    WRAP (rs, s, damageWindowRect, RubikDamageWindowRect);
    WRAP (rs, s, addWindowGeometry, RubikAddWindowGeometry);

    //WRAP (rs, s, drawWindowTexture, RubikDrawWindowTexture);

    //WRAP (rs, s, disableOutputClipping, RubikDisableOutputClipping);

    return TRUE;
}

static void
rubikFiniScreen (CompPlugin *p,
		CompScreen *s)
{
    RUBIK_SCREEN (s);
    CUBE_SCREEN (s);

    freeRubik(s);

    UNWRAP (rs, s, donePaintScreen);
    UNWRAP (rs, s, preparePaintScreen);
    UNWRAP (rs, cs, clearTargetOutput);
    UNWRAP (rs, cs, paintInside);

    //UNWRAP (rs, s, paintWindow);
    UNWRAP (rs, s, paintOutput);
    UNWRAP (rs, s, drawWindow);
    UNWRAP (rs, s, paintTransformedOutput);
    UNWRAP (rs, s, damageWindowRect);
    UNWRAP (rs, s, addWindowGeometry);

    //UNWRAP (rs, s, drawWindowTexture);

    //UNWRAP (rs, s, disableOutputClipping);

    free(rs);
}

static Bool
rubikInitWindow(CompPlugin *p, CompWindow *w){
    RubikWindow *rw;
    RUBIK_SCREEN(w->screen);

    rw = (RubikWindow*) malloc( sizeof(RubikWindow) );
    
    if (!rw)
    	return FALSE;

    w->base.privates[rs->windowPrivateIndex].ptr = rw;

    //WRAP (rw, w, drawWindowGeometry, RubikDrawWindowGeometry);
    
    return TRUE;
}

static void
rubikFiniWindow(CompPlugin *p, CompWindow *w){

    RUBIK_WINDOW(w);
    //RUBIK_DISPLAY(w->screen->display);

   // UNWRAP (rw, w, drawWindowGeometry);
    
    if (rw)
    	free(rw);
}
static Bool
rubikInit (CompPlugin * p)
{
	rubikDisplayPrivateIndex = allocateDisplayPrivateIndex();

	if (rubikDisplayPrivateIndex < 0)
		return FALSE;

	return TRUE;
}

static void
rubikFini (CompPlugin * p)
{
	if (rubikDisplayPrivateIndex >= 0)
		freeDisplayPrivateIndex (rubikDisplayPrivateIndex);
}

static CompBool
rubikInitObject (CompPlugin *p,
		CompObject *o)
{
	static InitPluginObjectProc dispTab[] = {
			(InitPluginObjectProc) 0, /* InitCore */
			(InitPluginObjectProc) rubikInitDisplay,
			(InitPluginObjectProc) rubikInitScreen,
			(InitPluginObjectProc) rubikInitWindow
	};

	RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
rubikFiniObject (CompPlugin *p,
		CompObject *o)
{
	static FiniPluginObjectProc dispTab[] = {
			(FiniPluginObjectProc) 0, /* FiniCore */
			(FiniPluginObjectProc) rubikFiniDisplay,
			(FiniPluginObjectProc) rubikFiniScreen,
			(FiniPluginObjectProc) rubikFiniWindow,
	};

	DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable rubikVTable = {

		"rubik",
		0,
		rubikInit,
		rubikFini,
		rubikInitObject,
		rubikFiniObject,
		0,
		0
};


CompPluginVTable *
getCompPluginInfo (void)
{
	return &rubikVTable;
}
