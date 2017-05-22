/*
 * main.c
 *
 *  Created on: 29.04.2010
 *      Author: Anton Tratkovski
 */

#include <string.h>
#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <cv.h>
#include <highgui.h>
#include <math.h>

static const double pi = 3.14159265358979323846;

inline static double square(int a)
{
	return a * a;
}

/* This is just an inline that allocates images.  I did this to reduce clutter in the
 * actual computer vision algorithmic code.  Basically it allocates the requested image
 * unless that image is already non-NULL.  It always leaves a non-NULL image as-is even
 * if that image's size, depth, and/or channels are different than the request.
 */
inline static void allocateOnDemand ( IplImage **img, CvSize size, int depth, int channels )
		{
	if ( *img != NULL )	return;

	*img = cvCreateImage( size, depth, channels );
	if ( *img == NULL )
	{
		fprintf(stderr, "Error: Couldn't allocate image.  Out of memory?\n");
		exit(-1);
	}
}

/*Diese Striktur eth�lt zwei Pointer auf Bilder, diese werden 
 *f�r die Detektion des optischen Flusses gef�llt und untersucht
 */
struct TwoFrames {
	IplImage *frame1;
	IplImage *frame2;
};

/*Diese Struktur enth�lt zwei Punkte, und definiert damit einen Vektor
 */
struct Pointer {
	CvPoint p1;
	CvPoint p2;
};

/*Diese Funktion gibt die L�nge eines Vektors zur�ck
 */
float LaengePtr (struct Pointer P){
	int x, y;
	x = (P.p1.x > P.p2.x)? (P.p1.x - P.p2.x) : (P.p2.x - P.p1.x);	// berechnet den Betrag von der x-Komponente
	y = (P.p1.y > P.p2.y)? (P.p1.y - P.p2.y) : (P.p2.y - P.p1.y);	// berechnet dem Betrag von der Y-Komponente
	return sqrt(square(x) + square(y));								// berechnet die L�nge und gibt sie zur�ck
}

/*Diese Funktion gibt die L�nge der X-Komponente eines Vektors zur�ck
 */
int LaengePtrX (struct Pointer P){
	return (P.p1.x > P.p2.x)? (P.p1.x - P.p2.x) : (P.p2.x - P.p1.x);// berechnet den Betrag von der x-Komponente
}

/*Diese Funktion gibt die L�nge der Y-Komponente eines Vektors zur�ck
 */
int LaengePtrY (struct Pointer P){
	return (P.p1.y > P.p2.y)? (P.p1.y - P.p2.y) : (P.p2.y - P.p1.y);// berechnet dem Betrag von der Y-Komponente
}

/*Diese Funktion zeichnet den Vektor ptr in das Bild *frame
 *frame_size ist die Gr��e des Bildes 
 *angle2 ist der Winkel in dem der Vektor steht 
 *(konnte nicht in der Funktion selber berechnet werden, es gab dabei immer ein Fehler)
 */
int FlowPointer(IplImage *frame, struct Pointer ptr, CvSize frame_size, double angle2) {

	CvFont Font_ = cvFont(0.5, 1);// Schrift der Ausgabe
	char str[256];					// String zur Text�bergabe
	int main_line_thickness = 1;	// Dicke der Linie
	CvScalar main_line_color = CV_RGB(0,255,0); // Farbe der Linie
	CvPoint p3;						// Punkt p3 definiert den Mittelpunkt des Bildes
	p3.x = frame_size.width/2;		// x-Komponente des Mittelpunktes
	p3.y = frame_size.height/2;		// y-Komponente des Mittelpunktes
	CvPoint p4;						// Punkt p4 wird zur Spitze des Vektors
	int Dx, Dy;						// Abweichungen des Anfangs des Vektors zur Mitte des Bildes
	Dx = ptr.p2.x - p3.x;			// Berechnung der Abweichung in x-Richtung
	Dy = ptr.p2.y - p3.y;			// Berechnung der Abweichung in y-Richtung
	p4.x = ptr.p1.x - Dx;			
	p4.y = ptr.p1.y - Dy;			//Berechnung der x und y-Komponente der Spitze

	// Textausgabe der x und y Werte
	sprintf(str, "X = %i ", (ptr.p1.x - ptr.p2.x));
	cvPutText(frame, str, cvPoint(frame_size.width-60, 10), &Font_, cvScalar(0, 0, 0, 0));
	sprintf(str, "Y = %i ", (ptr.p1.y - ptr.p2.y));
	cvPutText(frame, str, cvPoint(frame_size.width-60, 20), &Font_, cvScalar(0, 0, 0, 0));

	cvLine(frame, p3, p4 , main_line_color, main_line_thickness, CV_AA, 0); // Eigentliche Linie des Vektors
	p3.x = (int) (p4.x + 9 * cos(angle2 - pi / 5 + pi));
	p3.y = (int) (p4.y + 9 * sin(angle2 - pi / 5 + pi));					// Umrechnung zur einer der Pfeilspitzen-Linie
	cvLine(frame, p4, p3, main_line_color, main_line_thickness, CV_AA, 0);	// Zeichnet die Pfeilspitzen-Linie
	p3.x = (int) (p4.x + 9 * cos(angle2 + pi / 5 + pi));
	p3.y = (int) (p4.y + 9 * sin(angle2 + pi / 5 + pi));					// Umrechnung zur anderen Pfeilspitzen-Linie
	cvLine(frame, p3, p4, main_line_color, main_line_thickness, CV_AA, 0);	// Zeichnet die Pfeilspitzen-Linie

	return 1; // bei erfolgreichem Abarbeiten der Funktion, gibt eine Einz zur�ck
}

/*Diese Funktion berechnet den optischen Fluss nach der Block Mathing Methode
*/
struct Pointer FramesToPointerBM( struct TwoFrames tf, CvSize fs , int param){

			static IplImage *frame1 = NULL, *frame1_1C = NULL, *frame2_1C = NULL, *pyramid1 = NULL, *pyramid2 = NULL;
			static CvPoint point;

