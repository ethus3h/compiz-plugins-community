/*
 * Compiz cube maze plugin
 *
 * maze.c
 *
 * This code is copied from the gears plugin and attempts to provide a bit of
 * light relief by providing a maze to move a ball around inside a transparent
 * cude ;-)
 *
 * Maze Copyright    : (C) 2007 by Derek Freeman-Jones
 *
 * gears.c Copyright : (C) 2007 by Dennis Kasprzyk
 * E-mail            : onestone@opencompositing.org
 *
 * Copyright (c) 1999-2000 Pawel W. Olszta. All Rights Reserved.
 * Written by Pawel W. Olszta, <olszta@sourceforge.net>
 * Creation date: Fri Dec 3 1999   - fgElapsedTime
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
 * Based on glxgears.c:
 *    http://cvsweb.xfree86.org/cvsweb/xc/programs/glxgears/glxgears.c
 */

#include <string.h>
#include <math.h>
#include <GL/glu.h>

#include <compiz-core.h>
#include <compiz-cube.h>

#include "maze_options.h"

static int displayPrivateIndex;

static int cubeDisplayPrivateIndex;


typedef struct _Wall
{
  GLfloat min_x;
  GLfloat min_y;
  GLfloat min_z;
  GLfloat max_x;
  GLfloat max_y;
  GLfloat max_z;

  int brick;
} Wall;


typedef struct _MazeDisplay
{
    int screenPrivateIndex;

}
MazeDisplay;

typedef struct _MazeScreen
{
    DonePaintScreenProc    donePaintScreen;
    PreparePaintScreenProc preparePaintScreen;

    CubeClearTargetOutputProc clearTargetOutput;
    CubePaintInsideProc       paintInside;

    Bool damage;

    float contentRotation;

    GLuint maze;
    GLuint mazeBall;

    GLuint currentWallCount;
    Wall *wall;

    GLfloat position[3];        // Position of the ball
    GLfloat velocity[3];
    GLfloat acceleration[3];

    GLfloat last_mx;            /* Last X movement */
    GLfloat last_mz;            /* Last Z movement */

    int lc;                     // Last wall collied with
    int first;                  // First time through or a restart
    double t0;                  // Last time called
    double last_q;              // Last quotient

    GLfloat maxSpeed;             // Max velocity the ball can get to

    GLfloat yRotation;          // How we were first called i.e. which face were we looking at
  
    GLfloat last_q_yrot;        // Last quotient for y rotation

} MazeScreen;

char g_mazeString[257] = "\
****************\
*  *     **  *  \
* *  ***    *  *\
*  *   **** * **\
**  **   **   **\
* * * **  **** *\
*      **   *  *\
** ***  ** * * *\
*     *  *     *\
* *** * * **** *\
** *  *  *     *\
   * ***  * ****\
** *  *  *     *\
*    *  * ** * *\
** *  *      * *\
****************";

// Useful for ball movement debugging the extra square is 0,0
char gbm_mazeString[257] = "\
****************\
**             *\
*              *\
*              *\
*              *\
*              *\
*              *\
*              *\
*              *\
*              *\
*              *\
*              *\
*              *\
*              *\
*              *\
****************";
/* Useful for collision detection debugging */
char gcd_mazeString[257] = "\
****************\
**     **     **\
*      *       *\
*     *        *\
*              *\
*              *\
*              *\
*   *      *   *\
****        ****\
**            **\
*              *\
*      *       *\
*       *      *\
*       *      *\
**     **     **\
****************";


GLint g_mazeWidth = 16;
GLint g_mazeDepth = 16;
GLint g_mazeDiameter = 32.0f;
GLint g_mazeHeight = -15.0f;     

GLfloat Xmin = 0.0f, Xmax = 30.25f;
GLfloat Zmin = 0.0f, Zmax = 30.25f;
GLfloat Zstep = 180.0, Ystep = 180;

