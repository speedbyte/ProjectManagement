#include "roadsegmentation_stochasticsampling.h"


CRoadSegmentation_StochasticSampling::CRoadSegmentation_StochasticSampling(void) : RoadSegmentation(),
m_targetCorridor (.75f), significantBits(6), m_minprobevalue(MINPROBVALUE)
{
}

CRoadSegmentation_StochasticSampling::~CRoadSegmentation_StochasticSampling(void)
{
	Close();
}


void CRoadSegmentation_StochasticSampling::Open( int width, int height )
{
	RoadSegmentation::Open ( width, height);
}

void CRoadSegmentation_StochasticSampling::Close( )
{
	RoadSegmentation::Close( );
}


void CRoadSegmentation_StochasticSampling::ProductionRun(BrickPointType &pts)
{
	LaserInputMask( pts );

	RainersCallback ( m_input, m_output, m_Results8uC1, pts );

}

void CRoadSegmentation_StochasticSampling::LoadNextImage ( IplImage *nextImage )
{
	if (m_input == (IplImage *) 0) {
		m_input = cvCloneImage ( nextImage ); 
//		SetDefaultBrick();
	}

	cvZero ( m_input );
	cvCopy ( nextImage, m_input );
}




// +--------------------------------------------------------------------------+
// | Callback
// +--------------------------------------------------------------------------+
void CRoadSegmentation_StochasticSampling::RainersCallback(IplImage *m_input, IplImage *m_output,  IplImage *m_Results8uC1, BrickPointType &brick)
{
	if (m_input  == (IplImage *) 0) return;
	if (m_output == (IplImage *) 0) return;
	cvCopy(m_input, m_output);
	RainersCallback(m_output, m_Results8uC1, brick);
}