			allocateOnDemand( &frame1_1C, fs, IPL_DEPTH_8U, 1 ); 	//Alokieren des Bildes frame1_C1
			cvConvertImage(tf.frame1, frame1_1C, CV_CVTIMG_FLIP);	//�bertragen in das Bild das erste Frame

			allocateOnDemand( &frame2_1C, fs, IPL_DEPTH_8U, 1 );	//Alokieren des Bildes frame2_C1
			cvConvertImage(tf.frame2, frame2_1C, CV_CVTIMG_FLIP);	//�bertragen in das Bild das zweite Frame

			CvSize blockSize = cvSize(14,14);						//Gr�sse der Bl�cke
			CvSize shiftSize = cvSize(12,12);						//Abstand der Bl�cke
			CvSize maxRange = cvSize(2,2);							//Suchbereich um die Bl�cke herum

			/*hier werden die Bilder die sp�ter x und y-Komponenten enthalten angelegt (siehe Seite 41 3.5.1.	Funktion cvCalcOpticalFlowBM)
			 */
			pyramid1 = cvCreateImage(cvSize((fs.width-blockSize.width)/shiftSize.width,(fs.height-blockSize.height)/shiftSize.height),IPL_DEPTH_32F,1);
			pyramid2 = cvCreateImage(cvSize((fs.width-blockSize.width)/shiftSize.width,(fs.height-blockSize.height)/shiftSize.height),IPL_DEPTH_32F,1);

			//berechnet den Optischen Fluss nach der Block Matching Methode und legt das Ergebnis f�r x-richtung in pyramid1
			//und f�r y-Richtung in pyramid2 ab
			cvCalcOpticalFlowBM( frame1_1C, frame2_1C, blockSize, shiftSize, maxRange, 0, pyramid1, pyramid2 );

			CvPoint d, b, p, q; // punkte zur berechnung des Vektors werden angelegt
			b.x = 0;
			b.y = 0;
			d.x = 0;
			d.y = 0;
			p.x = 0;
			p.y = 0;
			q.x = 0;
			q.y = 0;

			for(int i=0;i<pyramid1->height;i+=3) // Berechnung der Teilvektoren aus den Intensit�ten der Ausgngsbilder
				{
					b.y = i*blockSize.height; //height
					for(int j=0;j<pyramid1->width;j+=3)
					{
						b.x = j*blockSize.width; //width
						d.x = b.x + (int)( (( (float*)(pyramid1->imageData + i*pyramid1->widthStep) )[j] )*blockSize.width);
						d.y = b.y + (int)((( ( (float*)(pyramid2->imageData + i*pyramid2->widthStep) )[j] ))*blockSize.height);
						p.x=(p.x + b.x);//zusammenfassen der Teilvektoren zu einem Gesammtvektor
						p.y=(p.y + b.y);
						q.x=(q.x + d.x);
						q.y=(q.y + d.y);
					}
				}

				struct Pointer pointer; // Pointer f�r die Ausgabe
				pointer.p1 = q;
				pointer.p2 = p;
				return pointer; // Zur�ckgeben des Pointers
}

/*Diese Funktion berechnet den optischen Fuss nach der Lucas & Kanade Methode, mit Gaus-Pyramiden und Featuretracker
*/
struct Pointer FramesToPointerPyrLK( struct TwoFrames tf, CvSize fs, const int param){

			const int Feat_Count = 100; // Anzahl der Pfeile (Features)

			//Variablen f�r den Algorithmus
			static IplImage *frame1 = NULL, *frame1_1C = NULL, *frame2_1C = NULL, *eig_image = NULL, *temp_image = NULL, *pyramid1 = NULL, *pyramid2 = NULL;

			allocateOnDemand( &frame1_1C, fs, IPL_DEPTH_8U, 1 ); 	//Alokieren des Bildes frame1_C1
			cvConvertImage(tf.frame1, frame1_1C);					//es enh�lt das erste Bild in Schwarzweis
			
			allocateOnDemand( &frame2_1C, fs, IPL_DEPTH_8U, 1 ); //Alokieren des Bildes frame2_C1
			cvConvertImage(tf.frame2, frame2_1C);				//es enh�lt das zweite Bild in Schwarzweis

			allocateOnDemand( &eig_image, fs, IPL_DEPTH_32F, 1 );	//eig_image ist ein Bild in dem die gefundenen Features bagelegt werden
			allocateOnDemand( &temp_image, fs, IPL_DEPTH_32F, 1 );	//temp_image ist ein Bild in dem die gefundenen Features bagelegt werden

			CvPoint2D32f frame1_features[Feat_Count];				// Array in dem die Features des ersten Bildes abgelegt werden

			int number_of_features;// anzahl der Features
			int number_of_good_features;// anzahl der gefundenen Features

			number_of_features = Feat_Count;  // Setzen der Variablen auf den Anfangswert
			number_of_good_features = Feat_Count;

			// Sucht Features im ersten Bild (siehe Seite 33)
			cvGoodFeaturesToTrack(frame1_1C, eig_image, temp_image, frame1_features, &number_of_features, .01, .01, NULL, 3, 0.04, 0);
			
			int i; // Variable f�r die Markierung der gefundenen Features
			for (i = 0; i < Feat_Count; i++){ // Features werden mit einem Kreis markiert
				cvCircle(tf.frame1,cvPoint(frame1_features[i].x,frame1_features[i].y),1,cvScalar(0,250,250,0));
			}
			
			CvPoint2D32f frame2_features[Feat_Count]; // Array f�r die Koordinaten der Features im zweiten Bild

			char optical_flow_found_feature[Feat_Count]; // Status der Features (wenn gefunden, dann 1)

			float optical_flow_feature_error[Feat_Count]; // Status der nichtgefundenen Features (wenn nicht gefunden, dann 1)

			CvSize optical_flow_window = cvSize(10,10);  // Suchbereich um die Features herum

