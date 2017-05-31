#ifndef _RUBIK_INTERNAL_H
#define _RUBIK_INTERNAL_H

#define RAD 57.296
#define RRAD 0.01745

/* some constants */
#define PI     3.14159265358979323846
#define PIdiv2 1.57079632679489661923
#define toDegrees 57.2957795130823208768
#define toRadians 0.01745329251994329577


#define LRAND()                 ((long) (random() & 0x7fffffff))
#define NRAND(n)                ((int) (LRAND() % (n)))
#define MAXRAND                 (2147483648.0) /* unsigned 1<<31 as a float */


#include <math.h>
#include <float.h>

#include <math.h>
#include <compiz-core.h>
#include <compiz-cube.h>

extern int rubikDisplayPrivateIndex;
extern int cubeDisplayPrivateIndex;

#define GET_RUBIK_DISPLAY(d) \
    ((RubikDisplay *) (d)->base.privates[rubikDisplayPrivateIndex].ptr)
#define RUBIK_DISPLAY(d) \
    RubikDisplay *rd = GET_RUBIK_DISPLAY(d);

#define GET_RUBIK_SCREEN(s, rd) \
    ((RubikScreen *) (s)->base.privates[(rd)->screenPrivateIndex].ptr)
#define RUBIK_SCREEN(s) \
    RubikScreen *rs = GET_RUBIK_SCREEN(s, GET_RUBIK_DISPLAY(s->display))

#define GET_RUBIK_WINDOW(w, rs) \
    ((RubikWindow *) (w)->base.privates[(rs)->windowPrivateIndex].ptr)

#define RUBIK_WINDOW(w) \
    RubikWindow *rw = GET_RUBIK_WINDOW  (w, \
                       GET_RUBIK_SCREEN  (w->screen, \
                       GET_RUBIK_DISPLAY (w->screen->display)))

#define WIN_REAL_X(w) (w->attrib.x - w->input.left)
#define WIN_REAL_Y(w) (w->attrib.y - w->input.top)

#define WIN_REAL_W(w) (w->width + w->input.left + w->input.right)
#define WIN_REAL_H(w) (w->height + w->input.top + w->input.bottom)


#define NRAND(n)                ((int) (LRAND() % (n)))

#define RANDOMCOLOR	1
#define BLACK		2
#define BLUE		3
#define BROWN		4
#define CYAN		5
#define GREEN		6
#define GREY		7
#define ORANGE		8
#define PINK		9
#define PURPLE		10
#define RED			11
#define YELLOW		12
#define WHITE		13

typedef struct _RubikDisplay
{
    int screenPrivateIndex;
}
RubikDisplay;

typedef struct _faceRec
{
	float  color[4];
}
faceRec;

typedef struct _RubikScreen
{
    int windowPrivateIndex;
    
	DonePaintScreenProc donePaintScreen;
    PreparePaintScreenProc preparePaintScreen;

    CubeClearTargetOutputProc clearTargetOutput;
    CubePaintInsideProc       paintInside;

    PaintOutputProc paintOutput;
    PaintWindowProc paintWindow;
    DrawWindowProc drawWindow;
    PaintTransformedOutputProc paintTransformedOutput;

    DrawWindowTextureProc drawWindowTexture;
    
    DamageWindowRectProc damageWindowRect;
    
    DisableOutputClippingProc disableOutputClipping;

    AddWindowGeometryProc addWindowGeometry;
    
	CubeGetRotationProc	getRotation;

	
    Bool initiated;
    
    Bool damage;
    
    CompTransform * tempTransform;
    
	float *th;
	float *oldTh;
	
	CompWindow ** w;
	
    Box * oldClip;

    faceRec *faces;
    
    int numDesktopWindows;
}
RubikScreen;

typedef struct _RubikWindow{
	float x, y, z;
	float psi;
	Bool rotated;
	
    DrawWindowGeometryProc drawWindowGeometry;
}
RubikWindow;


void rubikGetRotation( CompScreen *s, float *x, float *v );

void initializeWorldVariables(int, float);
void updateSpeedFactor(float);



//utility methods
float randf(float); //random float
float minimum(float,float); //my compiler at home didn't have min or fminf!
float maximum(float,float); //nor did it have max or fmaxf!
float symmDistr(void); //symmetric distribution
void setColor(float *, float, float, float, float, float, float);
void setSpecifiedColor (float *, int);

//maybe define a struct for these values
float speedFactor; // global variable (fish/crab speeds multiplied by this value)
float radius;//radius on which the hSize points lie
float distance;//perpendicular distance to wall from centre


//All calculations that matter with angles are done clockwise from top.
//I think of it as x=radius, y=0 being the top (towards 1st desktop from above view)
//and the z coordinate as height.


int hSize; // horizontal desktop size
float q;   // equal to float version of hSize (replace with hSizef some time)

int vSize; // vertical size
int currentVStrip; 

float currentStripCounter;
int currentStripDirection;

#endif
