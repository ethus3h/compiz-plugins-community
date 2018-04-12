//a collection of utility methods

#include "rubik-internal.h"
#include <math.h>
#include <float.h>

void
setColor(float* color, float r, float g, float b, float a,
         float randomOffset, float randomness)
{
    int i;
    float ro = randf(randomOffset);
    
    color[0]=r+ro+randf(randomness);
    color[1]=g+ro+randf(randomness);
    color[2]=b+ro+randf(randomness);
    color[3]=a;

    for (i=0; i<4; i++)
    {
	if (color[i]<0)
	    color[i]=0;
	else if (color[i]>1)
	    color[i]=1;
    }
}
void
setSpecifiedColor(float* color, int c)
{
    switch (c)
    {
    case RANDOMCOLOR:
	setColor(color, 0, 0, 0, 1, 0, 1);
	break;
    case BLACK:
	setColor(color, 0, 0, 0, 1, 0, 0);
	break;
    case BLUE:
	setColor(color, 0, 0, 1, 1, 0, 0);
	break;
    case BROWN:
	setColor(color, 0.5, 0.25, 0.05, 1, 0, 0);
	break;
    case CYAN:
	setColor(color, 0, 1, 1, 1, 0, 0);
	break;
    case GREEN:
	setColor(color, 0, 1, 0, 1, 0, 0);
	break;
    case GREY:
	setColor(color, 0.5, 0.5, 0.5, 1, 0, 0);
	break;
    case ORANGE:
	setColor(color, 1, 0.5, 0, 1, 0, 0);
	break;
    case PINK:
	setColor(color, 1, 0, 1, 1, 0, 0);
	break;
    case PURPLE:
	setColor(color, 0.5, 0, 1, 1, 0, 0);
	break;
    case RED:
	setColor(color, 1, 0, 0, 1, 0, 0);
	break;
    case YELLOW:
	setColor(color, 1, 1, 0, 1, 0, 0);
	break;
    case WHITE:
	setColor(color, 1, 1, 1, 1, 0, 0);
	break;
    }
}
void
rotateClockwise(squareRec * square)
{

    int m, j, k;

    for (m=0; m<hStrips/2; m++)
    {
	for (j=0; j<hStrips-1-2*m; j++)
	{
	    int pos1 = (vStrips-1-j-m);
	    int pos2 = j+m;
	    squareRec tempSquare2 = square[pos2*hStrips+m];
	    square[pos2*hStrips+m] = square[m*hStrips+pos1];
	    square[m*hStrips+pos1] = square[pos1*hStrips+(vStrips-1-m)];
	    square[pos1*hStrips+(vStrips-1-m)] = square[(vStrips-1-m)*hStrips+pos2];
	    square[(vStrips-1-m)*hStrips+pos2] = tempSquare2;
	}
    }

    for (k=0; k<vStrips; k++)
    {
	for (j=0; j<hStrips; j++)
	{
	    square[j*hStrips+k].psi = (square[j*hStrips+k].psi+1)%4;
	}
    }
}
void rotateAnticlockwise(squareRec * square)
{
    int m, j, k;

    for (m=0; m<hStrips/2; m++)
    {
	for (j=0; j<hStrips-1-2*m; j++)
	{
	    int pos1 = (vStrips-1-j-m);
	    int pos2 = j+m;
	    squareRec tempSquare2 = square[pos2*hStrips+m];
	    square[pos2*hStrips+m] = square[(vStrips-1-m)*hStrips+pos2];
	    square[(vStrips-1-m)*hStrips+pos2] = square[pos1*hStrips+(vStrips-1-m)];
	    square[pos1*hStrips+(vStrips-1-m)] = square[m*hStrips+pos1];
	    square[m*hStrips+pos1] = tempSquare2;
	}
    }

    for (k=0; k<vStrips; k++)
    {
	for (j=0; j<hStrips; j++)
	{
	    square[j*hStrips+k].psi = (square[j*hStrips+k].psi+4-1)%4;
	}
    }
}
