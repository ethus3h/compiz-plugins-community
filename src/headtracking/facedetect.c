#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "facedetect.h"

#define CASCADE "haarcascade_frontalface_alt2.xml"
#define SCALE 1.0
#define UX 1024
#define UY 768
#define MAX_TRACK_PT 100
#define MIN_TRACK_PT 10
#define SMOOTH_STEP 5.0
#define MAX_REFRESH 100

static CvCapture* capture = 0;
static CvMemStorage* storage = 0;
static CvHaarClassifierCascade* cascade = 0;
static IplImage *frame_copy = 0;
static int prevx1, prevy1, prevx2, prevy2;
static int realprevx1, realprevy1, realprevx2, realprevy2;
static int barPx, barPy, barRx, barRy;
static int prevlar = 0, prevhau = 0;
static long int limitdist, mindist;
static CvPoint2D32f *trackingPoint[2] = {0,0};
static int nbTrackingPoint = 0;
static char *status = 0;
static IplImage *prevGray = 0, *gray = 0;
static IplImage *prevPyramid = 0, *pyramid = 0;
static int trackingFlags = 0;
static double beginx1 = 0.0, beginy1 = 0.0, beginx2 = 0.0, beginy2 = 0.0;
static int endx1 = 0, endy1 = 0, endx2 = 0, endy2 = 0;
static double pasx1 = 0.0, pasy1 = 0.0, pasx2 = 0.0, pasy2 = 0.0;
unsigned int refresh = 0;

// Thread part
static pthread_mutex_t mutex;
static pthread_attr_t attrThread;
static int thret = 0, thx1 = 0, thy1 = 0, thx2 = 0, thy2 = 0, threadAlive = 0;
static pthread_t tid = -1;

// Local protos
static void init(void);
static void end(void);
static int headtrackThread(int *x1, int *y1, int *x2, int *y2, int lissage);
void endThread(void);
static int detect(double scale, int uX, int uY, int *x1, int *y1, int *x2, int *y2, int lissage);
typedef struct {
	int lissage;
	int delay;
} threadarg_t;

// Thread part : background detection
static void *threadRoutine(void *p) {
	int ret, x1, y1, x2, y2, alive = 1;
	int lissage = ((threadarg_t *)p)->lissage;
	int delay = ((threadarg_t *)p)->delay;
	// free(p);

	init();
	while (alive) {
		ret = headtrackThread(&x1, &y1, &x2, &y2, lissage);
		pthread_mutex_lock(&mutex);
		thx1 = x1;
		thy1 = y1;
		thx2 = x2;
		thy2 = y2;
		thret = ret;
		alive = threadAlive;
		pthread_mutex_unlock(&mutex);
		usleep(delay);
	}
	end();
	pthread_mutex_lock(&mutex);
	tid = -1;
	pthread_mutex_unlock(&mutex);
	return(NULL);
}

void endThread(void) {
	pthread_t t = tid;

	pthread_mutex_lock(&mutex);
	threadAlive = 0;
	pthread_mutex_unlock(&mutex);
	while (t != -1) {
		sleep(1);
		pthread_mutex_lock(&mutex);
		t = tid;
		pthread_mutex_unlock(&mutex);
	}
}