void CRoadSegmentation_StochasticSampling::RainersCallback(IplImage* src,  IplImage *m_Results8uC1,  BrickPointType &brick)
{
	ColorFeature feature ( significantBits, m_targetCorridor ); 
//	Corr2Feature feature ( significantBits, m_targetCorridor ); 
	if (m_frameNumber == 0)  {
		printf("************* significantBits %d m_targetCorridor %f m_frameNumber %d\n",
			significantBits, m_targetCorridor, m_frameNumber);
	}

	// origin==0 - top-left origin,
    // origin==1 - bottom-left origin (Windows bitmaps style)
	if (src->origin == 1) { cvFlip(src); src->origin = 0; }


	// Create laser brick mask;
	if (m_LaserBrickMask8uC1 == 0) m_LaserBrickMask8uC1= cvCreateImage( cvGetSize(src), 8, 1);
	CreateMask( brick[0], brick[1], brick[2], brick[3], m_LaserBrickMask8uC1);
	cvAnd( m_LaserBrickMask8uC1, m_CarMask8uC1, m_LaserBrickMask8uC1);
	

	// FeatureMapType    prob;			// Learned feature value distribution
	FeatureMapTypeInt probTmpNeu;		// current feature value distribution

	// Get feature distribution 
	long countBrickSize = 0;	// count number of features taken from current brick
	for (int y=feature.getTopBoundaryOffset() ; y<src->height-feature.getBottomBoundaryOffset() ; y++) {
		Ipp8u* srcPtr  = (Ipp8u*) src->imageData  + y * src->widthStep;
		Ipp8u* maskPtr = (Ipp8u*) m_LaserBrickMask8uC1->imageData + y * m_LaserBrickMask8uC1->widthStep;
		srcPtr  +=  feature.getLeftBoundaryOffset() * src->nChannels;
		maskPtr +=  feature.getLeftBoundaryOffset() * m_LaserBrickMask8uC1->nChannels;
		for (int x=feature.getLeftBoundaryOffset() ; x<src->width-feature.getRightBoundaryOffset() ; x++) {
			if (*maskPtr == 255) {
				// Calculate feature value
				int index = feature(src, srcPtr);

				FeatureMapTypeInt::iterator featureIter = probTmpNeu.find( index );
				if ( featureIter != probTmpNeu.end() ) {
					// feature value already in set
					featureIter->second++;
				} else {
					probTmpNeu [ index ] = 1;
				}
				countBrickSize++;
			}
			srcPtr += src->nChannels;
			maskPtr += m_LaserBrickMask8uC1->nChannels;
		}
	}

	// Remove all rare feature values
	int tt = probTmpNeu.size();
	for (FeatureMapTypeInt::iterator iter = probTmpNeu.begin() ; iter!= probTmpNeu.end(); ) { 
		if (iter->second < (m_minprobevalue*countBrickSize)) { 
			iter = probTmpNeu.erase(iter); 
		} else { ++iter; }
	}

	// Update threshold; Deterime whether the current threshold MINPROBVALUE would achieve the target 
	// coverage of the estimated distribution in the brick
	double sumsum=0;
	for (FeatureMapTypeInt::iterator iter = probTmpNeu.begin() ; iter!= probTmpNeu.end(); iter++) { 
		sumsum += iter->second;
	}
	sumsum /= countBrickSize;
	if (sumsum < feature.getMinTargetCorridor()) m_minprobevalue *= MINPROBVALUE_LEARNING_RATE;
	if (sumsum > feature.getMaxTargetCorridor()) m_minprobevalue /= MINPROBVALUE_LEARNING_RATE;
//	printf("SUMSUM %f\n", sumsum);


	// Update statistics
	if (m_frameNumber == 0) {
		for (FeatureMapTypeInt::iterator iter = probTmpNeu.begin() ; iter!= probTmpNeu.end(); iter++) { 
			prob[ iter->first ] = iter->second / float(countBrickSize);
		}
/*		for (std::map<Ipp32u, float>::iterator iter = prob.begin() ; iter!= prob.end(); iter++) {
			printf("prob %d %f\n", iter->first, iter->second);
		}
*/	} else {
		float learningRate = LEARNINGRATE;
//		if (count < 30) learningRate = 1.0f/(count+1.0f);
		// Leaky bucket at rate (1-LEARNINGRATE)
		for (FeatureMapType::iterator iter = prob.begin() ; iter!= prob.end(); iter++) { iter->second *= (1-learningRate); }

		for (FeatureMapTypeInt::iterator iterTmp = probTmpNeu.begin() ; iterTmp!= probTmpNeu.end(); iterTmp++) 
		{
			float probValue = iterTmp->second / float(countBrickSize);
			Ipp32u keyValue = iterTmp->first;

			if (probValue < m_minprobevalue) continue;

			FeatureMapType::iterator iter = prob.find ( keyValue );
			if (iter == prob.end()) {
				// Feature value not in top feature list yet -> add it
				prob.insert ( FeatureMapType::value_type (keyValue, probValue) );
			} else {
				// Feature value found it top feature list
				iter->second += ( probValue * learningRate );
			}
		}
	}

	// Remove all rare feature values
	for (FeatureMapType::iterator iter = prob.begin() ; iter!= prob.end(); ) { 
//		if (iter->second < m_minprobevalue) { iter = prob.erase(iter); iter--; }
		if (iter->second < m_minprobevalue) { 
			iter = prob.erase(iter); 
		} else { ++iter; }
	}
	double sumsum2=0;
	for (FeatureMapType::iterator iter = prob.begin() ; iter!= prob.end(); iter++) { 
		sumsum2 += iter->second;
	}
//	printf("%f %f  m_minprobevalue %f \n", sumsum, sumsum2, m_minprobevalue); 

	//	printf("SIZE %d\n", prob.size() );

	if (m_frameNumber >= LEARNINGTIME) {
		IplImage *tmp8uC1 = cvCreateImage( cvGetSize(src), 8, 1);
		cvSet(tmp8uC1, cvScalar(0));
		for (int y=feature.getTopBoundaryOffset() ; y<src->height-feature.getBottomBoundaryOffset() ; y++) {
			Ipp8u* ptr1 = (Ipp8u*) src->imageData     + y * src->widthStep;
			Ipp8u* ptr2 = (Ipp8u*) tmp8uC1->imageData + y * tmp8uC1->widthStep;

			ptr1 +=  feature.getLeftBoundaryOffset() * src->nChannels;
			ptr2 +=  feature.getLeftBoundaryOffset() * tmp8uC1->nChannels;

			for (int x=feature.getLeftBoundaryOffset() ; x<src->width-feature.getRightBoundaryOffset() ; x++)
			{
				// Calculate feature value
				int index = feature(src, ptr1);
				ptr1 += src->nChannels;

				FeatureMapType::iterator iter = prob.find( index );
//				if ((iter != prob.end()) && (iter->second > m_minprobevalue)) {
				if (iter != prob.end() ) {
					*ptr2 = 255; ptr2++;
				} else {
					*ptr2 = 0; ptr2++;		
				}
			}
		}

		cvMorphologyEx(tmp8uC1, tmp8uC1, 0, 0, CV_MOP_CLOSE, 2);
		cvMorphologyEx(tmp8uC1, tmp8uC1, 0, 0, CV_MOP_OPEN,  2);
		cvDilate(tmp8uC1, tmp8uC1, 0, 1);

		cvCopy(tmp8uC1, m_Results8uC1);
		// Remove scatter points
/*		CvPoint maxSeedPoint = {-1,-1};
		double maxArea = -1;
		IplImage *tmptmp8uC1 = cvCreateImage( cvGetSize(tmp8uC1), 8, 1);
		cvCopy(tmp8uC1, tmptmp8uC1); 
		for (int y=feature.getTopBoundaryOffset() ; y<src->height-feature.getBottomBoundaryOffset() ; y++) {
			Ipp8u* ptr1 = (Ipp8u*) tmp8uC1->imageData   + y * tmp8uC1->widthStep;
			Ipp8u* ptr2 = (Ipp8u*) tmptmp8uC1->imageData + y * tmptmp8uC1->widthStep;
			ptr1  +=  feature.getLeftBoundaryOffset() * tmp8uC1->nChannels;
			ptr2  +=  feature.getLeftBoundaryOffset() * tmptmp8uC1->nChannels;
			for (int x=feature.getLeftBoundaryOffset() ; x<src->width-feature.getRightBoundaryOffset() ; x++) {
				if (*ptr2 == 255) {
					CvConnectedComp comp;
					cvFloodFill(tmptmp8uC1, cvPoint(x,y), cvScalar(50), cvScalar(0), cvScalar(0), &comp, 8);
					if (maxArea < comp.area) {
						maxArea = comp.area;
						maxSeedPoint = cvPoint(x,y);
					} 
				}
				ptr1 += tmp8uC1->nChannels;
				ptr2 += tmptmp8uC1->nChannels;
			}
		}
		CvConnectedComp comp;
		cvFloodFill(tmptmp8uC1, maxSeedPoint, cvScalar(255), cvScalar(0), cvScalar(0), &comp, 8);
		cvThreshold(tmptmp8uC1, tmptmp8uC1, 240, 255, CV_THRESH_BINARY);
*/


		FloodFillBrick ( m_Results8uC1 );
		cvCopy ( m_LaserBrickMask8uC1, m_Results8uC1, m_LaserBrickMask8uC1);
				cvAnd ( m_Results8uC1, m_CarMask8uC1, m_Results8uC1);


//		cvSubS ( src, CV_RGB ( 100, 100, 100 ), src, m_Results8uC1 );
		cvSet ( src, CV_RGB ( 255, 255, 255 ), m_Results8uC1 );
//		cvCopy ( org, src, tmptmp8uC1 );
//		cvReleaseImage ( &tmptmp8uC1 );
		cvReleaseImage ( &tmp8uC1 );
	}

//	cvSubS ( src, CV_RGB ( 50, 50, 50 ), src, m_LaserBrickMask8uC1 );
	cvSet ( src, CV_RGB ( 255, 255, 255 ), m_LaserBrickMask8uC1 );

	if (src->origin == 0) { cvFlip(src); src->origin = 1; }

	m_frameNumber ++;
}

