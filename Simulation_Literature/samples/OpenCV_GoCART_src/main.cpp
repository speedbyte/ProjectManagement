
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

#include <cstdio>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <ctime>

#include <cv.h>
#include <highgui.h>

#include "RoadCART.h"
#include "RoadSegmentation_StochasticSampling.h"
#include "LaserBrickSegmentation.h"




// +-------------------------------------------------------------------------------------
// | Groundtruth code
// +-------------------------------------------------------------------------------------

class GTEntry {
public:
	int frameNo;
	std::string imageFileName;

	GTEntry(int _frameNo=-1, std::string _imageFileName="") : frameNo(_frameNo), imageFileName(_imageFileName) {}

	bool operator== (const GTEntry &gtentry) { 
		return gtentry.frameNo == frameNo; 
	}
	bool operator!= (const GTEntry &gtentry) { 
		return gtentry.frameNo != frameNo; 
	}
};

int loadGroundtruthDescription(const char *groundtruthDescriptionFileName, 
							   const char *targetVideoFileName,
							   std::vector<GTEntry> &frameNos)
{
	std::ifstream groundtruthDescriptionFileHandle ( groundtruthDescriptionFileName );
	if ( groundtruthDescriptionFileHandle == 0) {
		return 1;
	}

	std::string videoFileName;
	while ( !groundtruthDescriptionFileHandle.eof() ) 
	{
			groundtruthDescriptionFileHandle >> videoFileName;
			if ( groundtruthDescriptionFileHandle.eof() ) break;

			GTEntry gtEntry;
			groundtruthDescriptionFileHandle
				>> gtEntry.frameNo 
				>> gtEntry.imageFileName;

			if (  videoFileName == targetVideoFileName )
				frameNos.push_back ( gtEntry );

//			std::cout << videoFileName << "\t" << gtEntry.frameNo  << "\t" << gtEntry.imageFileName << std::endl;
	}
//	std::cout << "SIZE " << frameNos.size() << std::endl;

	return 0;
}



struct PerformanceNumbers {
	PerformanceNumbers(float _classificationThreshold=0.5f) : streetPixelIS(0), streetPixelGT(0), missedStreetPixelIS(0), 
		falseStreetPixelIS(0), recall(0), precision(0), frameCount(0), execTimeInSecs(0), 
		classificationThreshold(_classificationThreshold) {}

	long   streetPixelIS;		// sum of	  #isStreet
	long   streetPixelGT;		// sum of	  #gtStreet
	long   missedStreetPixelIS; // sum of     #
	long   falseStreetPixelIS;  // sum of     #
	double recall;				// sum of     #(isStreet & gtStreet) / #gtStreet
	double precision;			// sum of     #(isStreet & gtStreet) / #isStreet
	long   frameCount;			// how many values have been accumulated
	double execTimeInSecs;		// sum of execution times in seconds

	float classificationThreshold;

	PerformanceNumbers& operator+= (const PerformanceNumbers& pn) {
		streetPixelIS       += pn.streetPixelIS;
		streetPixelGT       += pn.streetPixelGT;
		missedStreetPixelIS += pn.missedStreetPixelIS;
		falseStreetPixelIS  += pn.falseStreetPixelIS;
		recall              += pn.recall;
		precision           += pn.precision;
		frameCount          += pn.frameCount;
		execTimeInSecs      += pn.execTimeInSecs;
		return *this;
	}