void init(void)
{
    // FIXME: Use a type=file option in the CCS configuration
    //        to store the path to the cascade file.
	char *home = getenv("HOME"), *path = NULL;
	struct stat fileExists;

	// Cascade path
	if (home && strlen(home) > 0) {
		asprintf(&path, "%s/.compiz/data/%s", home, CASCADE);
		if (stat(path, &fileExists) != 0) {
			free(path);
			path = NULL;
		}
	}
	if (path == NULL) {
		asprintf(&path, "%s/%s", DATADIR, CASCADE);
		if (stat(path, &fileExists) != 0) {
			free(path);
			path = NULL;
		}
	}
	if (path != NULL) cascade = (CvHaarClassifierCascade*)cvLoad(path, 0, 0, 0);
	else cascade = 0;
	storage = cvCreateMemStorage(0);
	capture = cvCaptureFromCAM(0);
	prevx1 = prevy1 = prevx2 = prevy2 = 0;
	realprevx1 = realprevy1 = realprevx2 = realprevy2 = 0;
	barPx = barPy = barRx = barRy = 0;
	prevlar = prevhau = 0;
	limitdist = 0;
	mindist = 25;
	frame_copy = 0;
	trackingPoint[0] = (CvPoint2D32f*)cvAlloc(MAX_TRACK_PT * sizeof(trackingPoint[0][0]));
	trackingPoint[1] = (CvPoint2D32f*)cvAlloc(MAX_TRACK_PT * sizeof(trackingPoint[0][0]));
	nbTrackingPoint = 0;
	status = 0;
	prevGray = gray = 0;
	prevPyramid = pyramid = 0;
	trackingFlags = 0;
	refresh = 0;

	// Thread part
	thx1 = 0;
	thy1 = 0;
	thx2 = 0;
	thy2 = 0;
	thret = 0;
	pthread_mutex_init(&mutex, NULL);
	pthread_attr_init(&attrThread);
	pthread_attr_setdetachstate(&attrThread, PTHREAD_CREATE_DETACHED);
	pthread_attr_setschedpolicy(&attrThread, SCHED_OTHER);
	pthread_attr_setinheritsched(&attrThread, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setscope(&attrThread, PTHREAD_SCOPE_SYSTEM);
}

void end(void)
{
	cvReleaseImage(&frame_copy);
	cvReleaseCapture(&capture);
}

#ifdef TEST
void display(int x1, int y1, int x2, int y2, int type)
{
	int boucle;

	CvScalar color = CV_RGB(255, 0, 0);
	if (type == 1) {
		color = CV_RGB(0, 255, 0);
	}
	else
	if (type == 2) {
		color = CV_RGB(0, 0, 255);
	}
	else {
		color = CV_RGB(255, 255, 255);
	}
	cvRectangle(frame_copy, cvPoint(x1, y1), cvPoint(x2, y2), color, 1, 8, 0);
	if (type < 3) {
		for (boucle = 0 ; boucle < nbTrackingPoint ; boucle++) {
			if (status[boucle]) {
				cvCircle(frame_copy, cvPointFrom32f(trackingPoint[0][boucle]), 3, CV_RGB(200,0,0), 0, 8,0);
			}
		}
	}
	cvShowImage( "Head track", frame_copy );
	cvWaitKey(10);
}
#endif

int headtrack(int *x1, int *y1, int *x2, int *y2, int lissage, int smooth, int delay)
{
	int ret;
	threadarg_t *arg = NULL;

	if (tid == -1) {
		threadAlive = 1;
		arg = (threadarg_t *)malloc(sizeof(threadarg_t *));
		arg->lissage = lissage;
		arg->delay = delay;
		pthread_create(&tid, &attrThread, threadRoutine, (void *)arg);
		sleep(1);
		beginx1 = beginy1 = beginx2 = beginy2 = 0.0;
		endx1 = endy1 = endx2 = endy2 = 0;
		pasx1 = pasy1 = pasx2 = pasy2 = 0.0;
	}
	pthread_mutex_lock(&mutex);
	*x1 = thx1;
	*y1 = thy1;
	*x2 = thx2;
	*y2 = thy2;
	ret = thret;
	pthread_mutex_unlock(&mutex);

	// Smmoothing the movement
#ifdef TEST
	display(*x1 * frame_copy->width / UX, *y1 * frame_copy->height / UY, *x2 * frame_copy->width / UX, *y2 * frame_copy->height / UY, 1);
#endif
	if (smooth) {
		if (endx1 != *x1 || endy1 != *y1 || endx2 != *x2 || endy2 != *y2) {
			endx1 = *x1;
			endy1 = *y1;
			endx2 = *x2;
			endy2 = *y2;
			if (beginx1 != 0.0 && beginy1 != 0.0 && beginx2 != 0.0 && beginy2 != 0.0) {
				pasx1 = ((double)*x1 - beginx1) / SMOOTH_STEP;
				pasy1 = ((double)*y1 - beginy1) / SMOOTH_STEP;
				pasx2 = ((double)*x2 - beginx2) / SMOOTH_STEP;
				pasy2 = ((double)*y2 - beginy2) / SMOOTH_STEP;
				if (pasx1 != 0.0 && pasy1 != 0.0 && pasx2 != 0.0 && pasy2 != 0.0) {
					beginx1 += pasx1;
					beginy1 += pasy1;
					beginx2 += pasx2;
					beginy2 += pasy2;
					*x1 = cvRound(beginx1);
					*y1 = cvRound(beginy1);
					*x2 = cvRound(beginx2);
					*y2 = cvRound(beginy2);
				}
				else {
					pasx1 = pasy1 = pasx2 = pasy2 = 0.0;
				}
			}
			else {
				beginx1 = (double)*x1;
				beginy1 = (double)*y1;
				beginx2 = (double)*x2;
				beginy2 = (double)*y2;
			}
		}
		else
		if (pasx1 != 0.0 && pasy1 != 0.0 && pasx2 != 0.0 && pasy2 != 0.0) {
			if (((double)*x1 - beginx1) / pasx1 > 0.0 && ((double)*y1 - beginy1) / pasy1 > 0.0 && ((double)*x2 - beginx2) / pasx2 > 0.0 && ((double)*y2 - beginy2) / pasy2 > 0.0) {
				beginx1 += pasx1;
				beginy1 += pasy1;
				beginx2 += pasx2;
				beginy2 += pasy2;
				*x1 = cvRound(beginx1);
				*y1 = cvRound(beginy1);
				*x2 = cvRound(beginx2);
				*y2 = cvRound(beginy2);
			}
			else {
				pasx1 = pasy1 = pasx2 = pasy2 = 0.0;
			}
		}
		else {
			beginx1 = (double)*x1;
			beginy1 = (double)*y1;
			beginx2 = (double)*x2;
			beginy2 = (double)*y2;
			pasx1 = pasy1 = pasx2 = pasy2 = 0.0;
		}
#ifdef TEST
		display(*x1 * frame_copy->width / UX, *y1 * frame_copy->height / UY, *x2 * frame_copy->width / UX, *y2 * frame_copy->height / UY, 3);
#endif
	}
	return(ret);
}

int headtrackThread(int *x1, int *y1, int *x2, int *y2, int lissage)
{
	IplImage *frame = 0;
	int retour = 0;

	if (!capture) {
		init();
	}
	if (capture) {
		if (cvGrabFrame(capture)) {
			frame = cvRetrieveFrame( capture, 0 );
			if(frame) {
				if( !frame_copy )
					frame_copy = cvCreateImage( cvSize(frame->width,frame->height), IPL_DEPTH_8U, frame->nChannels );
				if( frame->origin == IPL_ORIGIN_TL )
					cvCopy(frame, frame_copy, 0);
				else
					cvFlip(frame, frame_copy, 0);
				retour = detect(SCALE, UX, UY, x1, y1, x2, y2, lissage);
			}
		}
	}
	return(retour);
}

int detect(double scale, int uX, int uY, int *x1, int *y1, int *x2, int *y2, int lissage)
{
	// Haar
	IplImage *small_img;
	int i, resx1, resy1, resx2, resy2, barx, bary, minx, miny, maxx, maxy;
	int provrealprevx1, provrealprevy1, provrealprevx2, provrealprevy2, provbarRx, provbarRy;
	long int realdist, prevdist;
	int boucle, retour = 0;

	// Track points
	IplImage *subimg, *eig, *temp;
	CvPoint2D32f *swapPoint;
	double quality = 0.01;
	double min_distance = 5;
	CvPoint pt;
	int nbIn = 0;
	double barX, barY;
	int lar = 0, hau = 0;
#ifdef TEST
	int type = 0;
#endif

	if (gray == 0) {
		gray = cvCreateImage(cvSize(frame_copy->width,frame_copy->height), 8, 1);
		status = (char*)cvAlloc(MAX_TRACK_PT);
		prevGray = cvCreateImage(cvGetSize(frame_copy), 8, 1);
		prevPyramid = cvCreateImage(cvGetSize(frame_copy), 8, 1);
		pyramid = cvCreateImage(cvGetSize(frame_copy), 8, 1);
	}
	if (limitdist == 0) {
		limitdist = frame_copy->width * frame_copy->width / 100;
	}

	if(cascade) {
		cvCvtColor(frame_copy, gray, CV_BGR2GRAY);

		// Recall last result
		resx1 = prevx1;
		resy1 = prevy1;
		resx2 = prevx2;
		resy2 = prevy2;
		lar = hau = 0;

		// Haar detection
		if (nbTrackingPoint < MIN_TRACK_PT || refresh > MAX_REFRESH) {
			small_img = cvCreateImage(cvSize(cvRound(frame_copy->width / scale), cvRound(frame_copy->height / scale)), 8, 1);
			cvResize(gray, small_img, CV_INTER_LINEAR);
			cvEqualizeHist(small_img, small_img);
			cvClearMemStorage(storage);
			CvSeq* faces = cvHaarDetectObjects( small_img, cascade, storage,
				1.1, 2, 0
				|CV_HAAR_FIND_BIGGEST_OBJECT
				//|CV_HAAR_DO_ROUGH_SEARCH
				//|CV_HAAR_DO_CANNY_PRUNING
				//|CV_HAAR_SCALE_IMAGE
				,
				cvSize(30, 30),
				cvSize(0, 0) );
			for( i = 0; i < (faces ? faces->total : 0); i++ ) {
				CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
				resx1 = r->x * scale;
				resy1 = r->y * scale;
				resx2 = (r->x + r->width) * scale;
				resy2 = (r->y + r->height) * scale;

				// Initialize the tracking points
				cvSetImageROI(gray, cvRect(r->x * scale, r->y * scale, r->width * scale, r->height * scale));
				subimg = cvCreateImage(cvSize(r->width * scale, r->height * scale), gray->depth, gray->nChannels);
				cvCopy(gray, subimg, 0);
				cvResetImageROI(gray);
				eig = cvCreateImage(cvGetSize(subimg), 32, 1);
				temp = cvCreateImage(cvGetSize(subimg), 32, 1);
				nbTrackingPoint = MAX_TRACK_PT;
				cvGoodFeaturesToTrack(subimg, eig, temp, trackingPoint[0], &nbTrackingPoint, quality, min_distance, 0, 3, 0, 0.04);
				cvReleaseImage(&eig);
				cvReleaseImage(&temp);
				barX = barY = 0.0;
				for(boucle = 0 ; boucle < nbTrackingPoint ; boucle++) {
					pt = cvPointFrom32f(trackingPoint[0][boucle]);
					trackingPoint[0][boucle] = cvPoint2D32f(pt.x + r->x * scale, pt.y + r->y * scale);
					barX += trackingPoint[0][boucle].x;
					barY += trackingPoint[0][boucle].y;
				}
				barX /= (double)nbTrackingPoint;
				barY /= (double)nbTrackingPoint;

				// Initialisations
				trackingFlags = 0;

				// Dimensions
				prevlar = lar = (resx2 - resx1) / 2;
				prevhau = hau = (resy2 - resy1) / 2;

				retour = 1;
				refresh = 0;
#ifdef TEST
				type = 1;
#endif
			}
			cvReleaseImage(&small_img);
		}

		// Update the tracking points (if yet initialized)
		if (nbTrackingPoint >= MIN_TRACK_PT) {
			refresh++;
			cvCalcOpticalFlowPyrLK(prevGray, gray, prevPyramid, pyramid, trackingPoint[0], trackingPoint[1], nbTrackingPoint, cvSize(10, 10), 3, status, 0, cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 20, 0.03), trackingFlags);
			trackingFlags |= CV_LKFLOW_PYR_A_READY;
			nbIn = 0;
			barX = barY = 0.0;
			maxx = maxy = 0;
			minx = frame_copy->width;
			miny = frame_copy->height;

			// Barycenter
			for (boucle = 0 ; boucle < nbTrackingPoint ; boucle++) {
				if (status[boucle]) {
					nbIn++;
					barX += trackingPoint[1][boucle].x;
					barY += trackingPoint[1][boucle].y;
				}
			}
			barX /= (double)nbIn;
			barY /= (double)nbIn;

			// Dispersion
			nbIn = 0;
			for (boucle = 0 ; boucle < nbTrackingPoint ; boucle++) {
				if (status[boucle]) {
					if (    trackingPoint[1][boucle].x <= barX + (double)prevlar &&
						trackingPoint[1][boucle].x >= barX - (double)prevlar &&
						trackingPoint[1][boucle].y <= barY + (double)prevhau &&
						trackingPoint[1][boucle].y >= barY - (double)prevhau) {
						nbIn++;
						if (minx > cvPointFrom32f(trackingPoint[1][boucle]).x) minx = cvPointFrom32f(trackingPoint[1][boucle]).x;
						if (miny > cvPointFrom32f(trackingPoint[1][boucle]).y) miny = cvPointFrom32f(trackingPoint[1][boucle]).y;
						if (maxx < cvPointFrom32f(trackingPoint[1][boucle]).x) maxx = cvPointFrom32f(trackingPoint[1][boucle]).x;
						if (maxy < cvPointFrom32f(trackingPoint[1][boucle]).y) maxy = cvPointFrom32f(trackingPoint[1][boucle]).y;
					}
				}
			}

			nbTrackingPoint = nbIn;
			if (lar == 0) lar = (maxx - minx) / 2;
			if (hau == 0) hau = (maxy - miny) / 2;

			// Swaping
			CV_SWAP(prevGray, gray, temp);
			CV_SWAP(prevPyramid, pyramid, temp);
			CV_SWAP(trackingPoint[0], trackingPoint[1], swapPoint);

			// Too narrow
			if (lar < prevlar / 2 || hau < prevhau / 2) refresh = MAX_REFRESH;
		}

		// Enought points
		if (nbTrackingPoint >= MIN_TRACK_PT) {

			// Coordinates
			if (lar > 0 && hau > 0) {
				barx = (int)cvRound(barX);
				bary = (int)cvRound(barY);
				resx1 = barx - lar;
				resy1 = bary - hau;
				resx2 = barx + lar;
				resy2 = bary + hau;

				retour = 1;
			}
#ifdef TEST
			type = 2;
#endif
		}
		else
		if (lissage) {
			barx = (resx1 + resx2) / 2;
			bary = (resy1 + resy2) / 2;
		}

		// Distances from candidates
		if (lissage) {
			realdist = (barx - barRx) * (barx - barRx) + (bary - barRy) * (bary - barRy);
			if (barRx != barPx || barRy != barPy) {
				prevdist = (barx - barPx) * (barx - barPx) + (bary - barPy) * (bary - barPy);
			} else {
				prevdist = realdist;
			}

			provrealprevx1 = resx1;
			provrealprevy1 = resy1;
			provrealprevx2 = resx2;
			provrealprevy2 = resy2;
			provbarRx = barx;
			provbarRy = bary;

			// Too litte distance (avoid shivering)
			if (prevdist < mindist) {
				resx1 = prevx1;
				resy1 = prevy1;
				resx2 = prevx2;
				resy2 = prevy2;
				barx = barPx;
				bary = barPy;
			}
			else {

				// The previous chosen is better in case of fallback
				if (prevdist < realdist) {

					// Fallback
					if (prevdist > limitdist) {
						resx1 = prevx1;
						resy1 = prevy1;
						resx2 = prevx2;
						resy2 = prevy2;
						barx = barPx;
						bary = barPy;
					}
				}

				// The previous real is better in case of fallback
				else {

					// Fallback
					if (realdist > limitdist) {
						resx1 = realprevx1;
						resy1 = realprevy1;
						resx2 = realprevx2;
						resy2 = realprevy2;
						barx = barRx;
						bary = barRy;
					}
				}
			}
			realprevx1 = provrealprevx1;
			realprevy1 = provrealprevy1;
			realprevx2 = provrealprevx2;
			realprevy2 = provrealprevy2;
			barRx = provbarRx;
			barRy = provbarRy;

			prevx1 = resx1;
			prevy1 = resy1;
			prevx2 = resx2;
			prevy2 = resy2;
			barPx = barx;
			barPy = bary;
		}

		*x1 = resx1 * uX / frame_copy->width;
		*y1 = (resy1 + (resy2 - resy1) / 3) * uY / frame_copy->height;
		*x2 = resx2 * uX / frame_copy->width;
		*y2 = *y1;
		// lar = *x2 - *x1;
		// *x1 += lar / 3;
		// *x2 -= lar / 3;
		lar = (*x1 + *x2) / 2;
		*x1 = lar - 3;
		*x2 = lar + 3;
		// *x1 = resx1 * uX / frame_copy->width;
		// *y1 = resy1 * uY / frame_copy->height;
		// *x2 = resx2 * uX / frame_copy->width;
		// *y2 = resy2 * uY / frame_copy->height;

#ifdef TEST
		printf("----------------> %d points.\n", nbTrackingPoint);
		// display(resx1, resy1, resx2, resy2, type);
#endif
	}
	return(retour);
}

#ifdef TEST
int main() {
	int ret, x1 = 0, y1 = 0, x2 = 0, y2 = 0;
	int x1a = 0, y1a = 0, x2a = 0, y2a = 0;
	int radius;
	CvPoint center;

	init();
	cvNamedWindow( "Head track", 1 );
	while (1) {
		ret = headtrack(&x1, &y1, &x2, &y2, 1, 1, 50000);
		// printf("TRACE : %d %d/%d %d/%d\n", ret, x1, y1, x2, y2);
	}
	end();
	cvDestroyWindow("Head track");
}
#endif
