/*
 * Compiz cube rubik plugin
 *
 * rubik.c
 *
 * This plugin allows the cube to transform as a Rubik's cube.
 *
 * Written in 2008 by David Mikos
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

/*
 * Loosely based on freewins, atlantis, and anaglyph.
 */

#include <string.h>
#include <math.h>



#include "rubik-internal.h"
#include "rubik_options.h"


int rubikDisplayPrivateIndex;

int cubeDisplayPrivateIndex;



static void
initRubik (CompScreen *s)
{

	RUBIK_SCREEN (s);
	CUBE_SCREEN (s);

	rs->tempTransform = malloc (sizeof(CompTransform));
	vSize = rubikGetVerticalSize(s);
	
    rs->th    = malloc (vSize*sizeof(float));
    rs->oldTh = malloc (vSize*sizeof(float));

    int i;
    for (i=0; i<vSize; i++) { 
    	rs->th[i] = 0.0;
    	rs->oldTh[i] = rs->th[i];
    }

	currentVStrip = NRAND(vSize);
	currentStripCounter = 0;
	currentStripDirection = 1;
	
	rs->w = NULL;
	rs->numDesktopWindows = 0;
	speedFactor = rubikGetSpeedFactor(s);

	initializeWorldVariables(s->hsize, cs->distance);
	
	rs->faces = malloc (4*sizeof(faceRec));
	
	setSpecifiedColor (rs->faces[0].color, RED); 
	setSpecifiedColor (rs->faces[1].color, YELLOW); 
	setSpecifiedColor (rs->faces[2].color, BLUE); 
	setSpecifiedColor (rs->faces[3].color, GREEN); 
}

void initializeWorldVariables(int hs, float perpDistCentreToWall) { //precalculate some values

	hSize = hs;
	q = (float) hSize;

	distance = perpDistCentreToWall;
	radius = perpDistCentreToWall/sinf(PI*(0.5-1/q));
}

