/* --Sparse Optical Flow Demo Program--
* Based on a version written by David Stavens (david.stavens@ai.stanford.edu)
*/

#include <stdio.h>
#include <cv.h>
#include <highgui.h>
#include <math.h>

static const double pi = 3.14159265358979323846;

#define NUMBEROFSHITOMASIFEATURES 400       //Arbitarily set... Can be altered depending on data (There is an Accuracy/Speed trade off)

inline static double square(int a)
{
    return a * a;
}

/* This is just an inline that allocates images.  I did this to reduce clutter in the
* actual computer vision algorithmic code.  Basically it allocates the requested image
* unless that image is already non-NULL.  It always leaves a non-NULL image as-is even
* if that image's size, depth, and/or channels are different than the request.
*/
inline static void allocateOnDemand( IplImage **img, CvSize size, int depth, int channels )
{
    if ( *img != NULL ) return;

    *img = cvCreateImage( size, depth, channels );
    if ( *img == NULL )
    {
        fprintf(stderr, "Error: Couldn't allocate image.  Out of memory?\n");
        exit(-1);
    }
}

#define NUMSHOTCUTBINS 100
#define SHOTRATIO 0.7
#define MINMOVEMENT 0.1

bool global_toggle_shot_detect = false;
double global_shot_cut_detect[NUMSHOTCUTBINS];
double global_previous_shot_cut_total = 0;
double global_pixel_count = 0;
double global_pixel_zero_count = 0;

void ShotDetectPerPixel(CvPoint a, CvPoint b)
{
    double angle;       angle = atan2( (double) a.y - b.y, (double) a.x - b.x );
    double hypotenuse;  hypotenuse = sqrt( square(a.y - b.y) + square(a.x - b.x) );

    if (hypotenuse == 0)
    {
        global_pixel_zero_count++;
    }

    global_pixel_count++;

    int index = (int)( (pi+angle) / (2*pi/NUMSHOTCUTBINS) );

    if (global_shot_cut_detect[index]<hypotenuse)
        global_shot_cut_detect[index]=hypotenuse;
}

bool ShotCutDetect()
{
    if (global_toggle_shot_detect == false)
        return false;

    double accumilator=0;

    for (int i = 0 ; i < NUMSHOTCUTBINS ; i++)
    {
        accumilator += ( global_shot_cut_detect[i]*global_shot_cut_detect[i] );
    }

    double metric = abs(accumilator - global_previous_shot_cut_total)/global_previous_shot_cut_total;

    //double ratio = global_pixel_zero_count/global_pixel_count;
    //printf("Ratio %f\n", ratio);

    if (global_pixel_zero_count/global_pixel_count>(1-MINMOVEMENT))
    {
        global_previous_shot_cut_total = accumilator;
        return false;
    }

    if (metric > SHOTRATIO)
    {
        global_previous_shot_cut_total = accumilator;
        printf("\nShotCutDetect Metric: %f - Shot Cut Detected\n", metric);

        return true;
    }

    global_previous_shot_cut_total = accumilator;
    return false;
}

