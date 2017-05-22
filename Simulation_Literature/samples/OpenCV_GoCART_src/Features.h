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
//   Author:    Rainer Lienhart
//              email: Rainer.Lienhart@informatik.uni-augsburg.de
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 


#include <cstdio>
//typedef unsigned char Ipp8u;
//typedef unsigned int Ipp32u;
#include <algorithm>
#include <numeric>
#include <map>

#include <cv.h>
#include <ipp.h>


// +----------------------------------------------------------------------------------------------+
// +----------------------------------------------------------------------------------------------+


// struct FeatureType { Ipp32u val; };

// FeatureMapType is a pair of a key (=feature value, Ipp32u) and value (=rel. frequency, float) 
typedef std::map<Ipp32u, float> FeatureMapType;
typedef std::map<Ipp32u, int> FeatureMapTypeInt;

// +--------------------------------------------------------------------------+
// | Features
// +--------------------------------------------------------------------------+
class FeatureTemplate {
public:
	// The target corridor (defined by min/maxTargetCorridor) specifies the desired percentage of 
	// feature values that are explained in the current brick by its feature distribution.
	// Based on the diversity of feature values, this can only be achieved by reducing or increasing
	// the MINPROBVALUE thresholding value. The system slowly adapt the MINPROBVALUE value to achieve 
	// (on average) this desirable percentage of feature explaination. MINPROBVALUE_LEARNING_RATE determine 
	// the speed of the learning rate. 
	float targetCorridor;
	virtual const float getMinTargetCorridor() = 0;
	virtual const float getMaxTargetCorridor() = 0;

	FeatureTemplate(float _targetCorridor=.75f) :  targetCorridor(_targetCorridor) {}
	
	// ‘src’ is passed down in order to know the width, height, and other attributes of the image
	// ‘ptr’ is the pointer to the center pixel of the current feature calculation
	virtual inline Ipp32u operator()(IplImage* src, Ipp8u* ptr) = 0;

	// Return the number of pixels which cannot be accessed at the boundary, 
	// i.e., pixels in side of the rectangle (leftBO, rightBO) - (width-1-rightBO, height-1-bottomBO)
	// should be passed to operator()
	virtual const int getLeftBoundaryOffset()  { return 0; }
	virtual const int getRightBoundaryOffset() { return 0; }
	virtual const int getTopBoundaryOffset()   { return 0; }
	virtual const int getBottomBoundaryOffset(){ return 0; }
};

// +- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
class ColorFeature : public FeatureTemplate { 
	int significantBits, colorSize;
public:
	ColorFeature(int _significantBits, float _targetCorridor=.75f) : FeatureTemplate(_targetCorridor), 
		significantBits(_significantBits), colorSize(1<<significantBits) {}

	virtual const float getMinTargetCorridor() { return targetCorridor; } 
	virtual const float getMaxTargetCorridor() { return targetCorridor+.05f; } 

	virtual inline Ipp32u operator()(IplImage* src, Ipp8u* ptr)
	{
		// color feature
		Ipp8u b=( (*ptr)>>(8-significantBits) ); ptr++;		
		Ipp8u g=( (*ptr)>>(8-significantBits) ); ptr++;
		Ipp8u r=( (*ptr)>>(8-significantBits) ); ptr++;
		Ipp32u index = r*colorSize*colorSize+g*colorSize+b;
		return index;
	}
};

// +- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
class Corr2Feature : public FeatureTemplate { 
	int significantBits, colorSize;
public:
	Corr2Feature(int _significantBits=6, float _targetCorridor=.75f) : FeatureTemplate(_targetCorridor), 
	  significantBits(_significantBits), colorSize(1<<significantBits) {}

	virtual const float getMinTargetCorridor() { return targetCorridor; } 
	virtual const float getMaxTargetCorridor() { return targetCorridor+.05f; } 

	virtual inline Ipp32u operator()(IplImage* src, Ipp8u* ptr)
	{
		// color feature
		Ipp8u b=( (*ptr)>>(8-significantBits) ); ptr++;		
		Ipp8u g=( (*ptr)>>(8-significantBits) ); ptr++;
		Ipp8u r=( (*ptr)>>(8-significantBits) ); ptr++;
		Ipp32u index = r*colorSize*colorSize+g*colorSize+b;
		// color correlation feature
		Ipp8u* ptrTmp = ptr + 4*src->nChannels;
		Ipp8u b2=(*ptrTmp)>>(8-significantBits); ptrTmp++;		
		Ipp8u g2=(*ptrTmp)>>(8-significantBits); ptrTmp++;
		Ipp8u r2=(*ptrTmp)>>(8-significantBits); ptrTmp++;
		Ipp32u index2 = r2*colorSize*colorSize+g2*colorSize+b2;
		for (int b=0; b<src->nChannels ; b++) index2<<=significantBits;
		index += index2;
		return index;
	}

	virtual const int getLeftBoundaryOffset()  { return 0; }
	virtual const int getRightBoundaryOffset() { return 4; }
	virtual const int getTopBoundaryOffset()   { return 0; }
	virtual const int getBottomBoundaryOffset(){ return 0; }
};

// +- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
class Corr3Feature : public FeatureTemplate { 
	int significantBits, colorSize;
public:
	Corr3Feature(int _significantBits, float _targetCorridor=.75f) : FeatureTemplate(_targetCorridor), 
		significantBits(_significantBits), colorSize(1<<significantBits) {}

	virtual const float getMinTargetCorridor() { return targetCorridor; } 
	virtual const float getMaxTargetCorridor() { return targetCorridor+.05f; } 

	inline Ipp32u operator()(IplImage* src, Ipp8u* ptr)
	{
		// color correlation feature
		Ipp8u* ptrTmp = ptr - 5*src->nChannels;
		Ipp8u b3=(*ptrTmp)>>(8-significantBits); ptrTmp++;		
		Ipp8u g3=(*ptrTmp)>>(8-significantBits); ptrTmp++;
		Ipp8u r3=(*ptrTmp)>>(8-significantBits); ptrTmp++;
		Ipp32u index3 = r3*colorSize*colorSize+g3*colorSize+b3;
		for (int b=0; b<2*src->nChannels ; b++) index3<<=significantBits;
		// color feature
		Ipp8u b1=(*ptr)>>(8-significantBits); ptr++;		
		Ipp8u g1=(*ptr)>>(8-significantBits); ptr++;
		Ipp8u r1=(*ptr)>>(8-significantBits); ptr++;
		Ipp32u index = r1*colorSize*colorSize+g1*colorSize+b1;
		// color correlation feature
		ptrTmp = ptr + 4*src->nChannels;
		Ipp8u b2=(*ptrTmp)>>(8-significantBits); ptrTmp++;		
		Ipp8u g2=(*ptrTmp)>>(8-significantBits); ptrTmp++;
		Ipp8u r2=(*ptrTmp)>>(8-significantBits); ptrTmp++;
		Ipp32u index2 = r2*colorSize*colorSize+g2*colorSize+b2;
		for (int b=0; b<src->nChannels ; b++) index2<<=significantBits;
		index += index2 + index3;
		return index;
	}

	virtual const int getLeftBoundaryOffset()  { return 4; }
	virtual const int getRightBoundaryOffset() { return 4; }
	virtual const int getTopBoundaryOffset()   { return 0; }
	virtual const int getBottomBoundaryOffset(){ return 0; }
};