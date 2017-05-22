#include <iostream>
#include <vector>
#include <cv.h>
#include <highgui.h>

#include "RoadSegmentation.h"

std::ostream& operator<< (std::ostream& os, const BrickPointType& bp) {
		os << bp[0].x << "," << bp[0].y << "\t" 
			<< bp[1].x << "," << bp[1].y << "\t" 
			<< bp[2].x << "," << bp[2].y << "\t" 
			<< bp[3].x << "," << bp[3].y 
			<< std::endl;
		return os;
}


void readLaserBricks(const char *videoFileName, std::vector <BrickPointType> &brickPoints) 
{
	char brickFileName[FILENAME_MAX] = {0};
	strncpy ( brickFileName, videoFileName, strlen(videoFileName)-4 );
	strcat  ( brickFileName, ".Bricks.txt");

	FILE* brickFileID = fopen ( brickFileName, "r" );


	BrickPointType brick;
	int indexNo = 0; // holds the read index from the brick file
	int count = 0;   // holds the current counter of lines read so far.
	while ( !feof ( brickFileID ) ) {
		int argsParsed = fscanf( brickFileID, "%d %d,%d %d,%d %d,%d %d,%d", &indexNo,
			&brick[0].x, &brick[0].y,
			&brick[1].x, &brick[1].y,
			&brick[2].x, &brick[2].y,
			&brick[3].x, &brick[3].y);
		if (argsParsed != 9) break;
		while (indexNo != count) {
			// Brick data is missing; copy in new entry
			brickPoints.push_back ( brick );
			count++;
		}
		brickPoints.push_back ( brick );
		count++;
	}
	
	printf("Number of Bricks : %d\n", brickPoints.size() );
	fclose ( brickFileID ); brickFileID=0;
}


///////////////////////////////////////////////////////////////////////////////
// BuildCarMask - build a mask the covers the car hood - depends on carhoodtop!
///////////////////////////////////////////////////////////////////////////////
IplImage* RoadSegmentation::BuildCarMask(float tmp_CarHoodTop)
{
	IplImage* tmp_CarMask8uC1 = cvCreateImage ( cvSize (m_output->width, m_output->height), 8, 1); 
	CreateMask ( cvPoint (0, 0), 
				 cvPoint (m_output->width, 0),
				 cvPoint (m_output->width, m_output->height - (int) (m_output->height * tmp_CarHoodTop)), 
				 cvPoint (0, m_output->height - (int) (m_output->height * tmp_CarHoodTop)), 
				 tmp_CarMask8uC1);
	return tmp_CarMask8uC1;
}

///////////////////////////////////////////////////////////////////////////////
// CreateMask is just for the convenience of creating a mask from 4 points.
///////////////////////////////////////////////////////////////////////////////
void RoadSegmentation::CreateMask ( CvPoint p1, CvPoint p2, CvPoint p3, CvPoint p4, IplImage *mask)
{
#define NUM_POLY_POINTS (4)
	CvPoint pts[NUM_POLY_POINTS],*pt[1]; 
	int arr[1];
	cvZero ( mask );
	arr[0] = NUM_POLY_POINTS;

	pt[0] = &(pts[0]);
	pts[0] = p1;
	pts[1] = p2;
	pts[2] = p3;
	pts[3] = p4;

	cvFillPoly ( mask, pt, arr, 1, cvScalar(255));
	cvPolyLine ( mask, pt, arr, 1, 1, CV_RGB(255,255,255), 3);
}
void RoadSegmentation::CreateMask ( CvPoint pts[NUM_POLY_POINTS], IplImage *mask)
{
	CvPoint *pt[1] = { &(pts[0]) }; 
	int arr[1]= { NUM_POLY_POINTS };
	cvZero ( mask );

	cvFillPoly ( mask, pt, arr, 1, cvScalar(255));
	cvPolyLine ( mask, pt, arr, 1, 1, CV_RGB(255,255,255), 3);
}

void RoadSegmentation::Open( int width, int height)
{
	m_input = (IplImage *) 0;	// create it based on the input we get - grayscale or color image.
	m_output      = cvCreateImage ( cvSize ( width, height ), 8, 3);
	m_Results8uC1 = cvCreateImage ( cvSize (width, height), 8, 1);  
	m_CarMask8uC1 = BuildCarMask ( m_CarHoodTop ); 
}