	FILE* printHeading(FILE* os) {
		fprintf ( os, "classificationThreshold\t"
			"StreetPixelIS\t"
			"CorrectStreetPixelIS\t"
			"StreetPixelGT\t"
			"MissedStreetPixelIS\t"
			"FalseStreetPixelIS\t"
			"recall\t"
			"precision\t"
			"FrameCount\t"
			"execTimeInSecs\n");
		return os;
	}
	std::ostream& printHeading(std::ostream& os) {
		os << "classificationThreshold\t" 
			<< "StreetPixelIS\t"
			<< "CorrectStreetPixelIS\t"
			<< "StreetPixelGT\t"
			<< "MissedStreetPixelIS\t"
			<< "FalseStreetPixelIS\t"
			<< "recall\t"
			<< "precision\t"
			<< "FrameCount\t"
			<< "execTimeInSecs" << std::endl;
		return os;
	}
	FILE* print(FILE* os) {
		fprintf ( os, 
			"%f\t"
			"%f\t"
			"%f\t"
			"%f\t"
			"%f\t"
			"%f\t"
			"%f\t"
			"%f\t"
			"%d\t",
			"%f\n",
			classificationThreshold,
			(double) streetPixelIS / (double) frameCount, 
			(double) (streetPixelIS-falseStreetPixelIS) / (double) frameCount, 
			(double) streetPixelGT / (double) frameCount, 
			(double) missedStreetPixelIS / (double) frameCount,
			(double) falseStreetPixelIS / (double) frameCount,
			(double) recall / (double) frameCount,
			(double) precision / (double) frameCount,
			frameCount,
			execTimeInSecs);
		return os;
	}


};	
std::ostream& operator<< (std::ostream& os, const PerformanceNumbers& pn) {
		os << pn.classificationThreshold << "\t"
			<< (double) pn.streetPixelIS / (double) pn.frameCount << "\t" 
			<< (double) (pn.streetPixelIS-pn.falseStreetPixelIS) / (double) pn.frameCount << "\t" 
			<< (double) pn.streetPixelGT / (double) pn.frameCount << "\t" 
			<< (double) pn.missedStreetPixelIS / (double) pn.frameCount << "\t"
			<< (double) pn.falseStreetPixelIS / (double) pn.frameCount << "\t"
			<< (double) pn.recall / (double) pn.frameCount << "\t"
			<< (double) pn.precision / (double) pn.frameCount << "\t"
			<< pn.frameCount << "\t"
			<< pn.execTimeInSecs;
		return os;
	}



