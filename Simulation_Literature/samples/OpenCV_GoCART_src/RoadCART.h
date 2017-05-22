#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <stdio.h>
#include <cv.h>
#include "ml.h"

#include "roadsegmentation.h"

 
// This is the maxiumum number of Road pixels allowed in the non-road area - or it is a bad regression analysis
// Making this high will get more non-zero frames but might yield false positives.
#define NON_ROAD_THRESHOLD (1500*4)  // set this to 500 to eliminate any mojave problems - more to get more road pixels.

// This defines the minimum number of pixels that must be in the brick or we just skip the frame. 
#define BRICK_MIN_PIXELS ( 600 )  

// How many pixels should be used when training.  If more, subsample.  If less, not enough laser brick.
// You can reduce this to BRICK_MIN_PIXELS but no less than BRICK_MIN_PIXELS.
// Reducing this will speed up training so you want to be as low as possible and still preserve accuracy.
extern int TRAINING_PIXEL_COUNT;  // target is applied for both road and non-road 

//This is the number of features in the feature vector.
#define FEATURE_COUNT ( 3 ) // Count of the number of features in each CART vector.

// This defines the threshold for shadow.
#define SHADOW_MAX ( 75 ) // threshold for defining shadow.

// Should we dilate the shadow to get rid of penumbra?  Don't set to zero or you will regret it!
#define SHADOW_DILATE ( 2 ) 

// even if things are going well, we should retrain after so many.  Is this useful?  I need to measure.
#define BUILD_FREQ ( 1  ) 

// If you want to turn off the trimming of the brick top, comment this line.
// However, the brick is often sloppy and including much non-road and while this does not solve the problem,
// it does improve the situation.
// #define TRIM_BRICK_TOP


// When defining non-road automatically (no mask from the previous frame), this the target number of shadow 
// pixels before we stop adding to the non-road definition.
#define NON_ROAD_SHADOW_TARGET_PIXELS ( 1200*4 )  

// Just some defines for the CART map.
								// CART_NON_ROAD -> non-road pixel used in training (there might be more, but
								//					they are not necessarily used for training. These
								//					are marked as CART_UNKNOWN)
								// CART_ROAD	 -> brick-pixel used in training (there might be more brick pixels)
								// CART_DONT_CARE-> brick-pixel not used in training
								// CART_UNKNOWN  -> everything else 
#define CART_DONT_CARE (0)
#define CART_UNKNOWN (1)
#define CART_ROAD (2)
#define CART_NON_ROAD (3)



class CROADCART : public RoadSegmentation
{ 
public:
	CROADCART(); 
	~CROADCART(); 
	void Open( int width, int height ); // Initialize the object images and metrics.
	void LoadNextImage ( IplImage *nextImage );  // Load the next image - first call for each image.
	void Close( ); // This is when we are all done.
//	void LaserInputMask(CvSeq *NextSeq);
//	IplImage *m_input;  // Some exposed images we always keep around.
	void BrickShadows( );
	int m_BuildTreeRequested;  // Build the tree before predicting the pixels in the next image.
	void OpenCVCartTrain();  // Train the tree!
	void OpenCVCartTest();  // predict each pixel in the image.
	void PrepareAreaToTrainTest();  // prepare the CART map that says what and where to test.
	CvStatModel *m_cls;  // This is the stat model for Road.  

	void SetDefaultBrick(); // The minimum brick is used when there is no list of bricks for each image.
	void RemoveShadow(); // This is where the threshold for shadow is applied.  Anything below is shadow.
	void ShowCARTMap(); // show the map of locations where will test.

	double m_ConfidenceThreshold;  // The external interface will set our confidence threshold
	void BuildNonRoad();   // Build the mask for the non-road training area.
	void FinalDecision();  // make the final decision: do we have road or are we confused.
	double m_RoadConfidence; // How well did we predict the laser brick?  If above the user's threshold, we have road.

	// some metrics that we want to know!
	double m_PixelsFound;
	long m_ZeroPixelFrames;
	long m_TrainingCount;		// Stores the number total number of training samples
	long m_PredictBrickFailures;
	long m_NotEnoughBrickFailures;
	long m_NonRoadFailures;
	long m_GoodPredictions;

	// Tyzx only stuff
	IplImage *m_Disparity32fC1;  // Only used for the Tyzx camera.
	void ShowDisparity();  // Show the distances - Tyzx only.
	void ShowDistanceLevels(); // Show the Tyzx distance to the edge of the road.

	void setClassificationThreshold(float _th) { classificationThreshold = _th; }
	float getClassificationThreshold() { return classificationThreshold; }

	void ProductionRun(BrickPointType &pts);
	void RunRegression(void);
	void Image1(BrickPointType &pts);
	void Image2(void);
	void ResizeAndPlaceImage ( IplImage *in, IplImage *out, int x, int y);


	IplImage* GetOutputImage () { return  m_output; }
	IplImage* GetCarMask8uC1() {return m_CarMask8uC1; }
	IplImage* GetResults8uC1() {return m_Results8uC1; }




	IplImage *m_CARTMap8uC1,	// 8u image. All pixels used for training are marked =! 0 (one of CART_NON_ROAD, CART_ROAD, CART_UNKNOWN
								// The meaning of this image is not fully understood.
		*m_NonRoad8uC1,			// 8u mask image. 0 at road pixels, otherwise 255
		*m_LightedBrick8uC1,	// 8u mask image. 255 at every laser brick pixel that is not 
								// in the shadow and does not belong to the car; otherwise 0
		*m_Shadow8uC1;			// 8u mask image. 255 at every shadow pixel; otherwise 0
								// CART_NON_ROAD -> non-road pixel used in training (there might be more, but
								//					they are not necessarily used for training. These
								//					are marked as CART_UNKNOWN)
								// CART_ROAD	 -> brick-pixel used in training (there might be more brick pixels)
								// CART_DONT_CARE-> brick-pixel not used in training
								// CART_UNKNOWN  -> everything else 
private:
	IplImage *m_input32fC3, *m_MinimalBrick8uC1, 
		*m_Results32fC1;
	IplImage *m_RegressionResults8uC1;
	int m_EnoughBrick;
	int m_BuildJustCompleted;
	int m_TrainCount;
	int m_RoadBuildCount;
	int m_NonRoadMaskAvailable;
	int m_MinBrickSize, m_MinShadowPixels, m_NonRoadMaxPixels;
	double SkipPercentMask ( IplImage *mask );
	void TestLaserInShadow();
	void NextCARTdata ( float *out, float *in );
	void HighlightResults ( IplImage *mask);
	void NormalizeTo255 ( CvArr *in, CvArr *out );

	void WriteMsg(char* message);
	
	int m_MinPixels;

	// This is the classifcation threshold for the learning methods. Probabilities smaller than this threshold are regarded as
	// road pixels.
	float classificationThreshold;	
public:
	int m_BuildFreq;
};