			CvTermCriteria optical_flow_termination_criteria //  Variablen zum Suchabbruch
				= cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 10, 3 );

			allocateOnDemand( &pyramid1, fs, IPL_DEPTH_8U, 1 ); // Bilder in denen die Pyramiden aufgebaut werden
			allocateOnDemand( &pyramid2, fs, IPL_DEPTH_8U, 1 );

			//berechnet den optischen Fluss nach Lucas & Kanade, mit Gaus-Pyramiden (siehe Seite 34)
			cvCalcOpticalFlowPyrLK(frame1_1C, frame2_1C, pyramid1, pyramid2, frame1_features, frame2_features, number_of_features, optical_flow_window,0, optical_flow_found_feature, optical_flow_feature_error, optical_flow_termination_criteria,0 );

			for( i = 0; i < number_of_features; i++) // gibt die gefundenen Features in der Konsole aus
			{
				printf("%i \t, %i \n", frame1_features[i].x ,frame2_features[i].x ); // gibt die gefundene x- und y-Komponente aus
			}

			CvPoint d, b; // Punkte zur Zusamenstellung des Vektors
			b.x = 0;
			b.y = 0;
			d.x = 0;
			d.y = 0;

			for( i = 0; i < number_of_features; i++)
			{
				/* If Pyramidal Lucas Kanade didn't really find the feature, skip it. */
				if (optical_flow_found_feature[i] == 0) {
					number_of_good_features--;// die aktuele Anzahl der Features wird ermittelt
					continue;
				}

				CvPoint p,q; // werte der Arrays werden ausgelesen
				p.x = (int) frame1_features[i].x;
				p.y = (int) frame1_features[i].y;
				q.x = (int) frame2_features[i].x;
				q.y = (int) frame2_features[i].y;

				// Punkte des Haupt Vektors werden zusammenaddiert
				b.x = b.x + p.x;
				b.y = b.y + p.y;
				d.x = d.x + q.x;
				d.y = d.y + q.y;
			}
			
			CvFont f = cvFont(0.7,1); // Srift zur Markierung der Frames
			char str[256] ;				// String f�r den Text
			sprintf(str, "PyrLK");			// Text
			cvPutText(tf.frame1, str, cvPoint(10, 40), &f, cvScalar(0, 0, 0, 0)); // Markiert das Bild

			struct Pointer pointer;// Vektor f�r die R�ckgabe
			pointer.p1 = b;
			pointer.p2 = d;
			return pointer; // Zur�ckgeben des Vektors
}

/*Diese Funktion berechnet den optischen Fuss nach der Lucas & Kanade Methode
*/
struct Pointer FramesToPointerLK( struct TwoFrames tf, CvSize fs, CvSize WS ){

			static IplImage *pyramid1 = NULL, *pyramid2 = NULL; // Bilder in denen die Werte des optischen Flusses abgelegt werden

			allocateOnDemand( &pyramid1, fs, IPL_DEPTH_32F, 1 ); // m�ssen IPL_DEPTH_32F sein !!!!
			allocateOnDemand( &pyramid2, fs, IPL_DEPTH_32F, 1 );
			
			// berechnet den optischen Fluss nach der Methode von Lucas & Kanade (siehe Seite 41)
			cvCalcOpticalFlowLK(tf.frame1,tf.frame2,WS,pyramid1,pyramid2);

			CvPoint d, b,p,q;// Punkte zur erfassung des optischen Flusses
			b.x = 0;
			b.y = 0;
			d.x = 0;
			d.y = 0;
			p.x = 0;
			p.y = 0;
			q.x = 0;
			q.y = 0;

			//cvShowImage("Optical Flow from Cam 2", pyramid1); // zeigt die ermittelten Bewegungsframes, in x- und 
			//cvShowImage("Optical Flow from Cam", pyramid2);	//y-Richtung 

			//auswertung der Velx und Vely Bilder (pyramid1 und pyramid2)!!!
			for(int i=0;i<pyramid1->height;i+=3) // so lange wie das Frame breit ist (in Pixel)
				{
					b.y = i; //height
					int j;
					for (j=0;j<pyramid1->width;j+=3) // so lange wie das Frame hoch ist (in Pixel)
					{
						b.x = j; //width
						d.x = b.x + (int)( ( (float*)(pyramid1->imageData + i*pyramid1->widthStep) )[j] ); // Erfasst die x-Komponente
						d.y = b.y + (int)( ( (float*)(pyramid2->imageData + i*pyramid2->widthStep) )[j] ); // Erfasst die y-Komponente
						p.x=(p.x + b.x); // Bildung des Hauptvektors aus den Teilvektoren
						p.y=(p.y + b.y);
						q.x=(q.x + d.x);
						q.y=(q.y + d.y);
					}
				}

			CvFont f = cvFont(0.7,1);   // Srift zur Markierung der Frames
			char str[256] ;				// String f�r den Text
			sprintf(str, "LK");			// Text
			cvPutText(tf.frame1, str, cvPoint(10, 40), &f, cvScalar(0, 0, 0, 0)); // Markiert das Bild
			
			struct Pointer pointer; // Vektor f�r die R�ckgabe
			pointer.p1 = p;
			pointer.p2 = q;
			return pointer; // Zur�ckgeben des Vektors
}

/*Diese Funktion berechnet den optischen Fuss nach der Methode von Horn & Schunck
*/
struct Pointer FramesToPointerHS( struct TwoFrames tf, CvSize fs, const float param){
			
			// Bilder mit Hilfe dessen der optische Fluss berechnet wird
			static IplImage *frame1 = NULL, *frame1_1C = NULL, *frame2_1C = NULL, *eig_image = NULL, *temp_image = NULL, *pyramid1 = NULL, *pyramid2 = NULL;
			// Bildung des Terminationcriteria (weitere M�glichkeit das Criteria zu bilden)
			CvTermCriteria criteria;
			criteria.epsilon = 2; // Epsilonwert
			criteria.max_iter = 10; // Anzahl der Suchwiderhollungen
			criteria.type = 1; // Typ des Criteria