PerformanceNumbers 
calcPerformance(IplImage *refImg,  // IN
				IplImage* isImg,	// IN
				RoadSegmentation* proadCART)
{
	// cvNamedWindow("1", CV_WINDOW_AUTOSIZE); cvShowImage("1", isImg);

	IplImage *gtStreet           = cvCreateImage( cvGetSize(refImg), 8, 1 ); // RED
	IplImage *gtStreetWithShadow = cvCreateImage( cvGetSize(refImg), 8, 1 ); // RED + GREEN
	IplImage *gtShadowStreet     = cvCreateImage( cvGetSize(refImg), 8, 1 ); // GREEN
	IplImage *gtCarHood          = cvCreateImage( cvGetSize(refImg), 8, 1 ); // YELLOW = RED + GREEN
	IplImage *isStreet           = cvCreateImage( cvGetSize(refImg), 8, 1 ); 

	cvSplit ( refImg, gtCarHood, gtShadowStreet, gtStreet, 0); // BGR

	// Remove the top car hood if specified.
	cvAnd ( gtStreet,           proadCART->GetCarMask8uC1(), gtStreet );
	cvAnd ( gtShadowStreet,     proadCART->GetCarMask8uC1(), gtShadowStreet );
	cvAnd ( gtCarHood,          proadCART->GetCarMask8uC1(), gtCarHood );
	cvAnd ( isImg,              proadCART->GetCarMask8uC1(), isImg );

	cvThreshold ( gtShadowStreet, gtShadowStreet, 254, 255, CV_THRESH_BINARY ); // Shadow is purley green = (0,255,0)
	cvThreshold ( gtStreet, gtStreet, 254, 255, CV_THRESH_BINARY );	// Street (including Shadow) is red = (255, *, 0)

	cvAnd ( gtStreet, gtShadowStreet, gtCarHood);	    // gtCarHood = all YELLOW pixels
	cvSub ( gtShadowStreet, gtCarHood, gtShadowStreet); // gtShadowStreet = all purely GREEN pixels
	cvDilate ( gtShadowStreet, gtShadowStreet, 0, 2);	// There is a small gab of one or two pixels between shadow street pixels and street pixels
	cvSub ( gtStreet, gtCarHood, gtStreet);				// gtStreet = all purely RED pixels
	cvOr  ( gtStreet, gtShadowStreet, gtStreetWithShadow );


/*	cvNamedWindow ( "GROUNDTRUTH : Street (RED)", CV_WINDOW_AUTOSIZE );
	cvNamedWindow ( "GROUNDTRUTH : Shadow Street (GREEN)", CV_WINDOW_AUTOSIZE );
	cvNamedWindow ( "GROUNDTRUTH : Car Hood (YELLOW)", CV_WINDOW_AUTOSIZE );
	cvNamedWindow ( "GROUNDTRUTH : Correctly Detected Street", CV_WINDOW_AUTOSIZE );
	cvNamedWindow ( "GROUNDTRUTH : False Alarms", CV_WINDOW_AUTOSIZE );
	cvShowImage ( "GROUNDTRUTH : Street (RED)", gtStreet );
	cvShowImage ( "GROUNDTRUTH : Shadow Street (GREEN)", gtShadowStreet );
	cvShowImage ( "GROUNDTRUTH : Car Hood (YELLOW)", gtCarHood );
*/
	// Number of street pixels in the ground truth
	int StreetPixelGT = cvCountNonZero ( gtStreetWithShadow );	
	// Number of predicted street pixels (correct + false)
	int StreetPixelIS = cvCountNonZero ( isImg ) ;
	// Number of correctly predicted street pixels
	cvAnd ( isImg, gtStreetWithShadow, isStreet) ;
	int CorrectStreetPixelIS = cvCountNonZero ( isStreet );
	int MissedStreetPixelIS  = StreetPixelGT -  CorrectStreetPixelIS;
//	cvShowImage ( "GROUNDTRUTH : Correctly Detected Street", isStreet );

	// Calculate the false alarms = all pixels which are classified as street, but are not street and not car hood.
	cvNot ( gtStreetWithShadow, gtStreetWithShadow);
	cvAnd ( isImg, gtStreetWithShadow, isStreet) ;
//	cvDilate ( gtCarHood, gtCarHood, 0, 2);
//	cvNot ( gtCarHood, gtCarHood);	// remove car hood from false alarms
//	cvAnd ( isStreet, gtCarHood, isStreet) ;
	int FalseStreetPixelIS = cvCountNonZero ( isStreet ) ;

/*	std::cout 
		<< "StreetPixelGT : " << StreetPixelGT << "\t"
		<< "StreetPixelIS : " << StreetPixelIS << "\t" 
		<< "FalseStreetPixelIS : " << FalseStreetPixelIS  << "\t" << (StreetPixelIS-CorrectStreetPixelIS)
		<< std::endl;
*/
//	cvShowImage ( "GROUNDTRUTH : False Alarms", isStreet );

	PerformanceNumbers pn;
	pn.streetPixelIS       += StreetPixelIS;		// How many street pixel has the algorithm found (wrongly and correctly)
	pn.streetPixelGT       += StreetPixelGT;		// How many should it have found
	pn.missedStreetPixelIS += MissedStreetPixelIS;	// How many street pixels were missed
	pn.falseStreetPixelIS  += FalseStreetPixelIS;	// How many of the found pixels were wrong
	if (StreetPixelGT != 0) pn.recall    += ( (double) CorrectStreetPixelIS / (double) StreetPixelGT );
	if (StreetPixelIS != 0) pn.precision += ( (double) (StreetPixelIS-CorrectStreetPixelIS) / (double) StreetPixelIS);
	pn.frameCount++;
	if (StreetPixelGT == 0) std::cout << "WARNING:  (StreetPixelGT == 0)" << std::endl;

	// std::cout << pn << std::endl;

	cvReleaseImage ( &isStreet );
	cvReleaseImage ( &gtCarHood );
	cvReleaseImage ( &gtShadowStreet );
	cvReleaseImage ( &gtStreet );
	cvReleaseImage ( &gtStreetWithShadow );

	return pn;
}


