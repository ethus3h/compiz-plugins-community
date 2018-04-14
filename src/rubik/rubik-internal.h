#ifndef _RUBIK_INTERNAL_H
#define _RUBIK_INTERNAL_H

#define LRAND()                 ((long) (random() & 0x7fffffff))
#define NRAND(n)                ((int) (LRAND() % (n)))
#define MAXRAND                 (2147483648.0) /* unsigned 1<<31 as a float */


#include <math.h>
#include <float.h>

/* some constants */
#define PI     M_PI
#define PIdiv2 M_PI_2
#define toDegrees (180.0f * M_1_PI)
#define toRadians (M_PI / 180.0f)

//return random number in range [0,x)
#define randf(x) ((float) (rand()/(((double)RAND_MAX + 1)/(x))))


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

typedef struct _orderedFaceRec orderedFaceRec;


typedef struct _RubikDisplay
{
    int screenPrivateIndex;
}
RubikDisplay;


typedef struct _squareRec
{
    int side;
    int x;
    int y;
    int psi;
}
squareRec;

typedef struct _faceRec
{
    float  color[4];
	
    squareRec *square;
}
faceRec;

struct _orderedFaceRec
{ //not used yet
    squareRec *square;
    orderedFaceRec *nextOrderedFace;
    CompWindow *w;
};


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

    float *psi;
    float *oldPsi;

    float desktopOpacity;
    
    CompWindow ** w;

    Box * oldClip;

    faceRec *faces;

    int hsize;
    float distance;    //perpendicular distance to wall from centre
    float radius;      //radius on which the hSize points lie

    float speedFactor; // multiply rotation speed by this value
}
RubikScreen;

typedef struct _RubikWindow{
    DrawWindowGeometryProc drawWindowGeometry;
}
RubikWindow;


void rubikGetRotation( CompScreen *s, float *x, float *v );

void initializeWorldVariables(CompScreen *s);
void initFaces( CompScreen *s);
void updateSpeedFactor(float);



//utility methods
void setColor(float *, float, float, float, float, float, float);
void setSpecifiedColor (float *, int);
void rotateClockwise (squareRec * square);
void rotateAnticlockwise (squareRec * square);


//All calculations that matter with angles are done clockwise from top.
//I think of it as x=radius, y=0 being the top (towards 1st desktop from above view)
//and the z coordinate as height.


int vStrips;
int currentVStrip; 

int hStrips;
int currentHStrip;

float currentStripCounter;
int currentStripDirection;

int rotationAxis; //0 - horizontal
				  //1 - vertical from 1st viewport
				  //2 - vertical from 2nd viewport

#endif