			allocateOnDemand( &pyramid1, fs, IPL_DEPTH_32F, 1 ); // Platz f�r die Ergebnisserfassung wird allokiert / x-Komponente
			allocateOnDemand( &pyramid2, fs, IPL_DEPTH_32F, 1 ); //y-Komponente
			int usePrevious = 1; //Sagt aus ob die vorausgegangenen Ergebnisse ber�cksichtigt werden sollen (bei 1 ja, sonst nein)
			float lambda = param; // Rauschfaktor // kriegt seinen wert von der Testvariable 

			// berechnet den optischen Fluss nach der Methode von Horn & Schunck (siehe Seite 42)
			cvCalcOpticalFlowHS(tf.frame1,tf.frame2,usePrevious,pyramid1,pyramid2,lambda,criteria);

			CvPoint d, b,p,q; // Punkte zur Zusamenstellung des Vektors
			b.x = 0;
			b.y = 0;
			d.x = 0;
			d.y = 0;
			p.x = 0;
			p.y = 0;
			q.x = 0;
			q.y = 0;

			cvShowImage("Optical Flow from Cam 2", pyramid1); // zeigt die ermittelten Bewegungsframes, in x- und y-Richtung 
			cvShowImage("Optical Flow from Cam", pyramid2);

			//auswertung der Velx und Vely Bilder (pyramid1 und pyramid2) !!!
			for(int i=0;i<pyramid1->height;i+=3) // so lange wie das Frame hoch ist (in Pixel)
				{
					b.y = i; //height

					int j;
					for (j=0;j<pyramid1->width;j+=3) // so lange wie das Frame breit ist (in Pixel)
					{
						b.x = j; //width
						d.x = b.x + (int)( ( (float*)(pyramid1->imageData + i*pyramid1->widthStep) )[j] ); // Erfasst die x-Komponente
						d.y = b.y + (int)( ( (float*)(pyramid2->imageData + i*pyramid2->widthStep) )[j] ); // Erfasst die y-Komponente
						p.x=(p.x + b.x); // Bildung des Hauptvektors aus den Teilvektoren
						p.y=(p.y + b.y);
						q.x=(q.x + d.x);
						q.y=(q.y + d.y);
							
					}
				}
			CvFont f = cvFont(0.7,1);   // Srift zur Markierung der Frames
			char str[256] ;				// String f�r den Text
			sprintf(str, "HS");			// Text
			cvPutText(tf.frame1, str, cvPoint(10, 40), &f, cvScalar(0, 0, 0, 0)); // Markiert das Bild

			struct Pointer pointer; // Vektor f�r die R�ckgabe 
			pointer.p1 = p;
			pointer.p2 = q;
			return pointer; // Zur�ckgeben des Vektors
}

	//Fuktion die anhand der l�nge des Vektors eine Dauer in mS ausgeben soll, um optimal den Flow zu erkennen
	int lenghtToLooptime(int lenght){
		/*if (lenght < 10) return 500;
		if (20 > lenght > 10) return 250;
		else return 125; */
		if (lenght < 5) return 250;
		if (10 > lenght && lenght> 5) return 125;
		else return 60;
	}

	// Diese Funktion berechnet us den drei �bergebenen Vektoren, einen Vektor, in den der erste Vektor zu einem Drittel miteinberechnet wird
	// der zweite zur H�lfte und der dritte einfach.
	struct Pointer CalcTriPtr (struct Pointer ptr1,struct Pointer ptr2,struct Pointer ptr3){
		struct Pointer ptr_x; // Vektor v�r die R�ckgabe
		ptr_x.p1.x= 0; // Anfang des Vektors wird auf Null gesetzt
		ptr_x.p1.y= 0;
		ptr_x.p2.x= (LaengePtrX(ptr1)*3 + LaengePtrX(ptr2)*2 + LaengePtrX(ptr3))/6; // Spitze wird aus den L�ngen der �bergebenen
		ptr_x.p2.y= (LaengePtrY(ptr1)*3 + LaengePtrY(ptr2)*2 + LaengePtrY(ptr3))/6; // Vektoren zusammen gesetzt
		return ptr_x; // Der fertige Vektor wird zur�ckgegeben
	}