// +-------------------------------------------------------------------------------------
// | Groundtruth code
// +-------------------------------------------------------------------------------------
int findStreet (const char *groundtruthDescriptionFileName, 
				const char *videoFileName,
				RoadSegmentation* proadCART,
				PerformanceNumbers& pn
				)
{
	// For each video
	char videFileNameAVS[FILENAME_MAX];
	strcpy ( (char*) videFileNameAVS, videoFileName);
//	videFileNameAVS [ strlen(videFileNameAVS)-1 ] = 's';
	CvCapture* cap = cvCaptureFromFile( videFileNameAVS );
	if ( !cap ) return 1;

	char videFileNameAVI[FILENAME_MAX];
	strcpy ( (char*) videFileNameAVI, videoFileName);
//	videFileNameAVI [ strlen(videFileNameAVI)-1 ] = 'i';

	// Read in the list of all frames to which we have a manually labeled ground truth
	std::vector<GTEntry> frameNos;
	loadGroundtruthDescription ( groundtruthDescriptionFileName, videFileNameAVI, frameNos );

	// Read in the brick data and fill in eventual gaps. Note, however, there can be still 
	// garbage values in it
	std::vector <BrickPointType> brickPoints;
	readLaserBricks ( videoFileName, brickPoints );


	bool debug=false;
	IplImage *src_orig = 0;
	IplImage *src = 0;
	bool ResetPosition = true;
	for (int frameNo=0 ; 0 != (src_orig=cvQueryFrame(cap))  ; frameNo++) { 
		if (src == 0) src = cvCreateImage ( cvSize(360,240), 8, 3);
		cvResize( src_orig , src );

		if (ResetPosition) {
			proadCART->Open ( src->width, src->height );
		}

        proadCART->LoadNextImage( src );
/*		printf("%d BRICK %d,%d %d,%d %d,%d %d,%d\n",
			frameNo, 
			brickPoints[frameNo][0].x, brickPoints[frameNo][0].y,
			brickPoints[frameNo][1].x, brickPoints[frameNo][1].y,
			brickPoints[frameNo][2].x, brickPoints[frameNo][2].y,
			brickPoints[frameNo][3].x, brickPoints[frameNo][3].y
			);
*/

        // Show the results graphically
		if (debug) {
/*            //If FirstPass = True Then RoadLabels(PicIndex).Text = RoadLabels(PicIndex).Text + "ShowVideo"
 			if (ResetPosition) { cvNamedWindow( "Input Frame", CV_WINDOW_AUTOSIZE); }
			cvShowImage ( "Input Frame", proadCART->GetOutputImage ( ) );

            // This will show the left camera image, the horizon image, the brick distances, and the laser brick
            ((CROADCART*)proadCART)->Image1( brickPoints[frameNo] );
 			if (ResetPosition) { cvNamedWindow( "Shadow, LightBrick, NonRoad, CARTMap", CV_WINDOW_AUTOSIZE); }
			cvShowImage( "Shadow, LightBrick, NonRoad, CARTMap", proadCART->GetOutputImage ( ));

            ((CROADCART*)proadCART)->RunRegression();
			if (ResetPosition) { cvNamedWindow( "Regression Results", CV_WINDOW_AUTOSIZE); }
			cvShowImage( "Regression Results", proadCART->GetOutputImage ( ));

            ((CROADCART*)proadCART)->Image2();
			if (ResetPosition) { cvNamedWindow( "Final Decision", CV_WINDOW_AUTOSIZE); }
			cvShowImage( "Final Decision", proadCART->GetOutputImage ( ));
 			cvWaitKey( 1 );
*/
        } else {
			int brickFrameNo = min( frameNo, brickPoints.size()-1 );
			proadCART->ProductionRun ( brickPoints[brickFrameNo] );

//			if (ResetPosition) { cvNamedWindow ("ProductionRun" , CV_WINDOW_AUTOSIZE); }
			if (ResetPosition) { cvNamedWindow ("ProductionRun" , 0); }
			IplImage *dst = proadCART->GetOutputImage ( );
			cvShowImage ( "ProductionRun", dst );
        }

		// Load Reference Image
		GTEntry gtEntryTmp ( frameNo, "dummy" );
		std::vector<GTEntry>::const_iterator gtEntryIter = std::find ( frameNos.begin(), frameNos.end(), gtEntryTmp);
		if ( gtEntryIter != frameNos.end() ) {
			std::string tmpName = "groundtruth_images/" + gtEntryIter->imageFileName;
			// std::cout << "Frame with groundtruth data: " << gtEntryIter->frameNo << "\t" << tmpName << std::endl;

			IplImage* refImg = cvLoadImage ( tmpName.c_str() );

			if (refImg) {
				PerformanceNumbers pnTmp = calcPerformance(
					refImg,							// IN
					proadCART->GetResults8uC1(),		// IN
					proadCART);
				pn += pnTmp;
				if (pnTmp.streetPixelIS == 0) std::cout << brickPoints[frameNo] << std::endl;

			}

			cvReleaseImage ( &refImg );
		}

		cvWaitKey( 1 );
		ResetPosition = false;
	}

	cvReleaseImage ( &src );
	cvReleaseCapture ( &cap );

	return 0;
}
int main( int argc, char *argv[])
{
	const char *groundtruthDescriptionFileName = "frame.txt";

	if (argc < 3) return 0;
	char *algorithName = argv[2];

	float thresFrom = 0.5f;
	float thresTo   = 0.55f;
	float thresStep = 0.1f;
	if (argc > 3) thresFrom = atof( argv[3] );
	if (argc > 4) thresTo   = atof( argv[4] );
	if (argc > 5) thresStep = atof( argv[5] );
	//						  from  to step multiplicator
	// 1             2          3   4   5    6
	// testSet.txt CART        0.1 0.9 0.05 1.0
	// testSet.txt GENTLEBOOST 0.1 0.9 0.05 1.0
	// testSet.txt LASERBRICK
	// testSet.txt DISTEST
	std::cout << argv[0] << "\t<testSet.txt> CART <start threshold> <stop threshold> <step threshold> [<sampleFactor=1> <BuildFreq=1>]" << std::endl;
	std::cout << argv[0] << "\t<testSet.txt> GENTLEBOOST <start threshold> <stop threshold> <step threshold> <sampleFactor>" << std::endl;
	std::cout << argv[0] << "\t<testSet.txt> DISTEST <start targetCorridor> <stop targetCorridor> <step targetCooridor> <significantBits=6>" << std::endl;
	std::cout << argv[0] << "\t<testSet.txt> LASERBRICK" << std::endl;



	for (float classificationThreshold=thresFrom ; classificationThreshold <= thresTo ; classificationThreshold += thresStep)
	{
		// Sum of the respective feature over all test frames. 
		PerformanceNumbers total_pn;
		total_pn.classificationThreshold = classificationThreshold;

		// First loop run
		if ( classificationThreshold==thresFrom ) {	total_pn.printHeading ( std::cerr ); }

		const char *videoTestCaseFileName = argv[1];
		FILE *videoTestCaseID = fopen ( videoTestCaseFileName, "r" );
		while (! feof (videoTestCaseID) ) {
			PerformanceNumbers pn;
			pn.classificationThreshold = classificationThreshold;
			char buf[FILENAME_MAX] = {0};
			float carHoodTop;
			int targetWidth, targetHeight;
			int dummy;
			int count = fscanf ( videoTestCaseID, "%f,%d,%d,%d, %s", 
				&carHoodTop, &targetWidth, &targetHeight, &dummy, &buf);
			if (count <5) break;
			printf ("%f,%d,%d,%d, %s\n", carHoodTop, targetWidth, targetHeight, dummy, buf);


			RoadSegmentation* proadCART = 0;
			if ( strcmp(algorithName, "CART") == 0 ) 
			{
				// 1             2   3   4   5    6
				// testSet.txt CART 0.1 0.9 0.05 1.0
				proadCART = new CROADCART;
				((CROADCART*)proadCART)->m_ConfidenceThreshold = 0.5; // simple split - true or false.
				((CROADCART*)proadCART)->setClassificationThreshold ( classificationThreshold ); // simple split - true or false.
				((CROADCART*)proadCART)->m_BuildTreeRequested = TRUE;
				float trainFactor = 1; 	if (argc > 6) trainFactor = atof( argv[6] );
				int m_BuildFreq = 1; if (argc > 7) m_BuildFreq = atoi( argv[7] );
				((CROADCART*)proadCART)->m_BuildFreq = m_BuildFreq;
				if ( classificationThreshold==thresFrom ) TRAINING_PIXEL_COUNT = BRICK_MIN_PIXELS * trainFactor; // set only once
				std::cout << "TRAINING_PIXEL_COUNT : " << trainFactor << "\t" << TRAINING_PIXEL_COUNT << std::endl;
			} 
			else if ( strcmp(algorithName, "DISTEST") == 0 ) 
			{
				proadCART = new CRoadSegmentation_StochasticSampling;
				((CRoadSegmentation_StochasticSampling*)proadCART)->m_targetCorridor = classificationThreshold ; // simple split - true or false.
				if (argc > 6) ((CRoadSegmentation_StochasticSampling*)proadCART)->significantBits  = atoi( argv[6] );
				((CRoadSegmentation_StochasticSampling*)proadCART)->m_minprobevalue = MINPROBVALUE;
			} 
			else if ( strcmp(algorithName, "LASERBRICK") == 0 ) 
			{
				proadCART = new CLaserBrickSegmentation; 
			} 
			else if ( strcmp(algorithName, "GENTLEBOOST") == 0 ) 
			{
				// testSet.txt CART 0.1 0.9 0.05 1.0
				proadCART = new CROADCART;
				((CROADCART*)proadCART)->m_ConfidenceThreshold = 0.5; // simple split - true or false.
				((CROADCART*)proadCART)->setClassificationThreshold ( classificationThreshold ); // simple split - true or false.
				float trainFactor = 1;
				if (argc > 6) trainFactor = atof( argv[6] );
				if ( classificationThreshold==thresFrom ) TRAINING_PIXEL_COUNT *= trainFactor; // set only once

				CvBtClassifierTrainParams     param;
				memset ( &param, 1, sizeof(param) );
				param.boost_type = CV_BT_GENTLE;
				param.num_iter = 50;
				param.infl_trim_rate = 0.99;
				param.num_splits = 3;
			} 
			else {
				printf("Unkown learning algorithm name : %s\n", algorithName); 
				return 0;
			}
			proadCART->m_UseMinimalBrick = FALSE;
			proadCART->m_Debug = false;
			proadCART->m_CarHoodTop = carHoodTop; // the latest camera avoids the hood.


			clock_t startTime = clock();
			findStreet (groundtruthDescriptionFileName, 
				buf,
				proadCART,
				pn);
			clock_t stopTime = clock();
			pn.execTimeInSecs = (double) (stopTime-startTime) / double (CLOCKS_PER_SEC);

			pn.printHeading ( std::cout );
			std::cout << pn << std::endl;
			total_pn += pn;

			proadCART->Close();
			delete proadCART; proadCART=0;
		}

		fclose( videoTestCaseID ); videoTestCaseID=0;

		std::cerr << total_pn << std::endl;
	}

	return 0;
}

