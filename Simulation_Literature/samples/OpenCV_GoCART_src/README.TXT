Requirements:
============
The software package has the following requirements: 
(1) The Xvid (www.xvid.org) or Divx (www.divx.com) codec must be installed to decode the sample AVI videos 
(2) OpenCV (Intel's Open Computer Vision Library, http://sourceforge.net/projects/opencvlibrary) beta5 or higher should be installed, too.

Note: 
The current beta 5 release of the Open Computer Vision Library does not include the machine learning code yet. 
If at time of release of this software the machine learning part on OpenCV is not available, two function calls 
in RoadCART.cpp must be replace by calls to some other machine learning library. The name of these two functions
are:
(a) cvCreateCARTClassifier(...) -- it does the cart learning given the training data, and 
(b) cvStatModelPredict(...) -- it does the classification given a trained classifier.


Execution:
=========
Open a command shell and cd to the OpenCV_GoCART\bin directory.

----------------
** LASERBRICK **
----------------
In order to just use the data from the laser brick, call:
	OpenCV_GoCART.exe testSet.txt LASERBRICK 2>resultLaserBrick.txt

This will calculate the performance of the laser brick data on the test set.

The first command line parameter "testSet.txt" specifies a test file with a list of the videos to process:
	0.15,180,120,1,mojave.avi2004.10.23_16-55-59.avi
	0.15,180,120,1,mojave.avi2004.10.23_17-05-26.avi
	...
	0.0,180,120,1,stereo-sunday007.avi
One line is for each video. The first parameter in each lines tells the system how many 
lines it should ignore from the bottom (in percent / 100.0). This is sometimes necessary since the car hood is 
visible in the video at the bottom. 
The next three parameters in the file are currently ignored
The last string in a line specifies the video file name.

The second command line parameter specifies the detection mode (LASERBRICK, CART, or GENTLEBOOST). No additional
parameters are needed for the LASERBRICK mode.

----------------
** CART **
----------------
To run the CART road segmentation use the following general calling synopsis:
	OpenCV_GoCART.exe testSet.txt CART <start threshold> <stop threshold> <step threshold> [<sampleFactor=1> <BuildFreq=1>]
such as
	OpenCV_GoCART.exe testSet.txt CART 0.85 0.9 0.1 2 2  2>resultCART_x2_step2.txt

In this example the cart decision threshold is 0.8 for road. 

Command line parameter three to five must be read as a loop statement 
	for (float t=<start threshold> ; f<<stop threshold> ; f+=<step threshold>) { ...}
The program will run through all videos specified in "testSet.txt" if the various threshold t parameters.
For the meaning of sampling Factor and build frequency consult the paper.

-----------------
** GENTLEBOOST **
-----------------
Go into the source code of RoadCART.cpp. Change
	#define CART_LEARNING
	//#define BOOSTING
to
	//#define CART_LEARNING
	#define BOOSTING

Recompile the code and then use 
	OpenCV_GoCART.exe testSet.txt GENTLEBOOST <start threshold> <stop threshold> <step threshold> [<sampleFactor=1> <BuildFreq=1>]
such as
	OpenCV_GoCART.exe testSet.txt GENTLEBOOST 0.8 0.9 0.1 2 2  2>resultCART_x2_step2.txt

-------------
** DISTEST **
-------------
In order to run the classification with the color distribution learning code use
	OpenCV_GoCART.exe testSet.txt DISTEST <start targetCorridor> <stop targetCorridor> <step targetCooridor> <significantBits=6>
such as
	OpenCV_GoCART.exe testSet.txt DISTEST 0.65 0.75 0.1 6  2>resultDISTEST_x6_color.txt

<significantBits=6> specifies the number bit of each color channel to use in the color distribution learning code.

-------------
** DISTEST **
-------------
If you want to use color correlation features, then go into the source code of CRoadSegmentation_StochasticSampling.cpp. Change
	ColorFeature feature ( significantBits, m_targetCorridor ); 
	//Corr2Feature feature ( significantBits, m_targetCorridor ); 
to
	// ColorFeature feature ( significantBits, m_targetCorridor ); 
	Corr2Feature feature ( significantBits, m_targetCorridor ); 

Recompile the code and then use 
	OpenCV_GoCART.exe testSet.txt DISTEST <start targetCorridor> <stop targetCorridor> <step targetCooridor> <significantBits=6>
such as
	OpenCV_GoCART.exe testSet.txt DISTEST 0.65 0.75 0.1 6  2>resultDISTEST_x6_corr2.txt