static void
freeRubik (CompScreen *s)
{
	RUBIK_SCREEN (s);
	
	if (rs->tempTransform)
		free (rs->tempTransform);
	
	if (rs->th)
		free (rs->th);
	
	if (rs->oldTh)
		free (rs->oldTh);

	if (rs->faces)
		free (rs->faces);
	
	if (rs->w)
		free (rs->w);
	
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
	speedFactor = rubikGetSpeedFactor(s);
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


static void rubikPaintInside (CompScreen *s,
		const ScreenPaintAttrib *sAttrib,
		const CompTransform     *transform,
		CompOutput              *output,
		int                     size)
{
	RUBIK_SCREEN (s);
	CUBE_SCREEN (s);

	if (hSize!=s->hsize) updateRubik (s);

	/*static const float mat_shininess[]      = { 60.0 };
	static const float mat_specular[]       = { 0.8, 0.8, 0.8, 1.0 };
	static const float mat_diffuse[]        = { 0.46, 0.66, 0.795, 1.0 };
	static const float mat_ambient[]        = { 0.1, 0.1, 0.3, 1.0 };
	static const float lmodel_ambient[]     = { 1.0, 1.0, 1.0, 1.0 };
	static const float lmodel_localviewer[] = { 0.0 };*/
	
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

	/*glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
	glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
	glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
	glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
	glLightModelfv (GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glLightModelfv (GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_localviewer);
*/
	glEnable (GL_NORMALIZE);
	//glEnable (GL_LIGHTING);
	//glEnable (GL_LIGHT1);
	//glEnable (GL_LIGHT0);
	
	if (rs->w!=NULL && rs->initiated && rubikGetRotateDesktop(s)) {
	
	int k;
	for (k=0; k< rs->numDesktopWindows; k++) {
		
	enableTexture (s, (*rs->w[k]).texture, 
	COMP_TEXTURE_FILTER_GOOD); 
	
	glEnable ((*rs->w[k]).texture->target);	
	
	glEnable (GL_NORMALIZE);
	glDisable (GL_COLOR_MATERIAL);
	glDisable (GL_CULL_FACE);
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_DEPTH_TEST);
	
	if (rubikGetColoredSides(s)) {
		glEnable (GL_COLOR_MATERIAL);
		glDisable (GL_TEXTURE_2D);
	}
	//glColor4f (0.4, 0.3, 0.0, 1.0);

			// draw texture 

	float width  = ((float) (*rs->w[k]).width)/1280;
	float height = ((float) (*rs->w[k]).height)/1024;
	
	int texWidth = 1;//(rs->w[0].width)/vSize;
	int texHeight = 1;//(rs->w[0].height)/vSize;

	int i, j;
	for (i=0; i< 4; i++) {
		
		if (rubikGetColoredSides(s)) {
			glColor4fv (rs->faces[i].color);
		}
		for (j=0; j<vSize; j++) {

		glPushMatrix();
		
		glRotatef (90*i, 0, 1, 0);
		
		if (i==0)
			glRotatef (rs->th[j], 1, 0, 0);
		if (i==2)
			glRotatef (-rs->th[vSize-j-1], 1, 0, 0);
		if (i==1)
			glRotatef (rs->th[vSize-1], 0, 0, 1);
		if (i==3)
			glRotatef (-rs->th[0], 0, 0, 1);
		
		glTranslatef (0,0,0.5-0.00001);

		glTranslatef (-((float) (*rs->w[k]).attrib.x)/((float) s->width)-0.5,0,0);

		glTranslatef (0,-((float) (*rs->w[k]).attrib.y)/((float) s->height)+0.5,0);
		
		float s1 = ((float) j   )/((float) vSize);	
		float s2 = ((float) j+1 )/((float) vSize);	
		
		glBegin(GL_QUADS); 

			glNormal3f (1,1,0);
			glTexCoord2f (s1*texWidth, 0);
	 	 	glVertex3f( s1*width, 0,0 );	// Top Left Of The Texture and Quad
  
			//glNormal3f (0,0,1);
	 	 	glTexCoord2f (s2*texWidth, 0);  
			glVertex3f( s2*width, 0,0);	// Top Right Of The Texture and Quad    
				
			//glNormal3f (0,0,1);
			glTexCoord2f (s2*texWidth, texHeight);  
			glVertex3f( s2*width, -height,0);	// Bot Right Of The Texture and Quad    
								
			//glNormal3f (0,0,1);
			glTexCoord2f ( s1*texWidth, texHeight); 
			glVertex3f( s1*width,-height,0);

		glEnd();
		
		glPopMatrix();
		}
	}

	disableTexture (s, (*rs->w[k]).texture);
	}	
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

	UNWRAP (rs, s, preparePaintScreen);
	(*s->preparePaintScreen) (s, ms);
	WRAP (rs, s, preparePaintScreen, rubikPreparePaintScreen);
}

static Bool RubikDamageWindowRect(CompWindow *w, Bool initial, BoxPtr rect){

    Bool status = TRUE;
    RUBIK_SCREEN(w->screen);
    //RUBIK_WINDOW(w);

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
toggleRubikEffect (CompScreen *s)
{
	
	RUBIK_SCREEN(s);
	//CUBE_SCREEN (s);
	
	rs->initiated = !(rs->initiated);
	
	if (rs->initiated) {
		currentVStrip = NRAND(vSize);
		currentStripCounter =0;
		currentStripDirection = NRAND(2)*2-1;
		//cs->rotationState = RotationManual;
		//WRAP( rs, cs, getRotation, rubikGetRotation );
	}
	else {
		//if (rs-w)
		//	free (rs->w)
		//rs->w = NULL;
		
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

static void
RubikDrawWindowTexture(CompWindow * w, CompTexture * texture,
		      const FragmentAttrib *attrib,
		      unsigned int mask)
{
    //RUBIK_WINDOW(w);
    RUBIK_SCREEN(w->screen);

    if (texture!=NULL && rs->initiated && w->desktop && !w->hidden) {
    	
    	Bool newWindow = TRUE;
    	int i;
    	for (i=0; i<rs->numDesktopWindows; i++) {
    		if (w->texture->name==(*rs->w[i]).texture->name)
    			newWindow = FALSE;
    	}
    	if (newWindow) {
    		rs->numDesktopWindows++;
    		if (rs->w==NULL) {
    			rs->w = malloc (sizeof(CompWindow*));
        		rs->w[0] = w;
    		}
    		else {
    			rs->w = realloc (rs->w, rs->numDesktopWindows*sizeof (CompWindow*));
        		rs->w[rs->numDesktopWindows-1] = w;
    		}
    	}
    	//printf ("%i\n",w->texture->name);
    }

    UNWRAP(rs, w->screen, drawWindowTexture);
    (*w->screen->drawWindowTexture) (w, texture, attrib, mask);
    WRAP(rs, w->screen, drawWindowTexture, RubikDrawWindowTexture);
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
    	if (w->desktop && cs->rotationState!= RotationNone && rubikGetRotateDesktop(s))
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
	const CompTransform *transform, Region region, unsigned int mask){

	CompTransform wTransform = *transform;

	Bool wasCulled = glIsEnabled(GL_CULL_FACE);
	Bool status;

	RUBIK_SCREEN(w->screen);
	RUBIK_WINDOW(w);

	mask |= PAINT_WINDOW_TRANSFORMED_MASK;
	//mask |= PAINT_SCREEN_REGION_MASK;

    int i;

	UNWRAP(rs, w->screen, paintWindow);

	if (rs->initiated) {

		if(wasCulled)
			glDisable(GL_CULL_FACE);

		float winX = WIN_REAL_X(w) + WIN_REAL_W(w)/2.0+w->screen->x*w->screen->width;
		float screenW = w->screen->width;
		//printf ("\nwin: %f\n",winX);
		//printf ("screen: %f\n",screenW);

		if (!w->desktop && !w->hidden) {
			
			glDisable (GL_CLIP_PLANE0);
			glDisable (GL_CLIP_PLANE1);
			glDisable (GL_CLIP_PLANE2);
			glDisable (GL_CLIP_PLANE3);

			GLdouble oldClipPlane4[4];
			GLdouble oldClipPlane5[4];
			glGetClipPlane(GL_CLIP_PLANE4, oldClipPlane4);
			glGetClipPlane(GL_CLIP_PLANE5, oldClipPlane5);
			
			GLboolean oldClipPlane4status = glIsEnabled(GL_CLIP_PLANE4);
			GLboolean oldClipPlane5status = glIsEnabled(GL_CLIP_PLANE5);
			
			
			GLdouble clipPlane4[] = { 1.0,  0, 0, 0 };
			GLdouble clipPlane5[] = { -1, 0.0, 0, w->screen->width };

			glClipPlane (GL_CLIP_PLANE4, clipPlane4);
			glClipPlane (GL_CLIP_PLANE5, clipPlane5);
			glEnable (GL_CLIP_PLANE4);
			glEnable (GL_CLIP_PLANE5);

		if (((int) (winX/screenW))%2==0) {

			memcpy (rs->tempTransform, &wTransform, sizeof (CompTransform));

			for (i=0; i<vSize; i++) {

				if (i!=0) memcpy (&wTransform, rs->tempTransform, sizeof (CompTransform));

				clipPlane4[3] = -i*w->screen->width/vSize;
				clipPlane5[3] = (i+1)*w->screen->width/vSize;
				glClipPlane (GL_CLIP_PLANE4, clipPlane4);
				glClipPlane (GL_CLIP_PLANE5, clipPlane5);
				glEnable (GL_CLIP_PLANE4);
				glEnable (GL_CLIP_PLANE5);


				matrixScale (&wTransform, 1.0f, 1.0f, 1.0f/ w->screen->width);

				matrixTranslate(&wTransform, 
						(WIN_REAL_X(w) + WIN_REAL_W(w)/2.0), 
						(WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0),
						-distance*w->screen->width);

				rw->psi = 0;
				if (i==currentVStrip) {
					float increment = 0.1*speedFactor; 
					rs->th[i]  += currentStripDirection*increment;
					currentStripCounter +=increment;

					if (currentStripCounter>90) {
						rs->th[i]=rs->oldTh[i]+currentStripDirection*90;
						currentStripCounter -=90;
						currentVStrip = NRAND(vSize);
						currentStripDirection = NRAND(2)*2-1;
						rs->oldTh[i]= rs->th[i];

					}
				}


				rs->th[i] = fmodf(rs->th[i], 360);

				matrixRotate(&wTransform, rw->psi, 0.0, 0.0, 1.0);
				if ((int) (winX/screenW)==0)
					matrixRotate(&wTransform, -rs->th[i], 1.0, 0.0, 0.0);
				else
					matrixRotate(&wTransform, rs->th[i], 1.0, 0.0, 0.0);

				matrixTranslate(&wTransform, 
						-(WIN_REAL_X(w) + WIN_REAL_W(w)/2.0), 
						-(WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0),
						distance*w->screen->width);
				
				status = (*w->screen->paintWindow)(w, attrib, &wTransform, region, mask);
			}

		}
		else {
				matrixScale (&wTransform, 1.0f, 1.0f, 1.0f/ w->screen->width);

				matrixTranslate(&wTransform, 
						(WIN_REAL_X(w) + WIN_REAL_W(w)/2.0), 
						(WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0),
						0);

				if ((int) (winX/screenW)==1)
					matrixRotate(&wTransform, -rs->th[vSize-1], 0.0, 0.0, 1);
				else
					matrixRotate(&wTransform, rs->th[0], 0.0, 0.0, 1);

				matrixTranslate(&wTransform, 
						-(WIN_REAL_X(w) + WIN_REAL_W(w)/2.0), 
						-(WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0),
						0);
			
			status = (*w->screen->paintWindow)(w, attrib, &wTransform, region, mask);
			
		}
		glDisable (GL_CLIP_PLANE4);
		glDisable (GL_CLIP_PLANE5);

		glClipPlane(GL_CLIP_PLANE4, oldClipPlane4);
		glClipPlane(GL_CLIP_PLANE5, oldClipPlane5);
		
		if (oldClipPlane4status)
			glEnable(GL_CLIP_PLANE4);
		
		if (oldClipPlane5status)
			glEnable(GL_CLIP_PLANE5);
		
		}
		else {
			status = (*w->screen->paintWindow)(w, attrib, &wTransform, region, mask);
		}
	}
	else {
		rw->z = distance;
		rw->psi = 0;
		
		int i;
		for (i=0; i<vSize; i++) {
			rs->th[i]  = 0;
			rs->oldTh[i] = rs->th[i];
		}

		

		glEnable (GL_CLIP_PLANE0);
		glEnable (GL_CLIP_PLANE1);
		glEnable (GL_CLIP_PLANE2);
		glEnable (GL_CLIP_PLANE3);
		
		if(wasCulled)
			glDisable(GL_CULL_FACE);

		status = (*w->screen->paintWindow)(w, attrib, &wTransform, region, mask);
	}

	WRAP(rs, w->screen, paintWindow, RubikPaintWindow);


	if(wasCulled)
		glEnable(GL_CULL_FACE);


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
	rubikSetVerticalSizeNotify (s, rubikScreenOptionChange);

    rs->initiated = FALSE;
    
	
	WRAP (rs, s, donePaintScreen, rubikDonePaintScreen);
	WRAP (rs, s, preparePaintScreen, rubikPreparePaintScreen);
	WRAP (rs, cs, clearTargetOutput, rubikClearTargetOutput);
	WRAP (rs, cs, paintInside, rubikPaintInside);
	
    WRAP (rs, s, paintWindow, RubikPaintWindow);
    WRAP (rs, s, drawWindow, RubikDrawWindow);
    WRAP (rs, s, paintTransformedOutput, RubikPaintTransformedOutput);
    WRAP (rs, s, damageWindowRect, RubikDamageWindowRect);
    WRAP (rs, s, addWindowGeometry, RubikAddWindowGeometry);

    WRAP (rs, s, drawWindowTexture, RubikDrawWindowTexture);

    //WRAP (rs, s, disableOutputClipping, RubikDisableOutputClipping);

	return TRUE;
}

static void
rubikFiniScreen (CompPlugin *p,
		CompScreen *s)
{
	RUBIK_SCREEN (s);
	CUBE_SCREEN (s);

	freeRubik (s);

	UNWRAP (rs, s, donePaintScreen);
	UNWRAP (rs, s, preparePaintScreen);
	UNWRAP (rs, cs, clearTargetOutput);
	UNWRAP (rs, cs, paintInside);

    UNWRAP (rs, s, paintWindow);
    UNWRAP (rs, s, paintOutput);
    UNWRAP (rs, s, drawWindow);
    UNWRAP (rs, s, paintTransformedOutput);
    UNWRAP (rs, s, damageWindowRect);
    UNWRAP (rs, s, addWindowGeometry);

    UNWRAP (rs, s, drawWindowTexture);

    //UNWRAP (rs, s, disableOutputClipping);
    
	free (rs);
}

static Bool
rubikInitWindow(CompPlugin *p, CompWindow *w){
    RubikWindow *rw;
    RUBIK_SCREEN(w->screen);

    rw = (RubikWindow*) malloc( sizeof(RubikWindow) );
    
    if (!rw)
    	return FALSE;

    rw->psi = 0.0;
    
    rw->rotated = FALSE;
    
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
