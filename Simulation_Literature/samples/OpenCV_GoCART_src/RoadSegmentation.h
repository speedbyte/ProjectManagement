#pragma once

/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                University of Augsburg License Agreement
//                For Open Source Media Mining Library
//
// Copyright (C) 2005, University of Augsburg, Germany, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of University of Augsbrug, Germany may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the University of Augsburg, Germany or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
//   Author:    Bob Davies, Rainer Lienhart
//              email: bdavies@intel.com
//              email: Rainer.Lienhart@informatik.uni-augsburg.de
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 


#include <vector>
#include <cv.h>


// +-------------------------------------------------------------------------------------
// | Brick Code
// +-------------------------------------------------------------------------------------
const int NUM_POLY_POINTS = 4;	// Number of points describing a brick
struct BrickPointType {
	CvPoint points[4];
	BrickPointType ( ) { for (int i=0 ;i<4 ;  i++) { points[i] = cvPoint(-1,-1); }; }
	BrickPointType (const BrickPointType& bp ) {
		for (int i=0 ;i<4 ;  i++) { this->points[i] = bp.points[i]; }
	};
	CvPoint& operator[] (int index) { return points[index]; }
	CvPoint operator[]  (int index) const { return points[index]; }
};
std::ostream& operator<< (std::ostream& os, const BrickPointType& bp);


void readLaserBricks(const char *videoFileName, std::vector <BrickPointType> &brickPoints);


// When doing the flood fill, build a mask that is a dilation of the results to AND in with the real results.
const int FLOOD_DILATE = 2; // Obstacle Fix


// +-------------------------------------------------------------------------------------
// | RoadSegmentation
// +-------------------------------------------------------------------------------------
// This class contain the interface functions and the functionallity used by all road 
// segmentation algorithm. They are independent of the underlying learning algorithm.
// Discriminat as well as distribution learning algorithms will use this interface and
// extend it by a derived sub-class
class RoadSegmentation {

protected:
	IplImage *m_input;			// Copy of the current src image. We hold a copy of the 
								// source image, so that the parent code can release it at any time.
	IplImage *m_output;			// Current output image. The output image is a copy of the input image,
								// where the street pixels are marked brighter.
	IplImage *m_Results8uC1;	// Classification result (binary). 0 => not road; 255 => road

public:
	double m_CarHoodTop;		// fraction of the complete frame height that is considered to be part of
								// the car hood. 0 => use complete frame; 1 => use nothing from the frame
								// 0.1 => do not use anything from the 10% lowest part of the frame.
	IplImage *m_CarMask8uC1;	// Non-car hood pixels are marked 255, otherwise 0
								// Use BuildCarMask( m_CarHoodTop ) to create the mask image.

	IplImage *m_LaserBrickMask8uC1;	// 8u mask image. 255 at every laser brick pixel; otherwise 0


	int m_Debug;				// Flag to indicate we don't care about performance, but want to see a lot of
								// debug frames

	long m_frameNumber;		    // Count the number of frames which have been passed to LoadNextImage.
								// This variable is internally used to trigger many initializations!




	RoadSegmentation() : m_input(0), m_output(0), m_Results8uC1(0), m_CarHoodTop(0), m_CarMask8uC1(0), m_LaserBrickMask8uC1(0),
	m_Debug(false), m_frameNumber(0) {}
	~RoadSegmentation() { Close(); }


public:
	// These are the actual constructors and destructors
	virtual void Open( int width, int height );		// Initialize the object images and metrics.
	virtual void Close( );							// This is when we are all done.

	// This is the function to pass the next image to the algorithm
	virtual void LoadNextImage ( IplImage *nextImage ) = 0;  // Load the next image - first call for each image.

	// This is the class used to produce the output without debug informatation.
	virtual	void ProductionRun(BrickPointType &pts) = 0;


	IplImage* BuildCarMask(float tmp_CarHoodTop);
	void CreateMask ( CvPoint p1, CvPoint p2, CvPoint p3, CvPoint p4, IplImage *mask);
	void CreateMask ( CvPoint pts[NUM_POLY_POINTS], IplImage *mask);

	CvPoint m_CenterPoint;	// denotes the center point of laser brick
	void LaserInputMask(BrickPointType &m_BrickPt);
	void RoadSegmentation::FloodFillBrick( IplImage *results );
	int m_MinX, m_MaxX;
	int m_MinLeftX, m_MinLeftY, m_MinRightX, m_MinRightY;
	int m_MidX, m_Len, m_SaveMinY, m_PeakY;
	void ValidatePoint(CvPoint *pt ) ;
	int m_UseMinimalBrick;  // use the minimal brick when the video has no accompanying list of brick coordinates.
	IplImage *m_MinimalBrick8uC1;


	IplImage* GetOutputImage() { return m_output; }
	IplImage* GetResults8uC1() { return m_Results8uC1; }
	IplImage* GetCarMask8uC1() { return m_CarMask8uC1; }
};
