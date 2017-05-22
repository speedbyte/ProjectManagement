#pragma once

#include <cv.h>
#include <highgui.h>
#include "roadsegmentation.h"
#include "features.h"


// Set the speed of adaptation. All values are specified at frame speed
const int   LEARNINGTIME = 30 * 1;	// learn for 10 seconds (30 fps assumed)
// Rate at which the distribution forgets
const float LEARNINGRATE = 1.0f / LEARNINGTIME;

// MINPROBVALUE determines the minimal required probability with which a feature must occur
// before it is considered to be a feature of the distrbiution. 
// We start here with a very low level to reduce start-up time
const float MINPROBVALUE = 0.001f;

// How fast do I allow to adapt the treshold MINPROBVALUE to reach the target corridor
const float MINPROBVALUE_LEARNING_RATE=0.95f;


class CRoadSegmentation_StochasticSampling : public RoadSegmentation
{
public:
	CRoadSegmentation_StochasticSampling(void);
	~CRoadSegmentation_StochasticSampling(void);

	void Open( int width, int height ); // Initialize the object images and metrics.
	virtual void Close( ); // This is when we are all done.
	virtual void ProductionRun(BrickPointType &pts);
	void CRoadSegmentation_StochasticSampling::LoadNextImage ( IplImage *nextImage );


	void CRoadSegmentation_StochasticSampling::RainersCallback(IplImage *m_input, IplImage *m_output,  IplImage *m_Results8uC1, BrickPointType &brick);
	void CRoadSegmentation_StochasticSampling::RainersCallback(IplImage* src,  IplImage *m_Results8uC1,  BrickPointType &brick);

	float m_targetCorridor;
	int significantBits;
	float m_minprobevalue;

private:
	FeatureMapType    prob;		// Learned feature value distribution. This is the memory of the distribution learning
};
