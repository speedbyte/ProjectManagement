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

#include <cv.h>
#include <highgui.h>

#include "laserbricksegmentation.h"

CLaserBrickSegmentation::CLaserBrickSegmentation(void) : RoadSegmentation()
{
}

CLaserBrickSegmentation::~CLaserBrickSegmentation(void)
{
	Close();
}


void CLaserBrickSegmentation::Open( int width, int height )
{
	RoadSegmentation::Open ( width, height);
}
void CLaserBrickSegmentation::Close( )
{
	RoadSegmentation::Close( );
}


void CLaserBrickSegmentation::ProductionRun(BrickPointType &pts)
{
	LaserInputMask( pts );
	cvZero ( m_Results8uC1 );
	cvCopy ( m_LaserBrickMask8uC1 , m_Results8uC1, m_CarMask8uC1 );
}

void CLaserBrickSegmentation::LoadNextImage ( IplImage *nextImage )
{
	if (m_input == (IplImage *) 0) {
		m_input = cvCloneImage ( nextImage ); 
//		SetDefaultBrick();
	}

	cvCopy ( nextImage, m_input );
	cvFlip ( m_input, m_output);
	cvSet  ( m_output, cvScalar ( 255, 255, 255), m_LaserBrickMask8uC1 );
//	cvSubS ( m_output, cvScalar ( 50, 50, 50), m_output, m_LaserBrickMask8uC1 );
}