#define GET_MAZE_DISPLAY(d) \
    ((MazeDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define MAZE_DISPLAY(d) \
    MazeDisplay *gd = GET_MAZE_DISPLAY(d);

#define GET_MAZE_SCREEN(s, gd) \
    ((MazeScreen *) (s)->base.privates[(gd)->screenPrivateIndex].ptr)
#define MAZE_SCREEN(s) \
    MazeScreen *ms = GET_MAZE_SCREEN(s, GET_MAZE_DISPLAY(s->display))

#define COS(X)   cos( (X) * 3.14159/180.0 )
#define SIN(X)   sin( (X) * 3.14159/180.0 )

#define toRadians 0.01745329251994329577

/* Possible plugin options */
//const float MaxSpeed = 32.0f;
//const float MinSpeed = -32.0f;

//
// So everything is now in the positive plane, I see the world as X tracking the mouse going left to right
// and Z tracking the mouse movement up and down with Y being gravity. BTW: Gravity to be applied later.
// Beach ball effect currently on, rubber ball, sponge ball, ball bearing all to come later.
//
//     Z
//     0            
//  X 0--------------------------------  
//     |
//     |
//     |
//     |
//     |
//     |
//     |
//     |
//     |    
//     |    
//     |
//     |
//     |
//     |
//     |
//     |
//     |
//     |
//     |

static double
check_wall_x ( GLfloat bx, GLfloat bz, GLfloat dbx, GLfloat dbz, GLfloat min_wx, GLfloat min_wz, GLfloat max_wx, GLfloat max_wz, double dt )
{
  //
  // Local variables
  //
  GLfloat dtx = dt;   // Z time step


  // Have we hit the right hand wall 
  if ( (dbx <= min_wx && dbx >= max_wx) &&
       (bz <= min_wz && bz >= max_wz) )
    {
      if ( bx != dbx )
	{
	  dtx = ((bx-min_wx)*dt) / (bx-dbx);
	  
	  if ( dtx > dt ) dtx = dt;
	}
    }
  else /* Have we hit the left hand wall */
  if ( (dbx >= min_wx && dbx <= max_wx) &&
       (bz >= min_wz && bz <= max_wz) )
    {
      if ( bx != dbx )
	{
	  dtx = ((bx-min_wx)*dt) / (bx-dbx);
	  
	  if ( dtx > dt ) dtx = dt;
	}
    }
  
  return dtx;
}

static double
check_wall_z ( GLfloat bx, GLfloat bz, GLfloat dbx, GLfloat dbz, GLfloat min_wx, GLfloat min_wz, GLfloat max_wx, GLfloat max_wz, double dt )
{
  GLfloat dtz = dt;   // Z time step


  /* Have we hit the front wall */
  if ( (dbz <= min_wz && dbz >= max_wz) &&
       (bx >= min_wx && bx <= max_wx) )
    {
      if ( bz != dbz )
	{
	  dtz = ((bz-min_wz)*dt) / (bz-dbz);
	  
	  if ( dtz > dt ) dtz = dt;
	}
    }
  else  /* Have we hit the back wall */
  if ( (dbz >= min_wz && dbz <= max_wz) &&
       (bx <= min_wx && bx >= max_wx) )
    {
      if ( bz != dbz )
	{
	  dtz = ((bz-min_wz)*dt) / (bz-dbz);
	  
	  if ( dtz > dt ) dtz = dt;
	}
    }

  return dtz;
}

/*
 * Can't use glutGet(ELAPSED_TIME)
 * so pince the code ;-) I love open source
 */
static long
fgElapsedTime( void )
{
  static int first = 0;
  static struct timeval start;

    if ( first )
    {
        struct timeval now;
        long elapsed;

        gettimeofday( &now, NULL );

        elapsed = (now.tv_usec - start.tv_usec) / 1000;
        elapsed += (now.tv_sec - start.tv_sec) * 1000;

        return elapsed;
    }
    else
    {
        gettimeofday( &start, NULL );

        first = 1;

        return 0 ;
    }
}

static void
moveBall(CompScreen *s, GLfloat Xrot, GLfloat Yrot)
{
  int i = 0;
  int xc = -1;
  int zc = -1;
  double t, dt;
  double dtx;   /* Time step in X direction */
  double dtz;   /* Time step in Z direction */
  double tdtx;  /* Temp time step in X dir */
  double tdtz;  /* temp time step in Z dir */
  GLfloat mx;   /* Movement in the X direction */
  GLfloat mz;   /* Movement in the Z dirction */
  GLfloat dbx;  /* Possible X position for the ball */
  GLfloat dbz;  /* Possible Z position for the ball */


  MAZE_SCREEN(s);

  /* The compiz cube can be rotated -100 to 100 I max out at -90 to 90 */
  if ( Yrot > 90.0 )
    { 
      Yrot = 90.0;
    }
  else
  if ( Yrot < -90.0 )
    {
      Yrot = -90.0;
    }

  //
  // How long since we were last called
  //
  t = fgElapsedTime() / 1000.0;

  //
  // If this is the first time through set last time to current
  //
  if ( ms->t0 < 0.0 )
    {
      ms->t0 = t;
    }

  dt = t - ms->t0;

  ms->t0 = t;

  /*
   * Calculate the new position of the ball and move the origin to the center
   * To explain the +90 ... picture if you can the maze Y rotated 90 degress
   * and a line drawn from the center to the bottom of your screen. Now whenever
   * you rotate the maze moving the mouse left or right the calculation below
   * will keep that line pointing down, giving me a false gravity, but at least
   * a direction to move the ball to. The +90 moves the line from horizontal to
   * vertical.
   */
  mx = g_mazeWidth + g_mazeDiameter * cosf((Xrot+90)*toRadians);
  mz = g_mazeDepth + g_mazeDiameter * sinf((Xrot+90)*toRadians);

  /*
   * If the maze is flipped, then flip the co-ordinates
   */
  if ( Yrot < 0 )
     {
       mx = g_mazeDiameter - mx;
       mz = g_mazeDiameter - mz;
     }

  /*
   * Calculate a tentative new ball position
   */
  dbx = ms->position[0] + (ms->velocity[0] * dt); // + ms->acceleration[0] * (dt*dt*0.5);
  dbz = ms->position[2] + (ms->velocity[2] * dt); // + ms->acceleration[2] * (dt*dt*0.5);

  //
  // Set these just in case there's no change
  //
  dtx = dt;
  dtz = dt;

  //
  // Check the projected X & Z position
  //
  for ( i=0; i < ms->currentWallCount; i++ )
    {
      /* 
       * Don't check another wall on a cube we've already hit
       */      
      if ( ms->wall[i].brick == xc || ms->wall[i].brick == zc ) continue;

      //
      // If we've not already hit a wall in the X direction
      // then
      //    check to see if we're going to hit a wall by continuing to move
      //
      tdtx = check_wall_x ( ms->position[0], ms->position[2], dbx, dbz, ms->wall[i].min_x, ms->wall[i].min_z, ms->wall[i].max_x, ms->wall[i].max_z, dt );

      if ( tdtx < dtx )
	{
	  dtx = tdtx;
	  
	  xc = ms->wall[i].brick;
	}
      
      tdtz = check_wall_z ( ms->position[0], ms->position[2], dbx, dbz, ms->wall[i].min_x, ms->wall[i].min_z, ms->wall[i].max_x, ms->wall[i].max_z, dt );

      if ( tdtz < dtz )
	{
	  dtz = tdtz;

	  zc = ms->wall[i].brick;
	}
    }

  /* 
   * Now move the ball in the right direction to the collision point if necessary
   */
  ms->position[0] += (ms->velocity[0] * dtx); // + ms->acceleration[0] * (dtx*dtx*0.5);
  ms->position[2] += (ms->velocity[2] * dtz); //+ ms->acceleration[2] * (dtz*dtz*0.5);

  /*
   * Set the acceleration
   */
  ms->acceleration[0] = (mx - ms->last_mx);
  ms->acceleration[2] = (mz - ms->last_mz);

  /*
   * Increase or decrease the velocity
   */
  ms->velocity[0] += ms->acceleration[0];
  ms->velocity[2] += ms->acceleration[2];

  /*
   * Make sure we don't go too fast in the X direction
   */
  if ( ms->velocity[0] > ms->maxSpeed ) ms->velocity[0] = ms->maxSpeed;
  else
  if ( ms->velocity[0] < -ms->maxSpeed ) ms->velocity[0] = -ms->maxSpeed;

  //
  // Make sure we don't go too fast in the Z direction
  //
  if ( ms->velocity[2] > ms->maxSpeed ) ms->velocity[2] = ms->maxSpeed;
  else
  if ( ms->velocity[2] < -ms->maxSpeed ) ms->velocity[2] = -ms->maxSpeed;

  //
  // In theory we should never go out of bounds, but I found in testing if the mouse was moved out of the test window, the ball broke loose ;-(
  //
  if ( ms->position[0] >= Xmax ) // Wall edge
    {
     ms->position[0] = Xmax;
      Zstep = -Zstep;  // Rotate the ball the other way
    }

  if ( ms->position[0] <= Xmin)
    {
      ms->position[0] = Xmin;
      Zstep = -Zstep;
    }

  if ( ms->position[2] >= Zmax )
    {
      ms->position[2] = Zmax;
      Ystep = -Ystep;
    }
	

  if ( ms->position[2] <= Zmin )
    {
      ms->position[2] = Zmin;
      Ystep = -Ystep;
    }

  //  ms->last_xrot = Xrot;
  //  ms->last_yrot = Yrot;
  ms->last_mx = mx;
  ms->last_mz = mz;
}


static int 
check ( CompScreen *s, unsigned int i, GLfloat min_x, GLfloat min_y, GLfloat min_z, GLfloat max_x, GLfloat max_y, GLfloat max_z, int brick )
{
  int result = 0;

    MAZE_SCREEN(s);

    //
    // Make sure the brick being testing is within limits
    //
    if ( (i >= 0) && (i < strlen(g_mazeString)) )
      {
	//
	// If there is no brick there then it's something we can hit!
	//
	if ( g_mazeString[i] != '*' )
	  {
	    ms->wall = (Wall *)realloc(ms->wall, sizeof(Wall) * (ms->currentWallCount+1));

	    if ( !ms->wall ) return 1; /* At least draw the wall even if there's not enough memory */
	    
	    // Wall to bounce off
	    ms->wall[ms->currentWallCount].min_x = min_x;
	    ms->wall[ms->currentWallCount].min_y = min_y;
	    ms->wall[ms->currentWallCount].min_z = min_z;
	    
	    // Inner (hidden) wall
	    ms->wall[ms->currentWallCount].max_x = max_x;
	    ms->wall[ms->currentWallCount].max_y = max_y;
	    ms->wall[ms->currentWallCount].max_z = max_z;
	    
	    // Which brick we are part of, used by collsion detection to stop us hanging on a corner
	    ms->wall[ms->currentWallCount].brick = brick;
	    
	    //
	    // Increment the number of brick wall
	    ms->currentWallCount++;

	    result = 1;
	  }
      }

    return result;
}


static void
buildMaze(CompScreen *s)
{
  unsigned int i = 0;
  int j = 0;
  GLfloat Z = 0.0f;
  GLfloat X = 0.0f;

  MAZE_SCREEN(s);

  ms->currentWallCount = 0;

  do
    {
      //
      // Store where we are
      //
      glPushMatrix();
      
      //
      // Move to the first block of the relevant row, col
      //
      glTranslatef(X, 0.0f, Z);
      
      //
      // Now draw bricks in the relevant positions
      //
      for ( j=0; j < g_mazeWidth; j++)
	{
	  //
	  // If a the string map says draw a brick
	  // then
	  //     draw one
	  // fi
	  //
	  if ( g_mazeString[i] == '*' )
	    {
		  glBegin(GL_QUADS);
		  
		  // Draw the top and bottom of the cube
		  glColor4usv(mazeGetTopFaceColour (s));
		  //		  glColor3f(0.0f,1.0f,0.0f);			// Set The Color To Green
		  glVertex3f( 1.0f, 1.0f,-1.0f);			// Top Right Of The Quad (Top)
		  glVertex3f(-1.0f, 1.0f,-1.0f);			// Top Left Of The Quad (Top)
		  glVertex3f(-1.0f, 1.0f, 1.0f);			// Bottom Left Of The Quad (Top)
		  glVertex3f( 1.0f, 1.0f, 1.0f);			// Bottom Right Of The Quad (Top)

		  glColor4usv(mazeGetBottomFaceColour (s));
		  //		  glColor3f(1.0f,0.5f,0.0f);			// Set The Color To Orange
		  glVertex3f( 1.0f,-1.0f, 1.0f);			// Top Right Of The Quad (Bottom)
		  glVertex3f(-1.0f,-1.0f, 1.0f);			// Top Left Of The Quad (Bottom)
		  glVertex3f(-1.0f,-1.0f,-1.0f);			// Bottom Left Of The Quad (Bottom)
		  glVertex3f( 1.0f,-1.0f,-1.0f);			// Bottom Right Of The Quad (Bottom)
		  

		  if ( (j != g_mazeWidth-1) && check(s, i+1,           X+1.75f, 1.0f, Z+1.75f, X+0.25f, -1.0f, Z-1.75f, i ) == 1 )  // North
		    {
		      glColor4usv(mazeGetNorthFaceColour (s));
		      //glColor3f(1.0f,0.0f,1.0f);			// Set The Color To Violet
		      glVertex3f( 1.0f, 1.0f,-1.0f);			// Top Right Of The Quad (Right)
		      glVertex3f( 1.0f, 1.0f, 1.0f);			// Top Left Of The Quad (Right)
		      glVertex3f( 1.0f,-1.0f, 1.0f);			// Bottom Left Of The Quad (Right)
		      glVertex3f( 1.0f,-1.0f,-1.0f);			// Bottom Right Of The Quad (Right)
		    }
		  
		  if ( (j != 0) && check(s, i-1,           X-1.75f, 1.0f, Z-1.75f, X-0.25f, -1.0f, Z+1.75f, i ) == 1 )   // East - Left
		    {
		      glColor4usv(mazeGetEastFaceColour (s));
		      //		      glColor3f(0.0f,0.0f,1.0f);			// Set The Color To Blue
		      glVertex3f(-1.0f, 1.0f, 1.0f);			// Top Right Of The Quad (Left)
		      glVertex3f(-1.0f, 1.0f,-1.0f);			// Top Left Of The Quad (Left)
		      glVertex3f(-1.0f,-1.0f,-1.0f);			// Bottom Left Of The Quad (Left)
		      glVertex3f(-1.0f,-1.0f, 1.0f);			// Bottom Right Of The Quad (Left)
		    }
		  
		  if ( check(s, i+g_mazeWidth, X-1.75f, 1.0f, Z+1.75f, X+1.75f, -1.0f, Z+0.25f, i ) == 1 )   // Front
		    {
		      glColor4usv(mazeGetSouthFaceColour (s));
		      //		      glColor3f(1.0f,0.0f,0.0f);			// Set The Color To Red
		      glVertex3f( 1.0f, 1.0f, 1.0f);			// Top Right Of The Quad (Front)
		      glVertex3f(-1.0f, 1.0f, 1.0f);			// Top Left Of The Quad (Front)
		      glVertex3f(-1.0f,-1.0f, 1.0f);			// Bottom Left Of The Quad (Front)
		      glVertex3f( 1.0f,-1.0f, 1.0f);			// Bottom Right Of The Quad (Front)
		    }
		  
		  if ( check(s, i-g_mazeWidth, X+1.75f, 1.0f, Z-1.75f, X-1.75f, -1.0f, Z-0.25f, i ) == 1 )  // Back		    
		    {
		      glColor4usv(mazeGetWestFaceColour (s));
		      //		      glColor3f(1.0f,1.0f,0.0f);			// Set The Color To Yellow
		      glVertex3f( 1.0f,-1.0f,-1.0f);			// Bottom Left Of The Quad (Back)
		      glVertex3f(-1.0f,-1.0f,-1.0f);			// Bottom Right Of The Quad (Back)
		      glVertex3f(-1.0f, 1.0f,-1.0f);			// Top Right Of The Quad (Back)
		      glVertex3f( 1.0f, 1.0f,-1.0f);			// Top Left Of The Quad (Back)
		    }

		  glEnd();
	    }
	  
	  //
	  // Move to the middle of the next brick
	  //
	  glTranslatef(2.0,0.0f,0.0f);
	  
	  i++;               // Increment the counter into the maze string
	  X += 2.0f;         // Move the X position on 2.0 as well
	}
      
      X = 0.0f;  // Reset X
      Z += 2.0f;             // Move up the Z plane 2.0
      
      //
      // Restore position back to square one
      //
      glPopMatrix();
    }
  while ( i < strlen(g_mazeString));

  /*
   * Now put a nice border around the maze
   */
  glTranslatef(0.0f, 0.0f, 0.0f);
  
  glBegin(GL_QUAD_STRIP);
  
  glColor4usv(mazeGetBorderColour (s));
  //  glColor3f(0.5f,0.5f,0.5f);			// Set The Color To ???
  
  glVertex3f( -1.0f, -1.0f, -1.0f);			// Top Right Of The Quad (Right)    v0
  glVertex3f( -1.0f, 1.0f, -1.0f);			// Top Left Of The Quad (Right)     v1
  
  glVertex3f( 31.0f, -1.0f, -1.0f);			// Bottom Left Of The Quad (Right)  v2
  glVertex3f( 31.0f,1.0f, -1.0f);			// Bottom Right Of The Quad (Right) v3
  
  glVertex3f( 31.0f, -1.0f, 31.0f );
  glVertex3f( 31.0f, 1.0f, 31.0f );
  
  glVertex3f( -1.0f, -1.0f, 31.0f );
  glVertex3f( -1.0f, 1.0f, 31.0f );
  
  glVertex3f( -1.0f, -1.0f, -1.0f);			// Top Right Of The Quad (Right)    v0
  glVertex3f( -1.0f, 1.0f, -1.0f);			// Top Left Of The Quad (Right)     v1
  
  glEnd();

}

static void
buildBall (CompScreen *s)
{
  GLfloat a, b;
  GLfloat da = 18.0, db = 18.0;
  GLfloat radius = 0.75;
  GLuint color;
  GLfloat x, y, z;

  color = 0;
  for (a = -90.0; a + da <= 90.0; a += da) {

    glBegin(GL_QUAD_STRIP);
    for (b = 0.0; b <= 360.0; b += db) {

      if (color) {
	glColor4usv(mazeGetBallColour1 (s));
	//	glColor3f(1.0f,0.0f,0.0f);
	//        glColor3f(1, 0, 0);
      } else {
	glColor4usv(mazeGetBallColour2 (s));
	//	glColor3f(1.0f,1.0f,1.0f);
	//        glColor3f(1, 1, 1);
      }

      x = radius * COS(b) * COS(a);
      y = radius * SIN(b) * COS(a);
      z = radius * SIN(a);
      glVertex3f(x, y, z);

      x = radius * COS(b) * COS(a + da);
      y = radius * SIN(b) * COS(a + da);
      z = radius * SIN(a + da);
      glVertex3f(x, y, z);

      color = 1 - color;
    }
    glEnd();

  }
}

static void
mazeClearTargetOutput (CompScreen *s,
			float      xRotate,
			float      vRotate)
{
    MAZE_SCREEN (s);
    CUBE_SCREEN (s);

    UNWRAP (ms, cs, clearTargetOutput);
    (*cs->clearTargetOutput) (s, xRotate, vRotate);
    WRAP (ms, cs, clearTargetOutput, mazeClearTargetOutput);

    glClear (GL_DEPTH_BUFFER_BIT);
}

static void
mazePaintInside (CompScreen              *s,
		  const ScreenPaintAttrib *sAttrib,
		  const CompTransform     *transform,
		  CompOutput              *output,
		  int                     size)
{
    GLfloat xRot, vRot;
    GLfloat progress;

    MAZE_SCREEN (s);
    CUBE_SCREEN (s);

    //    static GLfloat white[4] = { 1.0, 1.0, 1.0, 1.0 };

    ScreenPaintAttrib sA = *sAttrib;

    sA.yRotate += (360.0f / size) * (cs->xRotations - (s->x * cs->nOutput));

    CompTransform mT = *transform;

    (*s->applyScreenTransform) (s, &sA, output, &mT);

    glPushMatrix();
    glLoadMatrixf (mT.m);
    glTranslatef (cs->outputXOffset, -cs->outputYOffset, 0.0f);
    glScalef (cs->outputXScale, cs->outputYScale, 1.0f);

    Bool enabledCull = FALSE;

    glPushAttrib (GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);

    glDisable (GL_BLEND);

    /* Enable DEPTH TEST */
    glEnable(GL_DEPTH_TEST);

    /* Don't CULL the wall faces */
    if (glIsEnabled (GL_CULL_FACE) )
      {
	enabledCull = FALSE;
	glDisable (GL_CULL_FACE);
      }

    glPushMatrix();


    /* start add */
    glScalef (mazeGetScale(s), mazeGetScale(s), mazeGetScale(s));

    /* Is this the first time through? */
    if ( ms->yRotation == -1.0f )
      {
	ms->yRotation = sA.yRotate;
      }
 
    /*
     * Rotate the maze so it faces the right way everytime
     */
    glRotatef(-ms->yRotation, 0.0f, 1.0f, 0.0f );

    /*
     * Move the maze so it's in the middle
     */
    glTranslatef(-g_mazeWidth,mazeGetHeight(s),-g_mazeDepth);

    /*
     * Draw the maze
     */
    glCallList(ms->maze);

    /* Get the current X and Y rotation values */
    (*cs->getRotation) (s, &xRot, &vRot, &progress);

    /* Calculate a new position for the ball */
    moveBall (s, xRot, vRot );	

    /* Move the ball to the new position */
    glTranslatef(ms->position[0],ms->position[1],ms->position[2]);

    /* Draw the ball */
    glCallList(ms->mazeBall);

    /* end add */
    glPopMatrix();

    glDisable (GL_LIGHT1);
    glDisable (GL_NORMALIZE);
    glEnable (GL_COLOR_MATERIAL);

    if (!s->lighting)
	glDisable (GL_LIGHTING);

    glDisable (GL_DEPTH_TEST);

    if (enabledCull)
	glDisable (GL_CULL_FACE);

    glPopMatrix();
    glPopAttrib();

    ms->damage = TRUE;

    UNWRAP (ms, cs, paintInside);
    (*cs->paintInside) (s, sAttrib, transform, output, size);
    WRAP (ms, cs, paintInside, mazePaintInside);
}

static void
mazePreparePaintScreen (CompScreen *s,
			int        mss )
{
    MAZE_SCREEN (s);

    ms->contentRotation += mss * 360.0 / 20000.0;
    ms->contentRotation = fmod (ms->contentRotation, 360.0);

    UNWRAP (ms, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, mss);
    WRAP (ms, s, preparePaintScreen, mazePreparePaintScreen);
}

static void
mazeDonePaintScreen (CompScreen * s)
{
    MAZE_SCREEN (s);

    if (ms->damage)
    {
	damageScreen (s);
	ms->damage = FALSE;
    }
    else
      {
	ms->velocity[0] = 0.0f;
	ms->velocity[1] = 0.0f;
	ms->velocity[2] = 0.0f;
	ms->position[0] = 30.0f;   /* Inital ball position */
	ms->position[1] = 0.5f;
	ms->position[2] = 2.0f;
	ms->t0 = -1;
	ms->last_mx = 16.0f;
	ms->last_mz = 16.0f;
	ms->yRotation = -1.0f;
      }

    UNWRAP (ms, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (ms, s, donePaintScreen, mazeDonePaintScreen);
}


static Bool
mazeInitDisplay (CompPlugin  *p,
		  CompDisplay *d)
{
    MazeDisplay *gd;

    if (!checkPluginABI ("core", CORE_ABIVERSION) ||
	!checkPluginABI ("cube", CUBE_ABIVERSION))
	return FALSE;

    if (!getPluginDisplayIndex (d, "cube", &cubeDisplayPrivateIndex))
	return FALSE;

    gd = malloc (sizeof (MazeDisplay));

    if (!gd)
	return FALSE;

    gd->screenPrivateIndex = allocateScreenPrivateIndex (d);

    if (gd->screenPrivateIndex < 0)
    {
	free (gd);
	return FALSE;
    }

    d->base.privates[displayPrivateIndex].ptr = gd;

    return TRUE;
}

static void
mazeFiniDisplay (CompPlugin  *p,
		  CompDisplay *d)
{
    MAZE_DISPLAY (d);

    freeScreenPrivateIndex (d, gd->screenPrivateIndex);
    free (gd);
}

static void
mazeScreenOptionChange ( CompScreen *s,
			 CompOption *opt,
			 MazeScreenOptions num)
{
  MAZE_SCREEN(s);

  if ( ms->wall )
    free(ms->wall);

  ms->wall = NULL;

  glDeleteLists (ms->maze, 1);
  glDeleteLists (ms->mazeBall, 1);
  
  ms->maze = glGenLists (1);
  glNewList(ms->maze, GL_COMPILE);
  buildMaze(s);
  glEndList();
  
  ms->mazeBall = glGenLists(1);
  glNewList(ms->mazeBall, GL_COMPILE);
  buildBall(s);
  glEndList();    
}

static Bool
mazeInitScreen (CompPlugin *p,
		 CompScreen *s)
{
    MazeScreen *ms;

    static GLfloat pos[4]         = { 5.0, 5.0, 10.0, 0.0 };
    //    static GLfloat red[4]         = { 0.8, 0.1, 0.0, 1.0 };
    //    static GLfloat green[4]       = { 0.0, 0.8, 0.2, 1.0 };
    //    static GLfloat blue[4]        = { 0.2, 0.2, 1.0, 1.0 };
    static GLfloat ambientLight[] = { 0.3f, 0.3f, 0.3f, 0.3f };
    static GLfloat diffuseLight[] = { 0.5f, 0.5f, 0.5f, 0.5f };

    MAZE_DISPLAY (s->display);

    CUBE_SCREEN (s);

    ms = malloc (sizeof (MazeScreen) );

    if (!ms)
	return FALSE;

    s->base.privates[gd->screenPrivateIndex].ptr = ms;

    glLightfv (GL_LIGHT1, GL_AMBIENT, ambientLight);
    glLightfv (GL_LIGHT1, GL_DIFFUSE, diffuseLight);
    glLightfv (GL_LIGHT1, GL_POSITION, pos);

    ms->contentRotation = 0.0;
    ms->wall = NULL;
    ms->velocity[0] = 0.0f;
    ms->velocity[1] = 0.0f;
    ms->velocity[2] = 0.0f;
    ms->position[0] = -14.0f;
    ms->position[1] = 0.5f;
    ms->position[2] = 7.0f;
    ms->yRotation   = -1.0f;
    ms->t0 = -1;
    ms->maxSpeed = mazeGetMaxSpeed(s);

    ms->maze = glGenLists (1);
    glNewList(ms->maze, GL_COMPILE);
    buildMaze(s);
    glEndList();

    ms->mazeBall = glGenLists(1);
    glNewList(ms->mazeBall, GL_COMPILE);
    buildBall(s);
    glEndList();
    
    mazeSetTopFaceColourNotify(s, mazeScreenOptionChange);
    mazeSetBottomFaceColourNotify(s, mazeScreenOptionChange);
    mazeSetNorthFaceColourNotify(s, mazeScreenOptionChange);
    mazeSetEastFaceColourNotify(s, mazeScreenOptionChange);
    mazeSetSouthFaceColourNotify(s, mazeScreenOptionChange);
    mazeSetWestFaceColourNotify(s, mazeScreenOptionChange);
    mazeSetBallColour1Notify(s, mazeScreenOptionChange);
    mazeSetBallColour2Notify(s, mazeScreenOptionChange);
    mazeSetBorderColourNotify(s, mazeScreenOptionChange);

    WRAP (ms, s, donePaintScreen, mazeDonePaintScreen);
    WRAP (ms, s, preparePaintScreen, mazePreparePaintScreen);
    WRAP (ms, cs, clearTargetOutput, mazeClearTargetOutput);
    WRAP (ms, cs, paintInside, mazePaintInside);

    return TRUE;
}

static void
mazeFiniScreen (CompPlugin *p,
		 CompScreen *s)
{
    MAZE_SCREEN (s);
    CUBE_SCREEN (s);

    if ( ms->wall )
      free(ms->wall);

    glDeleteLists (ms->maze, 1);
    glDeleteLists (ms->mazeBall, 1);

    UNWRAP (ms, s, donePaintScreen);
    UNWRAP (ms, s, preparePaintScreen);
    UNWRAP (ms, cs, clearTargetOutput);
    UNWRAP (ms, cs, paintInside);

    free (ms);
}

static Bool
mazeInit (CompPlugin * p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex();

    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
mazeFini (CompPlugin * p)
{
    if (displayPrivateIndex >= 0)
	freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompBool
mazeInitObject (CompPlugin *p,
		 CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) mazeInitDisplay,
	(InitPluginObjectProc) mazeInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
mazeFiniObject (CompPlugin *p,
		 CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) mazeFiniDisplay,
	(FiniPluginObjectProc) mazeFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable mazeVTable = {
    "maze",
    0,
    mazeInit,
    mazeFini,
    mazeInitObject,
    mazeFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &mazeVTable;
}
