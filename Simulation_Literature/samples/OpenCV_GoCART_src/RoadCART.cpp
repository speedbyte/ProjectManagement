
#ifndef ASSERT
#include <cassert>
#define ASSERT(a) assert(a);
#endif _ASSERT

#include "cv.h"
#include "highgui.h"
#include <cmath>
#include <time.h>
#include <vfw.h>
#include "RoadCART.h"

#define CART_LEARNING
//#define BOOSTING

CvFont m_font;

// How many pixels should be used when training.  If more, subsample.  If less, not enough laser brick.
// You can reduce this to BRICK_MIN_PIXELS but no less than BRICK_MIN_PIXELS.
// Reducing this will speed up training so you want to be as low as possible and still preserve accuracy.
int TRAINING_PIXEL_COUNT = BRICK_MIN_PIXELS * 3;  // target is applied for both road and non-road 


/////////////////////////////////////////////////////////////////////////////
// Open - allocate everything we are going to use throughout the process.
/////////////////////////////////////////////////////////////////////////////
void CROADCART::Open( int width, int height )
{
	RoadSegmentation::Open( width, height );

	m_BuildTreeRequested = TRUE;
	m_LightedBrick8uC1 = cvCreateImage ( cvSize ( width, height ), 8, 1);

	m_CARTMap8uC1 = cvCreateImage ( cvSize (width, height), 8, 1);
	m_Results32fC1 = cvCreateImage ( cvSize (width, height), 32, 1);
	m_RegressionResults8uC1 = cvCreateImage ( cvSize (width, height), 8, 1);

	m_MinimalBrick8uC1 = cvCreateImage ( cvSize (width, height), 8, 1);

	m_LaserBrickMask8uC1 = cvCreateImage ( cvSize (width, height), 8, 1);
	m_input32fC3  = cvCreateImage( cvGetSize(m_output), 32, 3);
	// This is the default car mask throughout the processing.
	
	m_NonRoad8uC1 =  cvCreateImage ( cvGetSize (m_output), 8, 1);

	m_Shadow8uC1 = cvCreateImage ( cvSize (width, height), 8, 1);
	
	m_cls = NULL;
	m_Disparity32fC1 = NULL;

	// The target pixel counts are based on a 360x240 frame.
	// If working with 720x480, then the ratio should be 4
	double ratio = (width * height) / (360*240);

	m_MinPixels = (TRAINING_PIXEL_COUNT * ratio);
	m_MinBrickSize = (int) (BRICK_MIN_PIXELS * ratio);
	m_MinShadowPixels = (int) (NON_ROAD_SHADOW_TARGET_PIXELS * ratio);
	m_NonRoadMaxPixels = (int) (NON_ROAD_THRESHOLD * ratio);

	// The target cannot be lower than the minimum number of pixels in the brick!
	ASSERT( TRAINING_PIXEL_COUNT >= BRICK_MIN_PIXELS );

	m_PixelsFound = 0.0;
	m_ZeroPixelFrames = 0;
	m_TrainingCount = 0;
	m_PredictBrickFailures = 0;
	m_NonRoadFailures = 0;
	m_RoadBuildCount = 0;
	m_NotEnoughBrickFailures = 0;
	m_frameNumber = 0;
	m_GoodPredictions = 0;
	m_NonRoadMaskAvailable = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Close - delete everything we have allocated.
/////////////////////////////////////////////////////////////////////////////
void CROADCART::Close( )
{
	RoadSegmentation::Close( );

	cvReleaseImage (&m_input32fC3);
	cvReleaseImage (&m_RegressionResults8uC1);
	cvReleaseImage (&m_output);
	cvReleaseImage (&m_LightedBrick8uC1);
	cvReleaseImage (&m_Results32fC1);
	cvReleaseImage (&m_MinimalBrick8uC1);
	cvReleaseImage (&m_CARTMap8uC1);
	cvReleaseImage (&m_LaserBrickMask8uC1);
	cvReleaseImage (&m_NonRoad8uC1);
	cvReleaseImage (&m_Shadow8uC1);
}

/////////////////////////////////////////////////////////////////////////////
// NormalizeTo255 - normalize the image to 0 to 255.  We do this a lot.
/////////////////////////////////////////////////////////////////////////////
void CROADCART::NormalizeTo255 ( CvArr *in, CvArr *out )
{
 	double InputMin, InputMax;
	cvMinMaxLoc( in, &InputMin, &InputMax );
	cvConvertScale ( in, in, 1, -InputMin);
	cvConvertScale ( in, out, 255.0 / (InputMax - InputMin), 0); 
}

/////////////////////////////////////////////////////////////////////////////
// CROADCART::CROADCART - Constructor
/////////////////////////////////////////////////////////////////////////////
CROADCART::CROADCART(): RoadSegmentation()
{
	cvInitFont( &m_font, CV_FONT_VECTOR0, .5, .5, 0.0, 1 );
	m_BuildFreq = BUILD_FREQ;
}

/////////////////////////////////////////////////////////////////////////////
// CROADCART::~CROADCART - Destructor
/////////////////////////////////////////////////////////////////////////////
CROADCART::~CROADCART()
{
	Close();
}




 
///////////////////////////////////////////////////////////////////////////////////////////////////
// HighlightResults - highlight the mask in the full size image.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CROADCART::HighlightResults ( IplImage *mask ) 
{
	if (m_Debug ) {
		cvZero ( m_output );
		cvCopy ( m_input, m_output, mask);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// WriteMsg - place a message in the image.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CROADCART::WriteMsg(char* message)
{
	// tell them no valid results.
	// char NewText[1000];
	// sprintf ( NewText, message );
	CvPoint pt1;
	pt1.x = m_output->width / 5;
	pt1.y = m_output->height / 5;
	cvPutText( m_output, message, pt1, &m_font, CV_RGB(255, 255, 255));
}
#if 1
///////////////////////////////////////////////////////////////////////////////////////////////////
// NextCARTdata - computes mean and std for r, g, and b around the pixels.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CROADCART::NextCARTdata ( float *out, float *in )
{
	memcpy ( (void *) out, (const void *) in, FEATURE_COUNT * sizeof (float) );
}
#else
///////////////////////////////////////////////////////////////////////////////////////////////////
// NextCARTdata - computes mean and std for r, g, and b around the pixels.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CROADCART::NextCARTdata ( float *fptr, int x, int y, int BrickSize )
{
	double mean, std; // , InputMin, InputMax; 
	CvRect tmpR;
 
	tmpR = cvRect ( x, y, BrickSize, BrickSize );
	int index = 0;

	// Red mean and std
	cvSetImageROI ( m_red8uC1, tmpR);
	cvMean_StdDev (m_red8uC1, &mean, &std );	

	fptr[index++] = (float) mean;
	fptr[index++] = (float) std;

	//cvMinMaxLoc ( m_red8uC1, &InputMin, &InputMax );  // Add these commented lines to test a 12 feature mix.
	cvResetImageROI ( m_red8uC1 );

	//fptr[index++] = (float) InputMin;
	//fptr[index++] = (float) InputMax;

	// Green mean and std
	cvSetImageROI ( m_grn8uC1, tmpR);
	cvMean_StdDev (m_grn8uC1, &mean, &std );	

	fptr[index++] = (float) mean;
	fptr[index++] = (float) std;

	//cvMinMaxLoc ( m_grn8uC1, &InputMin, &InputMax );
	cvResetImageROI ( m_grn8uC1 );

	//fptr[index++] = (float) InputMin;
	//fptr[index++] = (float) InputMax;

	// Blue mean and std
	cvSetImageROI ( m_blu8uC1, tmpR);
	cvMean_StdDev (m_blu8uC1, &mean, &std );	

	fptr[index++] = (float) mean;
	fptr[index++] = (float) std;

	//cvMinMaxLoc ( m_blu8uC1, &InputMin, &InputMax );
	cvResetImageROI ( m_blu8uC1 );

	//fptr[index++] = (float) InputMin;
	//fptr[index++] = (float) InputMax;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// NextCARTdata - computes mean and std for r, g, and b around the pixels.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CROADCART::NextCARTdata ( float *fptr, int x, int y, int BrickSize )
{
	double mean, std; 
	CvRect tmpR;

	tmpR = cvRect ( x, y, BrickSize, BrickSize );
	int index = 0;

	// Red mean and std
	cvSetImageROI ( m_red8uC1, tmpR);
	cvMean_StdDev (m_red8uC1, &mean, &std );	

	fptr[index++] = (float) mean;
	fptr[index++] = (float) std;

	double InputMin, InputMax; 
	cvMinMaxLoc ( m_red8uC1, &InputMin, &InputMax );  // Add these commented lines to test a 12 feature mix.
	cvResetImageROI ( m_red8uC1 );

	fptr[index++] = (float) InputMin;
	fptr[index++] = (float) InputMax;

	// Green mean and std
	cvSetImageROI ( m_grn8uC1, tmpR);
	cvMean_StdDev (m_grn8uC1, &mean, &std );	

	fptr[index++] = (float) mean;
	fptr[index++] = (float) std;

	cvMinMaxLoc ( m_grn8uC1, &InputMin, &InputMax );
	cvResetImageROI ( m_grn8uC1 );

	fptr[index++] = (float) InputMin;
	fptr[index++] = (float) InputMax;

	// Blue mean and std
	cvSetImageROI ( m_blu8uC1, tmpR);
	cvMean_StdDev (m_blu8uC1, &mean, &std );	

	fptr[index++] = (float) mean;
	fptr[index++] = (float) std;

	cvMinMaxLoc ( m_blu8uC1, &InputMin, &InputMax );
	cvResetImageROI ( m_blu8uC1 );

	fptr[index++] = (float) InputMin;
	fptr[index++] = (float) InputMax;
}
#endif
void CROADCART::TestLaserInShadow()
{
	// If only a little of the brick is lite, then forget it.  We don't know anything.
	double RealCount = cvCountNonZero ( m_LightedBrick8uC1 );
	// if we don't have at least x pixels of road, we don't have much to go on.
	if (RealCount < m_MinBrickSize ) 
		m_EnoughBrick = FALSE;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// BrickShadows - eliminate some dark regions from the brick - to create better training data.
// Can this be eliminated?  I think so.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CROADCART::BrickShadows( ) 
{
	// Just isolate the non-shadow of the brick.
	cvCopy (m_LaserBrickMask8uC1, m_LightedBrick8uC1 );
	cvSet (m_LightedBrick8uC1, cvScalar (0), m_Shadow8uC1 );
	cvAnd ( m_LightedBrick8uC1, m_CarMask8uC1, m_LightedBrick8uC1 );

	HighlightResults ( m_LightedBrick8uC1 );
	//cvCopy ( m_input, m_output);
	//cvSubS ( m_output, CV_RGB ( 50, 50, 50), m_output, m_LaserBrickMask8uC1 );
	if ( m_Debug ) {
		char NewText[1000];
		sprintf ( NewText, "%d", cvCountNonZero ( m_LightedBrick8uC1 )  );
		// WriteMsg ( NewText );
	}
}
void CROADCART::ShowDistanceLevels()
{
	IplImage *levels32fC1 = cvCreateImage( cvGetSize(m_input), 32, 1);
	IplImage *Mask8uC1 = cvCreateImage( cvGetSize(m_input), 8, 1);
	IplImage *out8uC1 = cvCreateImage( cvGetSize(m_input), 8, 1);
	IplImage *tmp8uC1 = cvCreateImage( cvGetSize(m_input), 8, 1);
	IplImage *tmp32fC1 = cvCreateImage( cvGetSize(m_input), 32, 1);

	int y; double InputMin, InputMax;
	int BrickSize = 1;
#define MIN_DISTANCE ( 5000 ) // minimum 5 meter distance 
	// Find the range of real distance (excluding distance less than 5 m.)
	cvThreshold ( m_Disparity32fC1, tmp8uC1, MIN_DISTANCE, 255, CV_THRESH_BINARY );
	cvMinMaxLoc ( m_Disparity32fC1, &InputMin, &InputMax, NULL, NULL, tmp8uC1 );

	CvRect tmpR;
	tmpR.width = m_Disparity32fC1->width;
	tmpR.height = BrickSize;
	tmpR.x = 0;

	cvZero ( tmp32fC1 );
	cvCopy ( m_Disparity32fC1, tmp32fC1, m_Results8uC1 );
	cvThreshold ( tmp32fC1, Mask8uC1, 0, 255, CV_THRESH_BINARY );

	// This will set the minimum distance throughout the image.
	cvSet ( levels32fC1, cvScalar ( InputMin ));

	double MaxDist = 0.0;

	// Find the top 10 distances and get a mean for that.
#define DIST_COUNT ( 10 )
	CvMat *Dist = cvCreateMat( 1, DIST_COUNT, CV_32FC1 );
	CvPoint MinLoc, MaxLoc;
	float *fptr = (float *) tmp32fC1->imageData;
	double OldMaxDist;
	for ( y = 0; y < DIST_COUNT; y++) {
		cvMinMaxLoc( tmp32fC1, &InputMin, &InputMax, &MinLoc, &MaxLoc );
		Dist->data.fl [ y ] = ( float ) InputMax;
		if ( y == 0) OldMaxDist = InputMax;
		// eliminate that point from the running.
		fptr [ MaxLoc.x + MaxLoc.y * tmp32fC1->widthStep / 4 ] = 0.0;
	}
	MaxDist = cvMean ( Dist );

	cvZero ( out8uC1 );
	cvCopy ( Mask8uC1, out8uC1, Mask8uC1 );
	cvCvtColor ( out8uC1, m_output, CV_GRAY2BGR );
	char NewText[1000];
	sprintf ( NewText, "Max = %.01f meters", MaxDist / 1000.0 );
	WriteMsg ( NewText );

	cvReleaseMat ( &Dist );
	cvReleaseImage ( &tmp32fC1 );
	cvReleaseImage ( &tmp8uC1 );
	cvReleaseImage ( &out8uC1 );
	cvReleaseImage ( &Mask8uC1 );
	cvReleaseImage ( &levels32fC1 );
}
void CROADCART::ShowDisparity()
{
	IplImage *tmp8uC1 = cvCreateImage( cvGetSize(m_input), 8, 1);
	IplImage *tmp32fC1 = cvCreateImage( cvGetSize(m_input), 32, 1);

	cvZero ( tmp32fC1 );
	cvCopy ( m_Disparity32fC1, tmp32fC1, m_Results8uC1 );
	// cvThreshold ( tmp32fC1, tmp8uC1, 0, 255, CV_THRESH_BINARY );
	NormalizeTo255 ( tmp32fC1, tmp8uC1 );

	cvCvtColor ( tmp8uC1, m_output, CV_GRAY2BGR );

	cvReleaseImage ( &tmp32fC1 );
	cvReleaseImage ( &tmp8uC1 );
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// SetDefaultBrick - define a small region right in front of the car to guarantee that we will have something to define road.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CROADCART::SetDefaultBrick()
{
	// This creates a minimal brick that is right in front of the car.
	// If we are training and the brick is not present, it is not good.
	// The safest thing is to always have a small brick in front of the car.
	m_MinLeftX = (int) (m_MinimalBrick8uC1->width / 3);

	m_MinLeftY = (int) (m_MinimalBrick8uC1->height * 0.8 - m_MinimalBrick8uC1->height * m_CarHoodTop);

	m_MinRightX = (int) (m_MinimalBrick8uC1->width * 2 / 3);
	m_MinRightY = (int) (m_MinimalBrick8uC1->height - m_MinimalBrick8uC1->height * m_CarHoodTop);

#define BRICK_BOTTOM_FRACTION (8)

	CreateMask ( cvPoint (m_MinLeftX, m_MinLeftY), 
				 cvPoint (m_MinRightX, m_MinLeftY), 
				 cvPoint (m_input->width * (BRICK_BOTTOM_FRACTION - 1) / BRICK_BOTTOM_FRACTION, m_MinRightY), 
				 cvPoint (m_input->width / BRICK_BOTTOM_FRACTION, m_MinRightY), m_MinimalBrick8uC1);
}
/*
///////////////////////////////////////////////////////////////////////////////
// LaserInputMask - This builds the laser brick from the 4 points provided from
// the laser code.  
///////////////////////////////////////////////////////////////////////////////
void CROADCART::LaserInputMask(CvSeq *NextSeq)
{
	// input bricks are constructed assuming a 320x240 image but the videos are 720x480
	// Remapping them to m_input size is to multiply times 2 and then the ratio of m_input->height / 480.
	float factor = (float) 2.0 * m_input->height / 480.0; 

	CvSeqReader reader;
	cvStartReadSeq( NextSeq, &reader );

	// We are only expecting to get 4 points here!
	ASSERT ( NextSeq->total == 4); 

	// Determine the center point of the laser brick
	m_CenterPoint.x = m_CenterPoint.y = 0;
	int BadBrick = 0, i; 
	for( i = 0; i < NextSeq->total; i++ ) {
		CV_READ_SEQ_ELEM( m_BrickPt[i], reader );
		if (m_BrickPt[i].x < 0 || m_BrickPt[i].y < 0 ) BadBrick = TRUE;
	 	m_BrickPt[i].x = (int) (m_BrickPt[i].x * factor);
		m_BrickPt[i].y = (int) (m_BrickPt[i].y * factor);
		m_CenterPoint.x += m_BrickPt[i].x;
		m_CenterPoint.y += m_BrickPt[i].y;
	}
	m_CenterPoint.x = (m_CenterPoint.x + NextSeq->total/2) / NextSeq->total;
	m_CenterPoint.y = (m_CenterPoint.y + NextSeq->total/2) / NextSeq->total;
	ValidatePoint ( &m_CenterPoint );

	// Determine the min/max point of the laser brick in X and Y direction:
	// m_MinX, m_MaxX (global) and MinY, MaxY (local)
	// MinY_i is the index in m_BrickPt[] for the point with the smallest y position
	// MinY2_i is the index in m_BrickPt[] for the point with the second smallest y position
	m_MinX=m_input->width;
	m_MaxX = 0;
	int MinY = m_input->height , MaxY = 0, MinY_i = -1, MinY2_i = -1;	
	int rainer=0;
	m_SaveMinY = 0; // default - indicates this can't be good!
	for ( i = 0; i < 4; i++)  {
		if (m_MinX > m_BrickPt[i].x) m_MinX = m_BrickPt[i].x;  // Some default values for the left and right.
		if (m_MaxX < m_BrickPt[i].x) m_MaxX = m_BrickPt[i].x;
		if (MinY > m_BrickPt[i].y) { MinY = m_BrickPt[i].y; MinY_i = i;	}
		if (MaxY < m_BrickPt[i].y)   MaxY = m_BrickPt[i].y;
	}

	// Keep the highest point on the image (actually it is the smallest.)
	m_PeakY = MinY;  // This is as low as non-road should have to go.
	if (m_PeakY < 0 ) m_PeakY = 0;  // validate the y value!  It is ocassionally wrong.
	if (m_PeakY > m_input->height) m_PeakY = m_input->height - 1;

	// Find the second smallest y and make that the top of the laser brick
	m_Len = 0;
	if (MinY_i >= 0) {
		int MinY2 = m_input->height;
		for ( i = 0; i < 4; i++)  {
			if (i != MinY_i) {
				if (MinY2 > m_BrickPt[i].y) {
					MinY2 = m_BrickPt[i].y;
					MinY2_i = i;
				}
			}
		}
		//MinY2 = MinY2 + 10; // reduce the top of the brick a little - too aggressive!
		//m_BrickPt[MinY2_i].y = MinY2;
	
		// And narrow the top of the trapazoid so less non-road is included.
		if ( MinY2_i >= 0 ) {
			m_MidX = (m_BrickPt[MinY_i].x + m_BrickPt[MinY2_i].x ) / 2;
			m_Len = abs ( m_BrickPt[MinY_i].x - m_BrickPt[MinY2_i].x );

#ifdef TRIM_BRICK_TOP
			// These lines to flatten the top of the laser brick.
			if ( m_BrickPt[MinY_i].x < m_BrickPt[MinY2_i].x ) {
				m_BrickPt[MinY_i].x = m_MidX - m_Len / 4;
				m_BrickPt[MinY2_i].x = m_MidX + m_Len / 4;
			} else {
				m_BrickPt[MinY_i].x = m_MidX + m_Len / 4;
				m_BrickPt[MinY2_i].x = m_MidX - m_Len / 4;
			}
			m_BrickPt[MinY_i].y = MinY2; // make the top line parallel to the bottom
#endif
			m_SaveMinY = MinY2; // save this to make the regression run faster!
		}
		if (m_MidX < 0) m_MidX = 0;
		if (m_MidX >= m_input->width) m_MidX = m_input->width - 1;

		// Set m_MinX, m_MaxX to the values of the locations of the two top corners of the laser brick.
		// Discard all prior calculations.
		if ( m_BrickPt[MinY_i].x < m_BrickPt[MinY2_i].x ) {
			m_MinX = m_BrickPt[MinY_i].x;
			m_MaxX = m_BrickPt[MinY2_i].x;
		} else {
			m_MinX = m_BrickPt[MinY2_i].x;
			m_MaxX = m_BrickPt[MinY_i].x;
		}
	} 

	// This creates the official laser brick mask from the laser code.
	CreateMask ( m_BrickPt, m_LaserBrickMask8uC1);

	// If there is no real laser brick just use the default brick in front of the car hood.
	if (m_UseMinimalBrick) {
		m_MidX = (m_MinLeftX + m_MinRightX ) / 2;
		m_SaveMinY = m_MinLeftY;
		m_Len = m_MinRightX - m_MinLeftX;
		m_PeakY = m_SaveMinY;
		cvOr ( m_LaserBrickMask8uC1, m_MinimalBrick8uC1, m_LaserBrickMask8uC1 );
		m_CenterPoint.x = (int) m_MidX;
		m_CenterPoint.y = (int) m_SaveMinY;
	}

	if ( m_Debug ) {
		cvSubS ( m_input, CV_RGB ( 50, 50, 50 ), m_output );
		cvCopy ( m_input, m_output, m_LaserBrickMask8uC1);
	}
}
*/
///////////////////////////////////////////////////////////////////////////////////////////////////
// SkipPercentMask - how much of this mask can we skip to get the minimum pixels we desire to test on?
///////////////////////////////////////////////////////////////////////////////////////////////////
double CROADCART::SkipPercentMask ( IplImage *mask )
{
	int count = cvCountNonZero ( mask );
	double PercentToSkip;
	if (count > m_MinPixels ) {
		PercentToSkip = 1 - (double) m_MinPixels / (double) count;
	} else { 
		PercentToSkip = 0.0;
	}
	return PercentToSkip;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// LoadNextImage - loads the next image in the video into the class.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CROADCART::LoadNextImage ( IplImage *nextImage )
{
	m_frameNumber++;
	{ cvFlip(nextImage); nextImage->origin = 0; }

	// Assume the best about the laser brick - that it will be good.
	m_EnoughBrick = TRUE;
	if (m_input == (IplImage *) 0) {
		m_input = cvCloneImage ( nextImage ); 
		SetDefaultBrick();
	}

	cvZero ( m_input );
	cvCopy ( nextImage, m_input, m_CarMask8uC1 );

	cvCopy ( m_input, m_output);

	// We will be using the 32-bit edition 
	cvConvertScale ( m_input, m_input32fC3, 1, 0);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// Prepare the map of the training and testing data on each image.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CROADCART::PrepareAreaToTrainTest()
{
	if (m_EnoughBrick == FALSE) return; // no sense doing this when there is not enough road pixels.
	// Prepare the training data for this image.
	// red_mean,red_std, green_mean,green_std, blue_mean,blue_std ==> Road
	cvZero ( m_CARTMap8uC1 );  // default is CART_DONT_CARE
	double NRPercentToSkip = SkipPercentMask ( m_NonRoad8uC1 );
	double RPercentToSkip  = SkipPercentMask ( m_LightedBrick8uC1 );
	int NonRoadCount = 0, RoadCount = 0;
	for (int y = 0 ; y < m_CARTMap8uC1->height; y++) {
		for (int x = 0; x < m_CARTMap8uC1->width; x++) {

			m_CARTMap8uC1->imageData[x + y * m_CARTMap8uC1->widthStep] = CART_UNKNOWN; 

			// Now determine if this is road or non-road.
			if ( m_NonRoad8uC1->imageData [ x + y * m_CARTMap8uC1->widthStep] != 0) {
				if ( rand() / (double) RAND_MAX > NRPercentToSkip) {
					NonRoadCount++;
					m_CARTMap8uC1->imageData[x + y * m_CARTMap8uC1->widthStep] = CART_NON_ROAD; 
				}
			} else {
				// We are sampling the laser brick to determine if it is predicting ok.				
				if ( m_LightedBrick8uC1->imageData [ x + y * m_LightedBrick8uC1->widthStep] != 0) {	
					if ( rand() / (double) RAND_MAX > RPercentToSkip) {
						RoadCount++;
						m_CARTMap8uC1->imageData[x + y * m_CARTMap8uC1->widthStep] = CART_ROAD; 
					}
				}
			}
		}
	}
//	for (int i=0 ; i<100 ; i++) m_CARTMap8uC1->imageData[i + i * m_CARTMap8uC1->widthStep]=0;
//	cvThreshold ( m_CARTMap8uC1, m_RegressionResults8uC1, 0, 255, CV_THRESH_BINARY );
//	cvNamedWindow("test",CV_WINDOW_AUTOSIZE); cvShowImage("test", m_RegressionResults8uC1);

	m_TrainCount = RoadCount + NonRoadCount;
	if (m_TrainCount <= 0) m_EnoughBrick = FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenCVCartTrain - train with the opposite of road and the laser brick to allow predicting road and non-road.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CROADCART::OpenCVCartTrain()
{
	int x, y;

	// No sense training if the laser brick is in shadow.
	// is the brick all in shadow?  Then forget training anything.  Or are the counts too low?  We are confused.
	TestLaserInShadow();
	if ( m_EnoughBrick == FALSE ) return;

	CvMat*  train_data  = cvCreateMat( m_TrainCount, FEATURE_COUNT, CV_32FC1 );
	CvMat*  class_data  = cvCreateMat( m_TrainCount, 1, CV_32FC1 );
	CvMat*  missed_mask = cvCreateMat( m_TrainCount, FEATURE_COUNT, CV_8UC1 );
	CvMat*  type_mask   = cvCreateMat( FEATURE_COUNT + 1, 1, CV_8UC1 );

	cvZero ( missed_mask ); // we have no missing data!

	int trainIndex;
	float *out = train_data->data.fl;
	float *in  = (float *) m_input32fC3->imageData;
	int widthStep = m_input32fC3->widthStep >> 2;
	for ( y = 0, trainIndex = 0 ; y < m_CARTMap8uC1->height; y++) {
		for ( x = 0; x < m_CARTMap8uC1->width; x++) {
			switch (m_CARTMap8uC1->imageData[x + y * m_CARTMap8uC1->widthStep]) {
				case CART_DONT_CARE: // something we don't care about - lower than SaveMinY, for instance.
					break;
				case CART_UNKNOWN: // This is what we want to predict later!
					break;
				case CART_ROAD: // Road
					CV_MAT_ELEM(*class_data, float, trainIndex, 0) = 1.0;
					NextCARTdata ( &out[trainIndex * FEATURE_COUNT], &in[x * 3 + y * widthStep] );
					trainIndex++;
					break;
				case CART_NON_ROAD: // Non-Road
//#if defined ( CART_LEARNING )
//					CV_MAT_ELEM(*class_data, float, trainIndex, 0) = 0.0;
//#elif defined ( BOOSTING )
					CV_MAT_ELEM(*class_data, float, trainIndex, 0) = -1.0;
//#endif
					NextCARTdata ( &out[trainIndex * FEATURE_COUNT], &in[x * 3 + y * widthStep] );
					trainIndex++;
					break;
				default:
					ASSERT(FALSE);
					break;
			}
		}
	}
	/* set features and response types */
	for (int i = 0; i <= FEATURE_COUNT; i++ ) {
		CV_MAT_ELEM(*type_mask, uchar, i, 0) = CV_VAR_NUMERICAL; 
	}

	// if there was a previous model, we can delete it now.
	cvReleaseStatModel(&m_cls);

#if defined ( CART_LEARNING )
    CvCARTClassifierTrainParams     param;
    cvSetDefaultParamCARTClassifier((CvClassifierTrainParams*)&param);
    param.num_competitors = 3;
    param.num_surrogates = 3;
    param.cross_validation_folds = 6;

	/* train cart */
	m_cls = cvCreateCARTClassifier( train_data, CV_ROW_SAMPLE, class_data, (CvClassifierTrainParams*)&param, 
									0, 0, type_mask, missed_mask );
#elif defined ( BOOSTING )
    CvBtClassifierTrainParams     param;
	memset ( &param, 1, sizeof(param) );
//    param.boost_type = CV_BT_GENTLE;
    param.boost_type = CV_BT_DISCRETE;
	param.num_iter = 50;
	param.infl_trim_rate = 0.99;
	param.num_splits = 2;
	m_cls = cvCreateBtClassifier ( train_data, CV_ROW_SAMPLE, class_data, (CvStatModelParams*)&param, 
									0, 0, 0, 0 );
	// cvSetParamValue ( m_cls, NULL, CV_CART_COMPETITOR_NUM, NULL, 0 );
#endif

	m_TrainingCount++;

	m_BuildJustCompleted = TRUE;
	m_BuildTreeRequested = FALSE;
	m_RoadBuildCount = 0;

	cvReleaseMat(&train_data);
	cvReleaseMat(&class_data);
	cvReleaseMat(&missed_mask);
	cvReleaseMat(&type_mask);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenCVCartTest - predict whether each pixel is road or non-road.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CROADCART::OpenCVCartTest()
{
	if ( m_EnoughBrick == TRUE ) {
		int x, y;
		CvMat* vec = cvCreateMatHeader(1, FEATURE_COUNT, CV_32FC1);
		cvZero ( m_Results8uC1 );
		cvZero ( m_Results32fC1 );
		float *fptr = (float *) m_Results32fC1->imageData;
		float *in   = (float *) m_input32fC3->imageData;
		const int widthStep = m_input32fC3->widthStep >> 2;
		const int resultsWidth = m_Results32fC1->widthStep >> 2;
		for ( y = 0 ; y < m_CARTMap8uC1->height; y++ ) {
			for ( x = 0; x < m_CARTMap8uC1->width; x++) {
//				if ( m_Shadow8uC1->imageData [ x + y * m_Shadow8uC1->widthStep] == 0) {
//					if ( m_CARTMap8uC1->imageData[x + y * m_CARTMap8uC1->widthStep] == CART_UNKNOWN ||
//						m_CARTMap8uC1->imageData[x + y * m_CARTMap8uC1->widthStep] == CART_ROAD ) {
							vec->data.fl = &in[x * 3 + y * widthStep]; // 3 is the number of channels!
							float nextBrick = cvStatModelPredict( m_cls, vec );
							fptr[ x + y * resultsWidth ] = nextBrick; // keep it to compute how well we predict the brick
							if ( nextBrick  < classificationThreshold ) // close to 1 or 0?
								m_Results8uC1->imageData[ x + y * m_Results8uC1->widthStep ] = 0;
							else
								m_Results8uC1->imageData[ x + y * m_Results8uC1->widthStep ] = -6; // -6 == 250 (unsigned char)
//						}
//				}
			}
		}

		// After a while, rebuild the tree on the next iteration.
		if ( ++m_RoadBuildCount  >= m_BuildFreq ) 
			m_BuildTreeRequested = TRUE;

		cvReleaseMatHeader ( &vec );
	} 
	// Mark where the laser brick was in this image.
	if (m_Debug) { 
		cvSubS ( m_input,  CV_RGB ( 50, 50, 50 ), m_output, m_LaserBrickMask8uC1 );
		cvSubS ( m_output, CV_RGB ( 50, 50, 50 ), m_output, m_NonRoad8uC1 );
	}
}
void CROADCART::ShowCARTMap()
{
	if (m_Debug) {
		IplImage *tmp8uC1 = cvCreateImage( cvGetSize(m_input), 8, 1);
		cvConvertScale ( m_CARTMap8uC1, tmp8uC1, 255.0 / CART_NON_ROAD, -1);
		cvCvtColor ( tmp8uC1, m_output, CV_GRAY2BGR );

		char NewText[1000];
		sprintf ( NewText, "target %d", m_MinPixels );
		WriteMsg ( NewText );
		cvReleaseImage ( &tmp8uC1 );
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// RemoveShadow - anything lower than SHADOW_MAX is shadow.  Simple but effective way to detect shadow.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CROADCART::RemoveShadow()
{
	cvCvtColor ( m_input, m_Shadow8uC1, CV_BGR2GRAY );
	cvThreshold ( m_Shadow8uC1, m_Shadow8uC1, SHADOW_MAX, 255, CV_THRESH_BINARY_INV );

	// Expand the shadow to capture penumbra (a confusing mix of pixels.)
	// This proved to be helpful in removing mixed colors pixels from the training set.
	// cvDilate ( m_Shadow8uC1, m_Shadow8uC1, NULL, SHADOW_DILATE ); 
	
	HighlightResults ( m_Shadow8uC1 );
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// BuildNonRoad - Show the area that includes distant hills and sky - definitely non-road.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CROADCART::BuildNonRoad()
{
	IplImage *mask8uC1 = cvCreateImage( cvGetSize(m_input), 8, 1);
	CvRect tmpr;
	// if we had no success predicting the road on the last frame or if successful, with only meager results.
	// Use the automated segmentation of non-road.
	if (m_NonRoadMaskAvailable == TRUE ) {
		// If we have a successful mask from the previous frame, we can use it to predict non-road in this frame.
		cvNot ( m_RegressionResults8uC1, mask8uC1 );
		cvSet ( mask8uC1, cvScalar ( 0 ), m_LaserBrickMask8uC1 );
	}

	int count, y, NextCount, FirstShadowY = 0;
	tmpr.x = 0;
	tmpr.width = m_input->width;
	tmpr.height = 1;
	// Find a top percentage that includes some shadow
	for ( y = m_input->height / 10, count = 0; y < m_input->height / 2; y++) {
		tmpr.y = y;
		cvSetImageROI ( m_Shadow8uC1, tmpr );
		NextCount = cvCountNonZero ( m_Shadow8uC1 );
		count = count + NextCount; 
		// We will extend the mask all the way across at the first shadow pixels.
		if (count > 0 && FirstShadowY == 0) 
			FirstShadowY = y;
		cvResetImageROI ( m_Shadow8uC1 );
		// if there is enough shadow, we have a good cross-section of non-road
		if (count > m_MinShadowPixels) break;
		// The top of the brick is also the end!
		if (y >= m_PeakY) 
			break;
	}

	// Create a mask that represents sky and the top of the land mask.
	CreateMask (cvPoint ( 0, 0 ), 
				cvPoint ( m_input->width, 0 ), 
				cvPoint ( m_input->width, y ), 
				cvPoint ( 0, y), m_NonRoad8uC1); 

	// if we have a top of the brick, then use it.
	if (  y < m_PeakY ) {
#define FRACTION ( 10 )
		int XLeft = m_input->width / FRACTION;
		int XRight = m_input->width * (FRACTION - 1) / FRACTION;
		// Add a little non-road on the left side of the image.
		tmpr.x = 0;
		tmpr.y = y;
		tmpr.width = XLeft;
		tmpr.height = m_PeakY - y;
		cvSetImageROI ( m_NonRoad8uC1, tmpr);
		cvSet ( m_NonRoad8uC1, cvScalar ( 255 ));
		cvResetImageROI ( m_NonRoad8uC1 );

		// Add a little non-road on the right side of the image.
		tmpr.x = XRight;
		cvSetImageROI ( m_NonRoad8uC1, tmpr);
		cvSet ( m_NonRoad8uC1, cvScalar ( 255 ));
		cvResetImageROI ( m_NonRoad8uC1 );

		// Now zero out the section of non-road right ahead of the laser brick top.  That often
		// eliminates the road in the distance from the non-road mask.
		if (m_Len > 0 ) {
			int len = m_MaxX - m_MinX;
			if (len > 30) len = 30;
			if (len < 5) len = 5;
			tmpr.x = m_MidX - len;
			if (tmpr.x < XLeft) tmpr.x = XLeft;
			tmpr.y = 0;
			tmpr.width = len * 2;
			if (tmpr.x + tmpr.width > XRight)
				tmpr.width = XRight - tmpr.x;
			if (tmpr.width <=0 ) tmpr.width = 5;
			tmpr.height = m_PeakY;
			cvSetImageROI ( m_NonRoad8uC1, tmpr);
			cvSet ( m_NonRoad8uC1, cvScalar ( 0 ));
			cvResetImageROI ( m_NonRoad8uC1 );
		}
	}

	// The mask extends across the whole sky at the start of the first shadow.
	if (FirstShadowY == 0 ) 
		FirstShadowY = m_PeakY;
	tmpr.x = 0;
	tmpr.y = 0;
	tmpr.width = m_input->width;
	tmpr.height = FirstShadowY;
	if (tmpr.height == 0) tmpr.height = 1;
	cvSetImageROI(m_NonRoad8uC1, tmpr);
	cvSet ( m_NonRoad8uC1, cvScalar ( 255 ));
	cvResetImageROI( m_NonRoad8uC1 );

	// If the non-road estimate is good - or it into the results to fine-tune the estimate of non-road.
	if (m_NonRoadMaskAvailable == TRUE ) 
		cvOr ( mask8uC1, m_NonRoad8uC1, m_NonRoad8uC1 );

	// This overrides the or of the automated brick so that we can test with old videos that have no laser brick
	if ( m_UseMinimalBrick && m_NonRoadMaskAvailable == TRUE  ) 
		cvCopy ( mask8uC1, m_NonRoad8uC1 );


	HighlightResults ( m_NonRoad8uC1 );
	cvReleaseImage ( &mask8uC1 );
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// FinalDecision - make the decision on whether the road is correctly identified.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CROADCART::FinalDecision()
{
	cvResize ( m_input, m_output );
	IplImage *tmp8uC1  = cvCreateImage( cvGetSize(m_input), 8,  1);
	IplImage *tmp32fC1 = cvCreateImage( cvGetSize(m_input), 32, 1);
	cvZero ( tmp32fC1 );
	cvCopy ( m_Results32fC1, tmp32fC1, m_LightedBrick8uC1);
	// This will measure how well we predicted the brick we started with.
	// If too low, then we are not predicting well.
	cvAnd ( m_CARTMap8uC1, m_LightedBrick8uC1, m_CARTMap8uC1);
	m_RoadConfidence = cvMean ( tmp32fC1, m_CARTMap8uC1 );
	if (m_RoadConfidence < 0.0) m_RoadConfidence = 0.0;

	m_NonRoadMaskAvailable = FALSE;
	if ( m_EnoughBrick == TRUE ) {
		// If confidence is good enough, fill in the road pixels.
		if ( m_RoadConfidence >= m_ConfidenceThreshold ) {
			// Is there much in the non-road area?  If so, we can just forget this image.
			cvAnd ( m_Results8uC1, m_NonRoad8uC1, tmp8uC1 );
			int count = cvCountNonZero ( tmp8uC1 );
			int MinPixels = m_NonRoadMaxPixels;
			// We get to use more pixels when using the laser brick because the non-road is 
			// so large when the laser brick is not defined (the minimal brick is very low in the image.)
			if (m_UseMinimalBrick) 
				MinPixels = MinPixels * 4;

			if (m_Debug) {
				char NewText[1000];
				sprintf ( NewText, "t=%d c=%d", MinPixels, count );
				WriteMsg ( NewText );
			}

			// If we can pass this first threshold, flood fill and check the second threshold.
			if ( count < MinPixels) {
				// Save this for the next frame
				cvThreshold ( m_Results8uC1, m_RegressionResults8uC1, 249, 255, CV_THRESH_BINARY );
				CvRect tmpr;
				tmpr.x = 0;
				tmpr.width = m_input->width;

				tmpr.y = m_PeakY;
				tmpr.height = m_input->height - tmpr.y;
				cvSetImageROI ( m_RegressionResults8uC1, tmpr );
				cvSet (m_RegressionResults8uC1, cvScalar ( 255 ) ); // This will be negated before used
				cvResetImageROI (m_RegressionResults8uC1 );

				m_NonRoadMaskAvailable = TRUE;

				m_GoodPredictions++;
				FloodFillBrick ( m_Results8uC1 );
				cvCopy ( m_LaserBrickMask8uC1, m_Results8uC1, m_LaserBrickMask8uC1);
				cvAnd ( m_Results8uC1, m_CarMask8uC1, m_Results8uC1);

				cvSet ( m_output, CV_RGB ( 255, 255, 255 ), m_Results8uC1 );
			} else {
				m_NonRoadFailures++;  // Count how many failures due to too much road in the non-road definition.
			}
		} else {
			m_PredictBrickFailures++; // we failed to predict the brick!
		}
	} else {
		// A black flag means that shadow is covering the brick or too little of it is available.  
		// Can't do anything if that happens.
		m_NotEnoughBrickFailures++;
	} 

	if (m_NonRoadMaskAvailable == FALSE) {
		cvRectangle( m_output, cvPoint(0,0),
					cvPoint(m_output->width / 10, m_output->height / 10),
					CV_RGB(0,0,0), -1, 1, 0 );
		cvZero ( m_Results8uC1 );
		m_ZeroPixelFrames++; // bump the frames with zero pixels.
		m_BuildTreeRequested = TRUE;
	}


	ASSERT ( m_frameNumber == m_GoodPredictions + m_ZeroPixelFrames );
	ASSERT ( m_ZeroPixelFrames == m_NotEnoughBrickFailures + m_PredictBrickFailures + m_NonRoadFailures);

	m_BuildJustCompleted = FALSE; // for the next iteration

	cvReleaseImage ( &tmp8uC1 );
	cvReleaseImage ( &tmp32fC1 );
}






void CROADCART::RunRegression(void)
{
	if (m_input == NULL) return;

	if (m_BuildTreeRequested == TRUE )
		OpenCVCartTrain();

	OpenCVCartTest();
}
void CROADCART::Image1(BrickPointType &pts)
{
	if (m_output == NULL) return;
	int width = m_output->width;
	int height = m_output->height;

	IplImage *intermediate1_8uC3 = cvCreateImage( cvGetSize(m_output), 8, 3 ); 


	LaserInputMask(pts);

	// Find shadows first.
	RemoveShadow();
	ResizeAndPlaceImage ( m_output, intermediate1_8uC3, 0, 0);

	BrickShadows();
	ResizeAndPlaceImage ( m_output, intermediate1_8uC3, width / 2, 0 );

	BuildNonRoad();
 	ResizeAndPlaceImage ( m_output, intermediate1_8uC3, 0, height / 2 );

	// Prepare the map of the area to train and test.
	PrepareAreaToTrainTest();
	ShowCARTMap();
 	ResizeAndPlaceImage ( m_output, intermediate1_8uC3, width / 2, height / 2 );

	// all 4 images are now placed in the output buffer to be displayed when IntermediateImages completes.
	cvCopy ( intermediate1_8uC3, m_output ) ;  

	cvReleaseImage ( &intermediate1_8uC3) ;
}
void CROADCART::Image2(void)
{
	if (m_input == NULL) return;
	FinalDecision ();
}
void CROADCART::ProductionRun(BrickPointType &pts)

{
//	m_Debug = FALSE; // looking for performance here!
#if 0
		// preparing for the install into Linux: here we set what would normally be parameters.
		m_CarHoodTop = 0.0; // the latest camera avoids the hood.
		m_ConfidenceThreshold = 0.5; // simple split - true or false.
		m_UseMinimalBrick = FALSE; // we will have a laser brick input.
		// Open (180, 120); // this is the resolution that gets the best performance
		// m_CreateAVI = FALSE; // we won't have this on Linux.
#endif

	LaserInputMask(pts);

	// Find shadows first.
	RemoveShadow();

	// Build the non-road
	BuildNonRoad();

	// prepare running average of the images. 
	BrickShadows();

	// Run the regression analysis
	PrepareAreaToTrainTest();
 	if (m_BuildTreeRequested == TRUE ) 
		OpenCVCartTrain();

	OpenCVCartTest();

	FinalDecision ();
}


void CROADCART::ResizeAndPlaceImage ( IplImage *in, IplImage *out, int x, int y)
{
	if ( m_Debug ) {
		int width  = m_output->width;
		int height = m_output->height;
		cvSetImageROI ( out, cvRect ( x, y, width / 2, height / 2 ));
		cvResize ( in, out );
		cvResetImageROI( out );
	}
}