void RoadSegmentation::Close( )
{
	if (m_input)       cvReleaseImage (&m_input); m_input=0;
	if (m_output)      cvReleaseImage (&m_output); m_output=0;
	if (m_Results8uC1) cvReleaseImage (&m_Results8uC1); m_Results8uC1=0;
	if (m_CarMask8uC1) cvReleaseImage (&m_CarMask8uC1); m_CarMask8uC1=0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FloodFillBrick - floodfill the region in front of the car to eliminate islands of road on either side.
///////////////////////////////////////////////////////////////////////////////////////////////////
void RoadSegmentation::FloodFillBrick( IplImage *results )
{
/*	long centerPointx =0,  centerPointy = 0;
	int pointCounter = 0;
	for (int y=0 ; y<m_LaserBrickMask8uC1->height ; y++) {
		char* ptr = (char*) &m_LaserBrickMask8uC1->imageData[ y * m_LaserBrickMask8uC1->widthStep ];
		for (int x=0 ; x<m_LaserBrickMask8uC1->width ; x++) {
			if (*ptr != 0) {
				centerPointx += x;
				centerPointy += y;
				pointCounter++;
				ptr++;
			}
		}
	}
	if (pointCounter==0) {
		return;
	}
	m_CenterPoint.x = centerPointx / pointCounter;
	m_CenterPoint.y = centerPointy / pointCounter;
//		printf( "** m_CenterPoint : %d\t%d\n", m_CenterPoint.x , m_CenterPoint.y );
*/

	IplImage *tmp8uC1 = cvCreateImage ( cvGetSize(results), results->depth, results->nChannels );
	cvThreshold( results, tmp8uC1, 128, 250, CV_THRESH_BINARY );

	cvSet   ( tmp8uC1, cvScalar ( 250 ), m_LaserBrickMask8uC1 );
	cvErode ( tmp8uC1, tmp8uC1, 0, FLOOD_DILATE );
	cvDilate( tmp8uC1, results, NULL, FLOOD_DILATE );
	cvSet   ( results, cvScalar ( 250 ), m_LaserBrickMask8uC1 );

	// Flood fill at the top the road laser brick
	bool FoundFillPoint = false;
	for (m_CenterPoint.y = m_CenterPoint.y; m_CenterPoint.y < m_input->height; m_CenterPoint.y++) {
		if (results->imageData[m_CenterPoint.x + m_CenterPoint.y * results->widthStep] != 0) {
			FoundFillPoint = true;
			break;
		}
	}

	// If there was a fill point that we can use, floodfill starting there.
	if (FoundFillPoint) {
		cvFloodFill (results, m_CenterPoint, cvScalar(255), cvScalar(1), cvScalar(0) );
//		printf ( " CONT HERE %d %d\n", m_CenterPoint.x, m_CenterPoint.y);
	} else {
//		printf ( " STOP HERE %d %d\n", m_CenterPoint.x, m_CenterPoint.y);
	}
	cvThreshold ( results, results, 254, 255, CV_THRESH_TOZERO );
	cvAnd ( results, m_CarMask8uC1, results);

	cvReleaseImage ( &tmp8uC1 );
}

///////////////////////////////////////////////////////////////////////////////
// LaserInputMask - This builds the laser brick from the 4 points provided from
// the laser code.  
///////////////////////////////////////////////////////////////////////////////
void RoadSegmentation::LaserInputMask(BrickPointType &m_BrickPt)
{
	// input bricks are constructed assuming a 320x240 image but the videos are 720x480
	// Remapping them to m_input size is to multiply times 2 and then the ratio of m_input->height / 480.
	float factor = (float) 2.0 * m_input->height / 480.0; 

	// Determine the center point of the laser brick
	m_CenterPoint.x = m_CenterPoint.y = 0;
	bool BadBrick = 0; 
	for (int i = 0; i < NUM_POLY_POINTS; i++ ) {
		if (m_BrickPt[i].x < 0 || m_BrickPt[i].y < 0 ) BadBrick = true;
	 	m_BrickPt[i].x = (int) (m_BrickPt[i].x * factor);
		m_BrickPt[i].y = (int) (m_BrickPt[i].y * factor);
		m_CenterPoint.x += m_BrickPt[i].x;
		m_CenterPoint.y += m_BrickPt[i].y;
	}
	m_CenterPoint.x = (m_CenterPoint.x + NUM_POLY_POINTS/2) / NUM_POLY_POINTS;
	m_CenterPoint.y = (m_CenterPoint.y + NUM_POLY_POINTS/2) / NUM_POLY_POINTS;
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
	for (int i = 0; i < 4; i++)  {
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
		for (int i = 0; i < 4; i++)  {
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
	if (m_LaserBrickMask8uC1 == 0) m_LaserBrickMask8uC1=cvCreateImage ( cvGetSize(m_input), 8, 1 );
	CreateMask ( m_BrickPt[0], 
				 m_BrickPt[1], 
				 m_BrickPt[2], 
				 m_BrickPt[3], m_LaserBrickMask8uC1);
//	CreateMask ( m_BrickPt, m_LaserBrickMask8uC1);

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
			m_CenterPoint.x = m_CenterPoint.y = 0;
		for (int i = 0; i < NUM_POLY_POINTS; i++ ) {
			m_CenterPoint.x += m_BrickPt[i].x;
			m_CenterPoint.y += m_BrickPt[i].y;
		}
		m_CenterPoint.x /= NUM_POLY_POINTS;
		m_CenterPoint.y /= NUM_POLY_POINTS;
		if (m_CenterPoint.x < 0) m_CenterPoint.x=0;
		if (m_CenterPoint.y < 0) m_CenterPoint.y=0;
		if (m_CenterPoint.x >= m_input->width) m_CenterPoint.x=m_input->width-1;
		if (m_CenterPoint.y >= m_input->height) m_CenterPoint.y=m_input->height-1;

}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Make sure the point we will use for a flood fill is within the image area.
///////////////////////////////////////////////////////////////////////////////////////////////////
void RoadSegmentation::ValidatePoint(CvPoint *pt ) 
{
	if (pt->x < 0) pt->x = 0;
	if (pt->x >= m_output->width) pt->x = m_output->width - 1;
	if (pt->y < 0) pt->y = 0;
	if (pt->y >= m_output->height ) pt->y = m_output->height - 1;
}