int main(int argc , char *argv[])
{
    /* Create an object that decodes the input video stream. */
    CvCapture *input_video;
    if(argv[1] == NULL)
        input_video = cvCaptureFromFile("optical_flow_input.avi");
    else
        input_video = cvCaptureFromFile(argv[1]);
    //CvCapture *input_video = cvCaptureFromFile("optical_flow_input.avi");
    //CvCapture *input_video = cvCaptureFromFile("The_Cart_Boy_Clip.avi");
    if (input_video == NULL)
    {
        /* Either the video didn't exist OR it uses a codec OpenCV
        * doesn't support.
        */
        fprintf(stderr, "Error: Can't open video.\n");
        return -1;
    }

    IplImage *velx, *vely, *velX, *velY;
    CvSize blockSize = cvSize(4,4), shiftSize = cvSize(1,1), maxRange = cvSize(3,3);
    int type_of_of = 0;

    printf("Press P to pause, Q to quit\n\n");

    printf("Select Flavour of Optical Flow:\n");
    printf("\t Type: 1-> cvCalcOpticalFlowBM \n");
    printf("\t       2-> cvCalcOpticalFlowHS \n");
    printf("\t       3-> cvCalcOpticalFlowLK \n");
    printf("\t       4-> cvCalcOpticalFlowPyrLK \n");
    printf("\t Then Press <Enter>\n");

    scanf("%d", &type_of_of);

    if ( ( type_of_of < 1) || (type_of_of > 4) )
    {
        printf("Invalid Option");
        exit(-1);
    }

    if (type_of_of == 1)
    {
        printf("Turn Shot Cut Detection On? ):\n");
        printf("\t Type: 1-> Yes \n");
        printf("\t       2-> No \n");

        int shot_detect_on = 0;
        scanf("%d", &shot_detect_on);


        if ( shot_detect_on == 1)
        {
            global_toggle_shot_detect = true;
        }
    }

    if (input_video == NULL)
    {
        /* Either the video didn't exist OR it uses a codec OpenCV
        * doesn't support.
        */
        fprintf(stderr, "Error: Can't open video.\n");
        return -1;
    }
    else
        printf("Video: Loaded Sucessfully\n",argv[1]);

    /* Read the video's frame size out of the AVI. */
    CvSize frame_size;
    frame_size.height = cvGetCaptureProperty( input_video, CV_CAP_PROP_FRAME_HEIGHT );
    frame_size.width =  cvGetCaptureProperty( input_video, CV_CAP_PROP_FRAME_WIDTH );

    velY = cvCreateImage(cvSize(frame_size.width,frame_size.height),IPL_DEPTH_32F,1);
    velX = cvCreateImage(cvSize(frame_size.width,frame_size.height),IPL_DEPTH_32F,1);

    velx = cvCreateImage(cvSize(frame_size.width/blockSize.width,frame_size.height/blockSize.height),IPL_DEPTH_32F,1);
    vely = cvCreateImage(cvSize(frame_size.width/blockSize.width,frame_size.height/blockSize.height),IPL_DEPTH_32F,1);

    /* Determine the number of frames in the AVI. */
    long number_of_frames;
    /* Go to the end of the AVI (ie: the fraction is "1") */
    cvSetCaptureProperty( input_video, CV_CAP_PROP_POS_AVI_RATIO, 1. );
    /* Now that we're at the end, read the AVI position in frames */
    number_of_frames = (int) cvGetCaptureProperty( input_video, CV_CAP_PROP_POS_FRAMES );
    //  printf ("Number of frames: %d Size = (%d,%d)\n",number_of_frames, frame_size.width, frame_size.height);
    /* Return to the beginning */
    cvSetCaptureProperty( input_video, CV_CAP_PROP_POS_FRAMES, 0. );


    /* Create a windows called "Optical Flow" for visualizing the output.
    * Have the window automatically change its size to match the output.
    */
    cvNamedWindow("Optical Flow", CV_WINDOW_AUTOSIZE);

    long current_frame = 0;
    while(true)
    {
        static IplImage *frame = NULL, *frame1 = NULL, *frame1_1C = NULL, *frame2_1C = NULL, *eig_image = NULL, *temp_image = NULL, *pyramid1 = NULL, *pyramid2 = NULL;

        /* Go to the frame we want.  Important if multiple frames are queried in
        * the loop which they of course are for optical flow.  Note that the very
        * first call to this is actually not needed. (Because the correct position
        * is set outsite the for() loop.)
        */
        cvSetCaptureProperty( input_video, CV_CAP_PROP_POS_FRAMES, current_frame );

        /* Get the next frame of the video.
        * IMPORTANT!  cvQueryFrame() always returns a pointer to the _same_
        * memory location.  So successive calls:
        * frame1 = cvQueryFrame();
        * frame2 = cvQueryFrame();
        * frame3 = cvQueryFrame();
        * will result in (frame1 == frame2 && frame2 == frame3) being true.
        * The solution is to make a copy of the cvQueryFrame() output.
        */
        frame = cvQueryFrame( input_video );
        if (frame == NULL)
        {
            /* Why did we get a NULL frame?  We shouldn't be at the end. */
            fprintf(stderr, "Error: Hmm. The end came sooner than we thought.\n");
            return -1;
        }
        /* Allocate another image if not already allocated.
        * Image has ONE channel of color (ie: monochrome) with 8-bit "color" depth.
        * This is the image format OpenCV algorithms actually operate on (mostly).
        */
        allocateOnDemand( &frame1_1C, frame_size, IPL_DEPTH_8U, 1 );
        /* Convert whatever the AVI image format is into OpenCV's preferred format.
        * AND flip the image vertically.  Flip is a shameless hack.  OpenCV reads
        * in AVIs upside-down by default.  (No comment :-))
        */
        cvConvertImage(frame, frame1_1C, CV_CVTIMG_FLIP);

        /* We'll make a full color backup of this frame so that we can draw on it.
        * (It's not the best idea to draw on the static memory space of cvQueryFrame().)
        */
        allocateOnDemand( &frame1, frame_size, IPL_DEPTH_8U, 3 );
        cvConvertImage(frame, frame1, CV_CVTIMG_FLIP);

        /* Get the second frame of video.  Sample principles as the first. */
        frame = cvQueryFrame( input_video );
        if (frame == NULL)
        {
            fprintf(stderr, "Error: Hmm. The end came sooner than we thought.\n");
            return -1;
        }
        allocateOnDemand( &frame2_1C, frame_size, IPL_DEPTH_8U, 1 );
        cvConvertImage(frame, frame2_1C, CV_CVTIMG_FLIP);

        /* Shi and Tomasi Feature Tracking! */

        /* Preparation: Allocate the necessary storage. */
        allocateOnDemand( &eig_image, frame_size, IPL_DEPTH_32F, 1 );
        allocateOnDemand( &temp_image, frame_size, IPL_DEPTH_32F, 1 );

        /* Preparation: This array will contain the features found in frame 1. */
        CvPoint2D32f frame1_features[NUMBEROFSHITOMASIFEATURES];

        /* Preparation: BEFORE the function call this variable is the array size
        * (or the maximum number of features to find).  AFTER the function call
        * this variable is the number of features actually found.
        */
        int number_of_features = NUMBEROFSHITOMASIFEATURES;

        /* Actually run the Shi and Tomasi algorithm!!
        * "frame1_1C" is the input image.
        * "eig_image" and "temp_image" are just workspace for the algorithm.
        * The first ".01" specifies the minimum quality of the features (based on the eigenvalues).
        * The second ".01" specifies the minimum Euclidean distance between features.
        * "NULL" means use the entire input image.  You could point to a part of the image.
        * WHEN THE ALGORITHM RETURNS:
        * "frame1_features" will contain the feature points.
        * "number_of_features" will be set to a value <= 400 indicating the number of feature points found.
        */
        cvGoodFeaturesToTrack(frame1_1C, eig_image, temp_image, frame1_features, &number_of_features, .01, .01, NULL);

        /* Pyramidal Lucas Kanade Optical Flow! */

        /* This array will contain the locations of the points from frame 1 in frame 2. */
        CvPoint2D32f frame2_features[NUMBEROFSHITOMASIFEATURES];

        /* The i-th element of this array will be non-zero if and only if the i-th feature of
        * frame 1 was found in frame 2.
        */
        char optical_flow_found_feature[NUMBEROFSHITOMASIFEATURES];

        /* The i-th element of this array is the error in the optical flow for the i-th feature
        * of frame1 as found in frame 2.  If the i-th feature was not found (see the array above)
        * I think the i-th entry in this array is undefined.
        */
        float optical_flow_feature_error[NUMBEROFSHITOMASIFEATURES];

        /* This is the window size to use to avoid the aperture problem (see slide "Optical Flow: Overview"). */
        CvSize optical_flow_window = cvSize(5,5);

        /* This termination criteria tells the algorithm to stop when it has either done 20 iterations or when
        * epsilon is better than .3.  You can play with these parameters for speed vs. accuracy but these values
        * work pretty well in many situations.
        */
        CvTermCriteria optical_flow_termination_criteria
            = cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, .3 );

        /* This is some workspace for the algorithm.
        * (The algorithm actually carves the image into pyramids of different resolutions.)
        */
        allocateOnDemand( &pyramid1, frame_size, IPL_DEPTH_8U, 1 );
        allocateOnDemand( &pyramid2, frame_size, IPL_DEPTH_8U, 1 );

        /* Actually run Pyramidal Lucas Kanade Optical Flow!!
        * "frame1_1C" is the first frame with the known features.
        * "frame2_1C" is the second frame where we want to find the first frame's features.
        * "pyramid1" and "pyramid2" are workspace for the algorithm.
        * "frame1_features" are the features from the first frame.
        * "frame2_features" is the (outputted) locations of those features in the second frame.
        * "number_of_features" is the number of features in the frame1_features array.
        * "optical_flow_window" is the size of the window to use to avoid the aperture problem.
        * "5" is the maximum number of pyramids to use.  0 would be just one level.
        * "optical_flow_found_feature" is as described above (non-zero iff feature found by the flow).
        * "optical_flow_feature_error" is as described above (error in the flow for this feature).
        * "optical_flow_termination_criteria" is as described above (how long the algorithm should look).
        * "0" means disable enhancements.  (For example, the second array isn't pre-initialized with guesses.)
        */

        if (type_of_of==3)
            cvCalcOpticalFlowLK(frame1_1C, frame2_1C, cvSize(5,5),velX, velY );
        else if (type_of_of==1)
            cvCalcOpticalFlowBM( frame1_1C, frame2_1C, blockSize, shiftSize, maxRange, 0, velx, vely );
        else if (type_of_of==4)
            cvCalcOpticalFlowPyrLK(frame1_1C, frame2_1C, pyramid1, pyramid2, frame1_features, frame2_features, number_of_features, optical_flow_window, 5, optical_flow_found_feature, optical_flow_feature_error, optical_flow_termination_criteria, 0 );
        else if (type_of_of==2)
            cvCalcOpticalFlowHS(frame1_1C, frame2_1C,0, velX, velY,0.1,optical_flow_termination_criteria);

        for (int i = 0 ; i < NUMSHOTCUTBINS ; i ++)
            global_shot_cut_detect[i] = 0.0;

        global_pixel_count = 0;
        global_pixel_zero_count = 0;

        cvConvertImage(frame1_1C,frame1);
        CvPoint p0,p1;

        if (type_of_of==3 || type_of_of==2)
        {
            for(int i=0;i<velX->height;i+=3)
            {
                p0.y = i; //height

                for(int j=0;j<velX->width;j+=3)
                {
                    p0.x = j; //width
                    p1.x = p0.x + (int)( ( (float*)(velX->imageData + i*velX->widthStep) )[j] );
                    p1.y = p0.y + (int)( ( (float*)(velY->imageData + i*velY->widthStep) )[j] );
                    //cvCircle( frame1, p1, 1, CV_RGB(0,255,0), -1);
                    cvLine( frame1, p0, p1, CV_RGB(255,0,0), 1);
                    ShotDetectPerPixel(p0, p1);
                }
            }
        }
        else if (type_of_of==1)
        {
            for(int i=0;i<velx->height;i+=3)
            {
                p0.y = i*blockSize.height; //height
                for(int j=0;j<velx->width;j+=3)
                {
                    p0.x = j*blockSize.width; //width
                    p1.x = p0.x + (int)( (( (float*)(velx->imageData + i*velx->widthStep) )[j] )*blockSize.width);
                    p1.y = p0.y + (int)((( ( (float*)(vely->imageData + i*vely->widthStep) )[j] ))*blockSize.height);
                    cvCircle( frame1, p1, 1, CV_RGB(0,255,0), -1);
                    cvLine( frame1, p0, p1, CV_RGB(255,0,0), 1);
                    ShotDetectPerPixel(p0, p1);
                }
            }
        }
        else if (type_of_of==4)
        {
            /* For fun (and debugging :)), let's draw the flow field. */
            for(int i = 0; i < number_of_features; i++)
            {
                /* If Pyramidal Lucas Kanade didn't really find the feature, skip it. */
                if ( optical_flow_found_feature[i] == 0 )   continue;

                int line_thickness;             line_thickness = 1;
                /* CV_RGB(red, green, blue) is the red, green, and blue components
                * of the color you want, each out of 255.
                */
                CvScalar line_color;            line_color = CV_RGB(255,0,0);

                /* Let's make the flow field look nice with arrows. */

                /* The arrows will be a bit too short for a nice visualization because of the high framerate
                * (ie: there's not much motion between the frames).  So let's lengthen them by a factor of 3.
                */
                CvPoint p,q;
                p.x = (int) frame1_features[i].x;
                p.y = (int) frame1_features[i].y;
                q.x = (int) frame2_features[i].x;
                q.y = (int) frame2_features[i].y;
                ShotDetectPerPixel(p, q);

                double angle;       angle = atan2( (double) p.y - q.y, (double) p.x - q.x );
                double hypotenuse;  hypotenuse = sqrt( square(p.y - q.y) + square(p.x - q.x) );

                /* Here we lengthen the arrow by a factor of three. */
                q.x = (int) (p.x - 3 * hypotenuse * cos(angle));
                q.y = (int) (p.y - 3 * hypotenuse * sin(angle));

                /* Now we draw the main line of the arrow. */
                /* "frame1" is the frame to draw on.
                * "p" is the point where the line begins.
                * "q" is the point where the line stops.
                * "CV_AA" means antialiased drawing.
                * "0" means no fractional bits in the center cooridinate or radius.
                */
                if (hypotenuse>3){
                    cvLine( frame1, p, q, line_color, line_thickness, CV_AA, 0 );
                    /* Now draw the tips of the arrow.  I do some scaling so that the
                    * tips look proportional to the main line of the arrow.
                    */
                    p.x = (int) (q.x + 3 * cos(angle + pi / 4));
                    p.y = (int) (q.y + 3 * sin(angle + pi / 4));
                    cvLine( frame1, p, q, line_color, line_thickness, CV_AA, 0 );
                    p.x = (int) (q.x + 3 * cos(angle - pi / 4));
                    p.y = (int) (q.y + 3 * sin(angle - pi / 4));
                    cvLine( frame1, p, q, line_color, line_thickness, CV_AA, 0 );
                }
            }
            /* Now display the image we drew on.  Recall that "Optical Flow" is the name of
            * the window we created above.
            */
        }

        bool shotDetect = ShotCutDetect();

        if (shotDetect)
        {
            cvWaitKey(-1);
        }
        //cvConvertImage(frame1, frame1, CV_CVTIMG_FLIP);
        cvShowImage("Optical Flow", frame1);
        /* And wait for the user to press a key (so the user has time to look at the image).
        * If the argument is 0 then it waits forever otherwise it waits that number of milliseconds.
        * The return value is the key the user pressed.
        */
        int key_pressed;
        key_pressed = cvWaitKey(40);

        /* If the users pushes "b" or "B" go back one frame.
        * Otherwise go forward one frame.
        */

        if (key_pressed == 'q' || key_pressed == 'Q')
            break;//    exit(0);
        else if (key_pressed == 'p' || key_pressed == 'P')
            cvWaitKey(0);

        current_frame++;

        if (current_frame < 0)                      current_frame = 0;
        if (current_frame >= number_of_frames - 1)  break;//current_frame = number_of_frames - 2;

        //printf("Frame number = %d\n",current_frame);
    }
    cvReleaseCapture(&input_video);
}