// Holt pro while-Durchgang zwei Bilder, und ubergibt sie in die Flussberechnung
	int FrameWork_1(){
	
printf("Start FW 2\n"); // Konsolenausgabe zur Fortsrittsverfolgung
    int p[3];// Kriterien-Array zur Speicherung der Bilder
	p[0] = CV_IMWRITE_PNG_COMPRESSION; //CV_IMWRITE_PNG_COMPRESSION; CV_IMWRITE_JPEG_QUALITY; CV_IMWRITE_PXM_BINARY // Drei untersiedliche M�glichkeiten Bilder abzulegen PNG-, JPG-Qualit�t (siehe Seite 57)
    p[1] = 0;
    p[2] = 0;

	// Variablen zur Steuerung der Frameworks
    int LOOPtime = 100; // Dauer eines Durchgangs
	//double poineter_scale = 0.3;
	int loop = 1;
	//int wait_time = 1;

	//Vareablen zur Textausgabe
	CvFont Font_= cvFont(0.5,1); // Srift
	char str[256]; // Arrays zur Text�bergabe
	char str2[256];

	CvCapture *input_video; // Anlegen der Framequelle 
	input_video  = cvCaptureFromCAM(0); // Definieren der Framequelle
	if (input_video == NULL) // �berpr�ft ob die Framequelle korrekt arbeitet
	{
		/* Either the video didn't exist OR it uses a codec OpenCV
		 * doesn't support.
		 */
		fprintf(stderr, "Error: No Camera found.\n");
		return -1;
	}
	printf("Cam AN \n");// Konsolenausgabe zur Fortsrittsverfolgung

	CvSize frame_size; // Gr��e der Framequelle
	frame_size.height =	(int) cvGetCaptureProperty( input_video, CV_CAP_PROP_FRAME_HEIGHT ); // H�he
	frame_size.width =	(int) cvGetCaptureProperty( input_video, CV_CAP_PROP_FRAME_WIDTH );  // Breite

	cvNamedWindow("Optical Flow from Cam 1", 0); // Fenster zur anzeige der Ergebnisse werden erstellt
	cvNamedWindow("Optical Flow from Cam 2", 0);

	printf("Fenster AN \n");// Konsolenausgabe zur Fortsrittsverfolgung

	//zeiten fur die ausgabe
	int t_0 = 0, t_1;

		//Schreibt das Video mit 
	// Pfad zum Video
	char VideoPath[256] = "C:\\Dokumente und Einstellungen\\root\\Desktop\\CV_HS_Auswertung\\images\\Mitschnitt.mpeg";
	// Pfad zur Bildablage
	char BildPath[256] = "C:\\Dokumente und Einstellungen\\root\\Desktop\\CV_HS_Auswertung\\images\\Bild.jpg";
	// Pfad zur Ergebnisserfassung
	char TextPath[256] = "C:\\Dokumente und Einstellungen\\root\\Desktop\\CV_HS_Auswertung\\images\\Werte.txt";
	//Writer zum zusammenstellen des Videos
	CvVideoWriter * VW = cvCreateVideoWriter(VideoPath,CV_FOURCC('P','I','M','1'),20,frame_size);
	
	printf("Writer eingerichtet\n");// Konsolenausgabe zur Fortsrittsverfolgung
	
	//Strukturen f�r die Frames
	struct TwoFrames Two_F; // Frames zum Hollen der der Bilder 
	struct TwoFrames Two_F2; // Frames zum berechnen des Flusses 
	IplImage * RecordFrame; // Frame zum Schreiben ins Video
	
	// Allokieren der Frames
	RecordFrame = cvCreateImage(frame_size, IPL_DEPTH_8U,3); // Zum sreiben des Videofiles mussen die Bilder diese Einstellungen besitzen
	Two_F.frame1 =  cvCreateImage( frame_size, IPL_DEPTH_8U, 1 ); 
	Two_F.frame2 =  cvCreateImage( frame_size, IPL_DEPTH_8U, 1 );
	Two_F2.frame1 =  cvCreateImage( frame_size, IPL_DEPTH_8U, 1 );
	Two_F2.frame2 =  cvCreateImage( frame_size, IPL_DEPTH_8U, 1 );

	struct Pointer ptr_HS, ptr_LK, ptr_PyrLK ,ptr_BM; // Vektoren f�r jede Art der Flusserkennung
	//ptr_v, ptr_vv; // Vektoren zur Berechnung des Duchsnitsvektors (f�r die Funktion CalcTriPtr)

//	ptr_v.p1.x=ptr_v.p1.y=ptr_v.p2.x=ptr_v.p2.y=0; // Routine der drei Vektoren
//	ptr_vv.p1.x=ptr_vv.p1.y=ptr_vv.p2.x=ptr_vv.p2.y=0; // Vektoren werden immer einz nach hinten geschoben

	// Datei zum Aufnehmen der Daten
	FILE *datei;
	datei = fopen (TextPath, "w");
	// erste Zeile der TXT-Datei wird angeh�ngt (die Spalten�berschriften)
	fprintf(datei,"Loop;Zeit[ms];L�nge_HS;L�ngeX_HS;L�ngeY_HS;L�nge_LK;L�ngeX_LK;L�ngeY_LK;L�nge_PyrLK;L�ngeX_PyrLK;L�ngeY_PyrLK;L�nge_BM;L�ngeX_BM;L�ngeY_BM;LK_winSize;t_HS;t_LK;t_PyrLK;t_BM\n");

	//Zeitsteuerung �ber clock()
	clock_t start, start2, end; // Variablen zur erfassung der Zeiten
	start2 = clock();			// Der erste Zeitpunkt wird erfasst

	// Variablen zur Steuerung des Frameworks
	int RecordFlag; // Flag zur aufnahme der Daten und zum Schreiben des Videos
	int i = 0;		// Counter zum Z�hlen der Durchg�nge
	char c = NULL;	// Variable zur erfassung der eingaben
	int lk_F= 0;	// Steuervariable zur Variirung der Parameter
	int lk_S= 2;	// erste Parametervariable
	float param = 2;// zweite Parametervariable

	while(c != 'q'){

		printf("WHILE  ");

		start = clock(); // Z�hlt cpu-Ticks, ungefair 1 000 000 pro Sekunde

		Two_F.frame1 = cvQueryFrame( input_video ); //holt ein Bild von der Kamera
		cvSaveImage(BildPath ,Two_F.frame1, p); // speichert und l�d das bild wieder
		Two_F.frame1 = cvLoadImage( BildPath,0); // so verschlechtert sich die qualit�t

		//cvWaitKey(LOOPtime/2);

		Two_F.frame2 = cvQueryFrame( input_video ); //holt ein Bild von der Kamera
		cvSaveImage(BildPath ,Two_F.frame2, p); // speichert und l�d das bild wieder
		Two_F.frame2 = cvLoadImage( BildPath, 0); // so verschlechtert sich die qualit�t
		end = clock();
		sprintf(str2, "Cam: %i ", ((end - start)));
		start = clock();

		//		allocateOnDemand( &frame2_1C, fs, IPL_DEPTH_8U, 1 );
		//		cvConvertImage(tf.frame2, frame2_1C, CV_CVTIMG_FLIP);

		//Variirung der Parameter
		if(lk_F<50 && RecordFlag == 1){ // beginnt wenn das Recordflag gesetztist, und wartet 50 Durchg�nge bis zum erh�hen des Parameters
			lk_F++;	
		}else if ( RecordFlag == 1){ // nach 50 Dirchg�ngen der While-Schleife, Ver�ndert die Variablen
			lk_F= 0;
			lk_S = lk_S +1 ;
			param = param - 0.005; // die Variablen k�nnen beliebig Ver�ndert werden
		}

		cvConvertImage(Two_F.frame1, Two_F2.frame1,0);
		cvConvertImage(Two_F.frame2, Two_F2.frame2,0);

//		cvShowImage("Optical Flow from Cam 1", Two_F2.frame1);
//		cvShowImage("Optical Flow from Cam 2", Two_F2.frame2);

		int t_HS, t_LK, t_PyrLK, t_BM; // Zeiten f�r die Algorithmen

		//diese Funktionen verwandeln zwei Bilder in einen 2D-Richtungsvektor
		start = clock();
		ptr_HS = FramesToPointerHS(Two_F2,frame_size,0.01);			//mit dem HS OF Allgoritm  "cvCalcOpticalFlowHS"
		t_HS = clock() - start;
		start = clock();
		ptr_PyrLK = FramesToPointerPyrLK(Two_F2,frame_size,lk_S);		//mit dem LK OF Allgoritm, mit Gausspyramiden  "cvCalcOpticalFlowPyrLK"
		t_PyrLK = clock() - start;
		start = clock();
		ptr_BM= FramesToPointerBM(Two_F2,frame_size,lk_S);				//mit dem BM OF Allgoritm  "cvCalcOpticalFlowBM"
		t_BM = clock() - start;
		start = clock();
		ptr_LK= FramesToPointerLK(Two_F2,frame_size,cvSize(5,5));				//mit dem LK OF Allgoritm  "cvCalcOpticalFlowLK"
		t_LK = clock() - start;

		//neues Bild wird zum alten
		cvConvertImage(Two_F.frame1,Two_F.frame2);
		
		// winkel des Vektors wird berechnet
		double angle;		angle = atan2( (double) ptr_BM.p1.y - ptr_BM.p2.y, (double) ptr_BM.p1.x - ptr_BM.p2.x );
		
		// Vektor wird in das bild gezeichnet
		FlowPointer(Two_F2.frame1,ptr_BM,frame_size, angle);
		
		end = clock(); // Zeit des Loops wird erfasst
		sprintf(str, "Alg: %i ", ((end - start))); // Zeit des Loops wird berechnet
		cvPutText(Two_F2.frame1, str, cvPoint(10, 10), &Font_, cvScalar(0, 0, 0, 0)); // ins Bild wird die Dauer des Loops eingef�gt
		cvPutText(Two_F2.frame1, str2, cvPoint(10, 20), &Font_, cvScalar(0, 0, 0, 0)); // ins Bild wird die Dauer eingef�gt, die die Kamere brauch um ein Frame zu hollen
		sprintf(str, "Parameter: %i ", lk_S); 
		cvPutText(Two_F2.frame1, str, cvPoint(10, 40), &Font_, cvScalar(0, 0, 0, 0));// die Variable der Parametersteuerung wird im Bild ausgegeben
		printf(str);
		printf("\tuSec \tX=%i, \tY=%i\n", (ptr_HS.p1.x - ptr_HS.p2.x), (ptr_HS.p1.y - ptr_HS.p2.y));// Konsolenausgabe zur Fortsrittsverfolgung
		cvShowImage("Optical Flow from Cam 1", Two_F2.frame1); // Das Bild wird angezeigt
		
		if (c == 'r') // Mit dem Dr�cken der Taste 'r' wird die Aufnahme gestartet
			RecordFlag = 1; // Aufnahme wird beim Setzen des RecordFlags gestartet

		if (RecordFlag == 1){		// Wenn die Aufnahme des Tests l�uft 
		i++; // Aufgenomene Loops werden gez�hlt
		cvConvertImage(Two_F2.frame1,RecordFrame); // Videoframe �bernimt das Bild um es in das video zu setzen
		cvWriteFrame(VW,RecordFrame); // Zetzt das aktuelle Frame ins Video
		fprintf(datei,"%i;%i;%f;%i;%i;%f;%i;%i;%f;%i;%i;%f;%i;%i;%i;%i;%i;%i;%i\n" // Testergebnisse werden in die TXT-Datei geschrieben
			,i,clock()-start2,
			LaengePtr(ptr_HS),LaengePtrX(ptr_HS),LaengePtrY(ptr_HS),
			LaengePtr(ptr_LK),LaengePtrX(ptr_LK),LaengePtrY(ptr_LK),
			LaengePtr(ptr_PyrLK),LaengePtrX(ptr_PyrLK),LaengePtrY(ptr_PyrLK),
			LaengePtr(ptr_BM),LaengePtrX(ptr_BM),LaengePtrY(ptr_BM),
			lk_S,t_HS, t_LK, t_PyrLK, t_BM);
		}
		
		c=cvWaitKey(2); // Wartet auf die Eingabe 
	}
		// das Programm wird beendet 	
	cvReleaseCapture(&input_video); // Kameraverbindung wird getrennt
	cvDestroyAllWindows(); // Fenster werden die Bilder angezeigt haben werden geschlo�en
	fclose (datei); // Verbindung zur TXT-Datei wird beendet
	cvReleaseVideoWriter(&VW); // VideoWriter wird beendet
	exit(0); // Programm wird verlassen
	
	}
	
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// Holt pro while-Durchgang ein Bild, und ubergibt das mit dem vorhergegangenem Bild in die Flussberechnung
	int FrameWork_2(){
	printf("Start FW 2\n"); // Konsolenausgabe zur Fortsrittsverfolgung
    int p[3];// Kriterien-Array zur Speicherung der Bilder
	p[0] = CV_IMWRITE_PNG_COMPRESSION; //CV_IMWRITE_PNG_COMPRESSION; CV_IMWRITE_JPEG_QUALITY; CV_IMWRITE_PXM_BINARY // Drei untersiedliche M�glichkeiten Bilder abzulegen PNG-, JPG-Qualit�t (siehe Seite 57)
    p[1] = 0;
    p[2] = 0;

	// Variablen zur Steuerung der Frameworks
    int LOOPtime = 100; // Dauer eines Durchgangs
	//double poineter_scale = 0.3;
	int loop = 1;
	//int wait_time = 1;

	//Vareablen zur Textausgabe
	CvFont Font_= cvFont(0.5,1); // Srift
	char str[256]; // Arrays zur Text�bergabe
	char str2[256];

	CvCapture *input_video; // Anlegen der Framequelle 
	input_video  = cvCaptureFromCAM(0); // Definieren der Framequelle
	if (input_video == NULL) // �berpr�ft ob die Framequelle korrekt arbeitet
	{
		/* Either the video didn't exist OR it uses a codec OpenCV
		 * doesn't support.
		 */
		fprintf(stderr, "Error: No Camera found.\n");
		return -1;
	}
	printf("Cam AN \n");// Konsolenausgabe zur Fortsrittsverfolgung

	CvSize frame_size; // Gr��e der Framequelle
	frame_size.height =	(int) cvGetCaptureProperty( input_video, CV_CAP_PROP_FRAME_HEIGHT ); // H�he
	frame_size.width =	(int) cvGetCaptureProperty( input_video, CV_CAP_PROP_FRAME_WIDTH );  // Breite

	cvNamedWindow("Optical Flow from Cam 1", 0); // Fenster zur anzeige der Ergebnisse werden erstellt
	cvNamedWindow("Optical Flow from Cam 2", 0);

	printf("Fenster AN \n");// Konsolenausgabe zur Fortsrittsverfolgung

	//zeiten fur die ausgabe
	int t_0 = 0, t_1;
	
	//Schreibt das Video mit 
	// Pfad zum Video
	char VideoPath[256] = "C:\\Dokumente und Einstellungen\\root\\Desktop\\CV_HS_Auswertung\\images\\Mitschnitt.mpeg";
	// Pfad zur Bildablage
	char BildPath[256] = "C:\\Dokumente und Einstellungen\\root\\Desktop\\CV_HS_Auswertung\\images\\Bild.jpg";
	// Pfad zur Ergebnisserfassung
	char TextPath[256] = "C:\\Dokumente und Einstellungen\\root\\Desktop\\CV_HS_Auswertung\\images\\Werte.txt";
	//Writer zum zusammenstellen des Videos
	CvVideoWriter * VW = cvCreateVideoWriter(VideoPath,CV_FOURCC('P','I','M','1'),20,frame_size);
	
	printf("Writer eingerichtet\n");// Konsolenausgabe zur Fortsrittsverfolgung
	
	//Strukturen f�r die Frames
	struct TwoFrames Two_F; // Frames zum Hollen der der Bilder 
	struct TwoFrames Two_F2; // Frames zum berechnen des Flusses 
	IplImage * RecordFrame; // Frame zum Schreiben ins Video
	
	// Allokieren der Frames
	RecordFrame = cvCreateImage(frame_size, IPL_DEPTH_8U,3); // Zum sreiben des Videofiles mussen die Bilder diese Einstellungen besitzen
	Two_F.frame1 =  cvCreateImage( frame_size, IPL_DEPTH_8U, 1 ); 
	Two_F.frame2 =  cvCreateImage( frame_size, IPL_DEPTH_8U, 1 );
	Two_F2.frame1 =  cvCreateImage( frame_size, IPL_DEPTH_8U, 1 );
	Two_F2.frame2 =  cvCreateImage( frame_size, IPL_DEPTH_8U, 1 );

	struct Pointer ptr_HS, ptr_LK, ptr_PyrLK ,ptr_BM; // Vektoren f�r jede Art der Flusserkennung
	//ptr_v, ptr_vv; // Vektoren zur Berechnung des Duchsnitsvektors (f�r die Funktion CalcTriPtr)

//	ptr_v.p1.x=ptr_v.p1.y=ptr_v.p2.x=ptr_v.p2.y=0; // Routine der drei Vektoren
//	ptr_vv.p1.x=ptr_vv.p1.y=ptr_vv.p2.x=ptr_vv.p2.y=0; // Vektoren werden immer einz nach hinten geschoben

	// Datei zum Aufnehmen der Daten
	FILE *datei;
	datei = fopen (TextPath, "w");
	// erste Zeile der TXT-Datei wird angeh�ngt (die Spalten�berschriften)
	fprintf(datei,"Loop;Zeit[ms];L�nge_HS;L�ngeX_HS;L�ngeY_HS;L�nge_LK;L�ngeX_LK;L�ngeY_LK;L�nge_PyrLK;L�ngeX_PyrLK;L�ngeY_PyrLK;L�nge_BM;L�ngeX_BM;L�ngeY_BM;LK_winSize;t_HS;t_LK;t_PyrLK;t_BM\n");

	//Zeitsteuerung �ber clock()
	clock_t start, start2, end; // Variablen zur erfassung der Zeiten
	start2 = clock();			// Der erste Zeitpunkt wird erfasst

	// Variablen zur Steuerung des Frameworks
	int RecordFlag; // Flag zur aufnahme der Daten und zum Schreiben des Videos
	int i = 0;		// Counter zum Z�hlen der Durchg�nge
	char c = NULL;	// Variable zur erfassung der eingaben
	int lk_F= 0;	// Steuervariable zur Variirung der Parameter
	int lk_S= 2;	// erste Parametervariable
	float param = 2;// zweite Parametervariable

	Two_F.frame2 = cvQueryFrame( input_video ); //holt das erste Bild von der Kamera
	cvSaveImage(BildPath ,Two_F.frame2, p); // speichert und l�d das bild wieder
	Two_F.frame2 = cvLoadImage( BildPath,0); // so verschlechtert sich die qualit�t

	// Whilw-Schleife beginnt
	while(c != 'q'){ // Arbeitet solange bis man 'q' eingibt

		printf("WHILE  "); // Konsolenausgabe zur Fortsrittsverfolgung

		start = clock(); // Z�hlt cpu-Ticks, ungefair 1 000 pro Sekunde

		Two_F.frame1 = cvQueryFrame( input_video ); //holt ein Bild von der Kamera
		cvSaveImage(BildPath ,Two_F.frame1, p); // speichert und l�d das bild wieder
		Two_F.frame1 = cvLoadImage( BildPath, 0); // so verschlechtert sich die qualit�t
		
		end = clock(); // Erfasst die zeit die f�rs hollen des Frames notwendig war
		sprintf(str2, "Cam: %i ", ((end - start))); // gibt die Zeit in der Konsole aus
		start = clock(); // F�ngt die N�chste Zeitmessung an

		//Variirung der Parameter
		if(lk_F<50 && RecordFlag == 1){ // beginnt wenn das Recordflag gesetztist, und wartet 50 Durchg�nge bis zum erh�hen des Parameters
			lk_F++;	
		}else if ( RecordFlag == 1){ // nach 50 Dirchg�ngen der While-Schleife, Ver�ndert die Variablen
			lk_F= 0;
			lk_S = lk_S +1 ;
			param = param - 0.005; // die Variablen k�nnen beliebig Ver�ndert werden
		}

		cvConvertImage(Two_F.frame1, Two_F2.frame1,0);
		cvConvertImage(Two_F.frame2, Two_F2.frame2,0);

//		cvShowImage("Optical Flow from Cam 1", Two_F2.frame1);
//		cvShowImage("Optical Flow from Cam 2", Two_F2.frame2);

		int t_HS, t_LK, t_PyrLK, t_BM; // Zeiten f�r die Algorithmen

		//diese Funktionen verwandeln zwei Bilder in einen 2D-Richtungsvektor
		start = clock();
		ptr_HS = FramesToPointerHS(Two_F2,frame_size,0.01);			//mit dem HS OF Allgoritm  "cvCalcOpticalFlowHS"
		t_HS = clock() - start;
		start = clock();
		ptr_PyrLK = FramesToPointerPyrLK(Two_F2,frame_size,lk_S);		//mit dem LK OF Allgoritm, mit Gausspyramiden  "cvCalcOpticalFlowPyrLK"
		t_PyrLK = clock() - start;
		start = clock();
		ptr_BM= FramesToPointerBM(Two_F2,frame_size,lk_S);				//mit dem BM OF Allgoritm  "cvCalcOpticalFlowBM"
		t_BM = clock() - start;
		start = clock();
		ptr_LK= FramesToPointerLK(Two_F2,frame_size,cvSize(5,5));				//mit dem LK OF Allgoritm  "cvCalcOpticalFlowLK"
		t_LK = clock() - start;

		//neues Bild wird zum alten
		cvConvertImage(Two_F.frame1,Two_F.frame2);
		
		// winkel des Vektors wird berechnet
		double angle;		angle = atan2( (double) ptr_BM.p1.y - ptr_BM.p2.y, (double) ptr_BM.p1.x - ptr_BM.p2.x );
		
		// Vektor wird in das bild gezeichnet
		FlowPointer(Two_F2.frame1,ptr_BM,frame_size, angle);
		
		end = clock(); // Zeit des Loops wird erfasst
		sprintf(str, "Alg: %i ", ((end - start))); // Zeit des Loops wird berechnet
		cvPutText(Two_F2.frame1, str, cvPoint(10, 10), &Font_, cvScalar(0, 0, 0, 0)); // ins Bild wird die Dauer des Loops eingef�gt
		cvPutText(Two_F2.frame1, str2, cvPoint(10, 20), &Font_, cvScalar(0, 0, 0, 0)); // ins Bild wird die Dauer eingef�gt, die die Kamere brauch um ein Frame zu hollen
		sprintf(str, "Parameter: %i ", lk_S); 
		cvPutText(Two_F2.frame1, str, cvPoint(10, 40), &Font_, cvScalar(0, 0, 0, 0));// die Variable der Parametersteuerung wird im Bild ausgegeben
		printf(str);
		printf("\tuSec \tX=%i, \tY=%i\n", (ptr_HS.p1.x - ptr_HS.p2.x), (ptr_HS.p1.y - ptr_HS.p2.y));// Konsolenausgabe zur Fortsrittsverfolgung
		cvShowImage("Optical Flow from Cam 1", Two_F2.frame1); // Das Bild wird angezeigt
		
		if (c == 'r') // Mit dem Dr�cken der Taste 'r' wird die Aufnahme gestartet
			RecordFlag = 1; // Aufnahme wird beim Setzen des RecordFlags gestartet

		if (RecordFlag == 1){		// Wenn die Aufnahme des Tests l�uft 
		i++; // Aufgenomene Loops werden gez�hlt
		cvConvertImage(Two_F2.frame1,RecordFrame); // Videoframe �bernimt das Bild um es in das video zu setzen
		cvWriteFrame(VW,RecordFrame); // Zetzt das aktuelle Frame ins Video
		fprintf(datei,"%i;%i;%f;%i;%i;%f;%i;%i;%f;%i;%i;%f;%i;%i;%i;%i;%i;%i;%i\n" // Testergebnisse werden in die TXT-Datei geschrieben
			,i,clock()-start2,
			LaengePtr(ptr_HS),LaengePtrX(ptr_HS),LaengePtrY(ptr_HS),
			LaengePtr(ptr_LK),LaengePtrX(ptr_LK),LaengePtrY(ptr_LK),
			LaengePtr(ptr_PyrLK),LaengePtrX(ptr_PyrLK),LaengePtrY(ptr_PyrLK),
			LaengePtr(ptr_BM),LaengePtrX(ptr_BM),LaengePtrY(ptr_BM),
			lk_S,t_HS, t_LK, t_PyrLK, t_BM);
		}
		
		c=cvWaitKey(2); // Wartet auf die Eingabe 
	}
		// das Programm wird beendet 	
	cvReleaseCapture(&input_video); // Kameraverbindung wird getrennt
	cvDestroyAllWindows(); // Fenster werden die Bilder angezeigt haben werden geschlo�en
	fclose (datei); // Verbindung zur TXT-Datei wird beendet
	cvReleaseVideoWriter(&VW); // VideoWriter wird beendet
	exit(0); // Programm wird verlassen
	
	}


int main(void)
{	
	//FrameWork_1();	// holt zwei Bilder und berechnet aus den, den optischen Fluss
	FrameWork_2();	//holt imer jeweils ein Bild und berechnet den Fluss zwischen dem aktuellen und dem vorherigem Bild
	return 0;
}
