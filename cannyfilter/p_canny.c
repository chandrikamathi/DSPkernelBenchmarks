/*"Canny" edge detector code:
---------------------------

This text file contains the source code for a "Canny" edge detector. It
was written by Mike Heath (heath@csee.usf.edu) using some pieces of a
Canny edge detector originally written by someone at Michigan State
University.

There are three 'C' source code files in this text file. They are named
"canny_edge.c", "hysteresis.c" and "pgm_io.c". They were written and compiled
under SunOS 4.1.3. Since then they have also been compiled under Solaris.
To make an executable program: (1) Separate this file into three files with
the previously specified names, and then (2) compile the code using

  gcc -o canny_edge canny_edge.c hysteresis.c pgm_io.c -lm
  (Note: You can also use optimization such as -O3)

The resulting program, canny_edge, will process images in the PGM format.
Parameter selection is left up to the user. A broad range of parameters to
use as a starting point are: sigma 0.60-2.40, tlow 0.20-0.50 and,
thigh 0.60-0.90.

If you are using a Unix system, PGM file format conversion tools can be found
at ftp://wuarchive.wustl.edu/graphics/graphics/packages/pbmplus/.
Otherwise, it would be easy for anyone to rewrite the image I/O procedures
because they are listed in the separate file pgm_io.c.

If you want to check your compiled code, you can download grey-scale and edge
images from http://marathon.csee.usf.edu/edge/edge_detection.html. You can use
the parameters given in the edge filenames and check whether the edges that
are output from your program match the edge images posted at that address.

Mike Heath
(10/29/96)

/*******************************************************************************
* --------------------------------------------
*(c) 2001 University of South Florida, Tampa
* Use, or copying without permission prohibited.
* PERMISSION TO USE
* In transmitting this software, permission to use for research and
* educational purposes is hereby granted.  This software may be copied for
* archival and backup purposes only.  This software may not be transmitted
* to a third party without prior permission of the copyright holder. This
* permission may be granted only by Mike Heath or Prof. Sudeep Sarkar of
* University of South Florida (sarkar@csee.usf.edu). Acknowledgment as
* appropriate is respectfully requested.
* 
*  Heath, M., Sarkar, S., Sanocki, T., and Bowyer, K. Comparison of edge
*    detectors: a methodology and initial study, Computer Vision and Image
*    Understanding 69 (1), 38-54, January 1998.
*  Heath, M., Sarkar, S., Sanocki, T. and Bowyer, K.W. A Robust Visual
*    Method for Assessing the Relative Performance of Edge Detection
*    Algorithms, IEEE Transactions on Pattern Analysis and Machine
*    Intelligence 19 (12),  1338-1359, December 1997.
*  ------------------------------------------------------
*
* PROGRAM: canny_edge
* PURPOSE: This program implements a "Canny" edge detector. The processing
* steps are as follows:
*
*   1) Convolve the image with a separable gaussian filter.
*   2) Take the dx and dy the first derivatives using [-1,0,1] and [1,0,-1]'.
*   3) Compute the magnitude: sqrt(dx*dx+dy*dy).
*   4) Perform non-maximal suppression.
*   5) Perform hysteresis.
*
* The user must input three parameters. These are as follows:
*
*   sigma = The standard deviation of the gaussian smoothing filter.
*   tlow  = Specifies the low value to use in hysteresis. This is a 
*           fraction (0-1) of the computed high threshold edge strength value.
*   thigh = Specifies the high value to use in hysteresis. This fraction (0-1)
*           specifies the percentage point in a histogram of the gradient of
*           the magnitude. Magnitude values of zero are not counted in the
*           histogram.
*
* NAME: Mike Heath
*       Computer Vision Laboratory
*       University of South Floeida
*       heath@csee.usf.edu
*
* DATE: 2/15/96
*
* Modified: 5/17/96 - To write out a floating point RAW headerless file of
*                     the edge gradient "up the edge" where the angle is
*                     defined in radians counterclockwise from the x direction.
*                     (Mike Heath)
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include "omp.h"


#define VERBOSE 0
#define BOOSTBLURFACTOR 90.0
long long int total_stats;
long long int stats[240];

double timerval() {
	struct timeval st_time;
	gettimeofday(&st_time, NULL);

	return st_time.tv_sec + st_time.tv_usec*pow(10, -6);
}

void canny(unsigned char *image, int rows, int cols, float sigma,
         float tlow, float thigh, unsigned char **edge, char *fname);
void gaussian_smooth(unsigned char *image, int rows, int cols, float sigma,
        short int **smoothedim);
void make_gaussian_kernel(float sigma, float **kernel, int *windowsize);
void derrivative_x_y(short int *smoothedim, int rows, int cols,
        short int **delta_x, short int **delta_y);
void magnitude_x_y(short int *delta_x, short int *delta_y, int rows, int cols,
        short int **magnitude);
void apply_hysteresis(short int *mag, unsigned char *nms, int rows, int cols,
        float tlow, float thigh, unsigned char *edge);
void radian_direction(short int *delta_x, short int *delta_y, int rows,
    int cols, float **dir_radians, int xdirtag, int ydirtag);
double angle_radians(double x, double y);

main(int argc, char *argv[])
{
   
   char *infilename = NULL;  /* Name of the input image */
   char *dirfilename = NULL; /* Name of the output gradient direction image */
   char outfilename[128];    /* Name of the output "edge" image */
   char composedfname[128];  /* Name of the output "direction" image */
   //unsigned char *image;     /* The input image */
   unsigned char *edge;      /* The output edge image */
   int rows, cols, i, j;           /* The dimensions of the image. */
   float sigma,              /* Standard deviation of the gaussian kernel. */
	 tlow,               /* Fraction of the high threshold in hysteresis. */
	 thigh;              /* High hysteresis threshold control. The actual
			        threshold is the (100 * thigh) percentage point
			        in the histogram of the magnitude of the
			        gradient image that passes non-maximal
			        suppression. */
   double st, et;
   /****************************************************************************
   * Get the command line arguments.
   ****************************************************************************/
  /* if(argc < 1){
   fprintf(stderr,"\n<USAGE> %s image sigma tlow thigh [writedirim]\n",argv[0]);
      fprintf(stderr,"\n      image:      An image to process. Must be in ");
      fprintf(stderr,"PGM format.\n");
      fprintf(stderr,"      sigma:      Standard deviation of the gaussian");
      fprintf(stderr," blur kernel.\n");
      fprintf(stderr,"      tlow:       Fraction (0.0-1.0) of the high ");
      fprintf(stderr,"edge strength threshold.\n");
      fprintf(stderr,"      thigh:      Fraction (0.0-1.0) of the distribution");
      fprintf(stderr," of non-zero edge\n                  strengths for ");
      fprintf(stderr,"hysteresis. The fraction is used to compute\n");
      fprintf(stderr,"                  the high edge strength threshold.\n");
      fprintf(stderr,"      writedirim: Optional argument to output ");
      fprintf(stderr,"a floating point");
      fprintf(stderr," direction image.\n\n");
      exit(1);
   }*/

   printf("max threads is %d", omp_get_max_threads());
   rows = atoi(argv[1]);
   sigma = 1.0;//atof(argv[2]);
   tlow = 0.3;//atof(argv[3]);
   thigh =0.7;// atof(argv[4]);
   cols = rows;

   if(argc == 6) dirfilename = infilename;
   else dirfilename = NULL;

   /****************************************************************************
   * Read in the image. This read function allocates memory for the image.
   ****************************************************************************/
  /* if(VERBOSE) printf("Reading the image %s.\n", infilename);
   if(read_pgm_image(infilename, &image, &rows, &cols) == 0){
      fprintf(stderr, "Error reading the input image, %s.\n", infilename);
      exit(1);
   } */

 /*   unsigned char **image = (unsigned char**)malloc (sizeof(unsigned char*)*rows);
  for (i = 0; i < cols; i++ ) {
	*(image+i) = (unsigned char*)malloc (sizeof(unsigned char)*cols);
  }
  */
 
 unsigned char *image = (unsigned char*)malloc (sizeof(unsigned char)*rows*cols);
  

 /*  // Generate a random matrix of size height by width
  for (i = 0; i<rows*cols; i++) {
	for(j = 0; j<cols; j++) {
	  image[i][j] = (char)(rand()%255);
    }
  } */
  
  #pragma omp parallel for private(i) shared(image)
   // Generate a random image of size height by width
  for (i = 0; i<rows*cols; i++) {
	  image[i] = (char)(rand()%255);


    }
   
   
   /****************************************************************************
   * Perform the edge detection. All of the work takes place here.
   ****************************************************************************/
   if(VERBOSE) printf("Starting Canny edge detection.\n");
   if(dirfilename != NULL){
      sprintf(composedfname, "%s_s_%3.2f_l_%3.2f_h_%3.2f.fim", infilename,
      sigma, tlow, thigh);
      dirfilename = composedfname;
   }
   
   for(i =0; i<omp_get_max_threads(); i++)
   stats[i] = 0;

   st = timerval ();   
  // #pragma omp parallel
   canny(image, rows, cols, sigma, tlow, thigh, &edge, dirfilename);
   
   et = timerval ();

      for(i = 0; i < omp_get_max_threads(); i++)
	{
	   printf("Stats[%d] = %lld",i, stats[i]);
	   total_stats = total_stats + (stats[i]/1000);
	}
	
 printf("\n Total stats = %lld", total_stats);  
 printf("\nOperations = %lld, time = %f, ops = %f KOPS ", total_stats, (et-st), total_stats/(et-st));
   
/*    /****************************************************************************
   * Write out the edge image to a file.
   ****************************************************************************
   sprintf(outfilename, "%s_s_%3.2f_l_%3.2f_h_%3.2f.pgm", infilename,
      sigma, tlow, thigh);
   if(VERBOSE) printf("Writing the edge iname in the file %s.\n", outfilename);
   if(write_pgm_image(outfilename, edge, rows, cols, "", 255) == 0){
      fprintf(stderr, "Error writing the edge image, %s.\n", outfilename);
      exit(1);
   } */
}

/*******************************************************************************
* PROCEDURE: canny
* PURPOSE: To perform canny edge detection.
* NAME: Mike Heath
* DATE: 2/15/96
*******************************************************************************/
void canny(unsigned char *image, int rows, int cols, float sigma,
         float tlow, float thigh, unsigned char **edge, char *fname)
{
   FILE *fpdir=NULL;          /* File to write the gradient image to.     */
   unsigned char *nms;        /* Points that are local maximal magnitude. */
   short int *smoothedim,     /* The image after gaussian smoothing.      */
             *delta_x,        /* The first devivative image, x-direction. */
             *delta_y,        /* The first derivative image, y-direction. */
             *magnitude;      /* The magnitude of the gadient image.      */
   int r, c, pos;
   float *dir_radians=NULL;   /* Gradient direction image.                */

   /****************************************************************************
   * Perform gaussian smoothing on the image using the input standard
   * deviation.
   ****************************************************************************/
   if(VERBOSE) printf("Smoothing the image using a gaussian kernel.\n");
	//#pragma omp parallel   
	gaussian_smooth(image, rows, cols, sigma, &smoothedim);

   /****************************************************************************
   * Compute the first derivative in the x and y directions.
   ****************************************************************************/
   if(VERBOSE) printf("Computing the X and Y first derivatives.\n");
	//#pragma omp parallel   
	derrivative_x_y(smoothedim, rows, cols, &delta_x, &delta_y);

   /****************************************************************************
   * This option to write out the direction of the edge gradient was added
   * to make the information available for computing an edge quality figure
   * of merit.
   ****************************************************************************/
   if(fname != NULL){
      /*************************************************************************
      * Compute the direction up the gradient, in radians that are
      * specified counteclockwise from the positive x-axis.
      *************************************************************************/
	//#pragma omp parallel      
	radian_direction(delta_x, delta_y, rows, cols, &dir_radians, -1, -1);

      /*************************************************************************
      * Write the gradient direction image out to a file.
      *************************************************************************/
      if((fpdir = fopen(fname, "wb")) == NULL){
         fprintf(stderr, "Error opening the file %s for writing.\n", fname);
         exit(1);
      }
      fwrite(dir_radians, sizeof(float), rows*cols, fpdir);
      fclose(fpdir);
      free(dir_radians);
   }

   /****************************************************************************
   * Compute the magnitude of the gradient.
   ****************************************************************************/
   if(VERBOSE) printf("Computing the magnitude of the gradient.\n");
   //#pragma omp parallel
   magnitude_x_y(delta_x, delta_y, rows, cols, &magnitude);

   /****************************************************************************
   * Perform non-maximal suppression.
   ****************************************************************************/
   if(VERBOSE) printf("Doing the non-maximal suppression.\n");
   if((nms = (unsigned char *) calloc(rows*cols,sizeof(unsigned char)))==NULL){
      fprintf(stderr, "Error allocating the nms image.\n");
      exit(1);
   }

   //#pragma omp parallel     
   non_max_supp(magnitude, delta_x, delta_y, rows, cols, nms);

   /****************************************************************************
   * Use hysteresis to mark the edge pixels.
   ****************************************************************************/
   if(VERBOSE) printf("Doing hysteresis thresholding.\n");
   if((*edge=(unsigned char *)calloc(rows*cols,sizeof(unsigned char))) ==NULL){
      fprintf(stderr, "Error allocating the edge image.\n");
      exit(1);
   }
   //#pragma omp parallel
   apply_hysteresis(magnitude, nms, rows, cols, tlow, thigh, *edge);

   /****************************************************************************
   * Free all of the memory that we allocated except for the edge image that
   * is still being used to store out result.
   ****************************************************************************/
   free(smoothedim);
   free(delta_x);
   free(delta_y);
   free(magnitude);
   free(nms);
}

/*******************************************************************************
* Procedure: radian_direction
* Purpose: To compute a direction of the gradient image from component dx and
* dy images. Because not all derriviatives are computed in the same way, this
* code allows for dx or dy to have been calculated in different ways.
*
* FOR X:  xdirtag = -1  for  [-1 0  1]
*         xdirtag =  1  for  [ 1 0 -1]
*
* FOR Y:  ydirtag = -1  for  [-1 0  1]'
*         ydirtag =  1  for  [ 1 0 -1]'
*
* The resulting angle is in radians measured counterclockwise from the
* xdirection. The angle points "up the gradient".
*******************************************************************************/
void radian_direction(short int *delta_x, short int *delta_y, int rows,
    int cols, float **dir_radians, int xdirtag, int ydirtag)
{
   int r, c, pos;
   float *dirim=NULL;
   double dx, dy;

   /****************************************************************************
   * Allocate an image to store the direction of the gradient.
   ****************************************************************************/
   if((dirim = (float *) calloc(rows*cols, sizeof(float))) == NULL){
      fprintf(stderr, "Error allocating the gradient direction image.\n");
      exit(1);
   }
   *dir_radians = dirim;

   pos = 0;
   #pragma omp parallel for private(r)   
   for(r=0;r<rows;r++){
      for(c=0;c<cols;c++,pos++){
         dx = (double)delta_x[pos];
         dy = (double)delta_y[pos];

         if(xdirtag == 1) dx = -dx;
         if(ydirtag == -1) dy = -dy;

         dirim[pos] = (float)angle_radians(dx, dy);
		 stats[omp_get_thread_num()]++;
	
      }
   }
}

/*******************************************************************************
* FUNCTION: angle_radians
* PURPOSE: This procedure computes the angle of a vector with components x and
* y. It returns this angle in radians with the answer being in the range
* 0 <= angle <2*PI.
*******************************************************************************/
double angle_radians(double x, double y)
{
   double xu, yu, ang;

   xu = fabs(x);
   yu = fabs(y);

   if((xu == 0) && (yu == 0)) return(0);

   ang = atan(yu/xu);

   if(x >= 0){
      if(y >= 0) return(ang);
      else return(2*M_PI - ang);
   }
   else{
      if(y >= 0) return(M_PI - ang);
      else return(M_PI + ang);
   }
}

/*******************************************************************************
* PROCEDURE: magnitude_x_y
* PURPOSE: Compute the magnitude of the gradient. This is the square root of
* the sum of the squared derivative values.
* NAME: Mike Heath
* DATE: 2/15/96
*******************************************************************************/
void magnitude_x_y(short int *delta_x, short int *delta_y, int rows, int cols,
        short int **magnitude)
{
   int r, c, pos, sq1, sq2;

   /****************************************************************************
   * Allocate an image to store the magnitude of the gradient.
   ****************************************************************************/
   if((*magnitude = (short *) calloc(rows*cols, sizeof(short))) == NULL){
      fprintf(stderr, "Error allocating the magnitude image.\n");
      exit(1);
   }

   pos = 0;
   //#pragma omp parallel private(r)
   for(r=0;r<rows;r++){
      for(c=0;c<cols;c++,pos++){
         sq1 = (int)delta_x[pos] * (int)delta_x[pos];
         sq2 = (int)delta_y[pos] * (int)delta_y[pos];
         (*magnitude)[pos] = (short)(0.5 + sqrt((float)sq1 + (float)sq2));
		 stats[omp_get_thread_num()]++;
      } 
   }

}

/*******************************************************************************
* PROCEDURE: derrivative_x_y
* PURPOSE: Compute the first derivative of the image in both the x any y
* directions. The differential filters that are used are:
*
*                                          -1
*         dx =  -1 0 +1     and       dy =  0
*                                          +1
*
* NAME: Mike Heath
* DATE: 2/15/96
*******************************************************************************/
void derrivative_x_y(short int *smoothedim, int rows, int cols,
        short int **delta_x, short int **delta_y)
{
   int r, c, pos;

   /****************************************************************************
   * Allocate images to store the derivatives.
   ****************************************************************************/
   if(((*delta_x) = (short *) calloc(rows*cols, sizeof(short))) == NULL){
      fprintf(stderr, "Error allocating the delta_x image.\n");
      exit(1);
   }
   if(((*delta_y) = (short *) calloc(rows*cols, sizeof(short))) == NULL){
      fprintf(stderr, "Error allocating the delta_x image.\n");
      exit(1);
   }

   /****************************************************************************
   * Compute the x-derivative. Adjust the derivative at the borders to avoid
   * losing pixels.
   ****************************************************************************/
   if(VERBOSE) printf("   Computing the X-direction derivative.\n");
   
   #pragma omp parallel private(r)
   for(r=0;r<rows;r++){
      pos = r * cols;
      (*delta_x)[pos] = smoothedim[pos+1] - smoothedim[pos];

      pos++;
      for(c=1;c<(cols-1);c++,pos++){
         (*delta_x)[pos] = smoothedim[pos+1] - smoothedim[pos-1];
		 stats[omp_get_thread_num()]++;
      }
      (*delta_x)[pos] = smoothedim[pos] - smoothedim[pos-1];
	  stats[omp_get_thread_num()]++;
   }

   /****************************************************************************
   * Compute the y-derivative. Adjust the derivative at the borders to avoid
   * losing pixels.
   ****************************************************************************/
   if(VERBOSE) printf("   Computing the Y-direction derivative.\n");
   #pragma omp parallel private(c)
   for(c=0;c<cols;c++){
      pos = c;
	if(delta_y!=NULL) {
      (*delta_y)[pos] = smoothedim[pos+cols] - smoothedim[pos];
      pos += cols;
      }

      for(r=1;r<(rows-1);r++,pos+=cols){
	if(delta_y!=NULL) {
         (*delta_y)[pos] = smoothedim[pos+cols] - smoothedim[pos-cols];
		 stats[omp_get_thread_num()]++;
      }
	}
      (*delta_y)[pos] = smoothedim[pos] - smoothedim[pos-cols];
	  stats[omp_get_thread_num()]++;
   }
}

/*******************************************************************************
* PROCEDURE: gaussian_smooth
* PURPOSE: Blur an image with a gaussian filter.
* NAME: Mike Heath
* DATE: 2/15/96
*******************************************************************************/
void gaussian_smooth(unsigned char *image, int rows, int cols, float sigma,
        short int **smoothedim)
{
   int r, c, rr, cc,     /* Counter variables. */
      windowsize,        /* Dimension of the gaussian kernel. */
      center;            /* Half of the windowsize. */
   float *tempim,        /* Buffer for separable filter gaussian smoothing. */
         *kernel,        /* A one dimensional gaussian kernel. */
         dot,            /* Dot product summing variable. */
         sum;            /* Sum of the kernel weights variable. */

   /****************************************************************************
   * Create a 1-dimensional gaussian smoothing kernel.
   ****************************************************************************/
   if(VERBOSE) printf("   Computing the gaussian smoothing kernel.\n");
   //#pragma omp parallel
   make_gaussian_kernel(sigma, &kernel, &windowsize);
   center = windowsize / 2;

   /****************************************************************************
   * Allocate a temporary buffer image and the smoothed image.
   ****************************************************************************/
   if((tempim = (float *) calloc(rows*cols, sizeof(float))) == NULL){
      fprintf(stderr, "Error allocating the buffer image.\n");
      exit(1);
   }
   if(((*smoothedim) = (short int *) calloc(rows*cols,
         sizeof(short int))) == NULL){
      fprintf(stderr, "Error allocating the smoothed image.\n");
      exit(1);
   }

   /****************************************************************************
   * Blur in the x - direction.
   ****************************************************************************/
   if(VERBOSE) printf("   Bluring the image in the X-direction.\n");
  
   #pragma omp parallel for private(r) shared(image)
   for(r=0;r<rows;r++){
      for(c=0;c<cols;c++){
         dot = 0.0;
         sum = 0.0;
         for(cc=(-center);cc<=center;cc++){
            if(((c+cc) >= 0) && ((c+cc) < cols)){
               dot += (float)image[r*cols+(c+cc)] * kernel[center+cc];
               sum += kernel[center+cc];
		//printf("\n Hi from %d.",omp_get_thread_num());	
        stats[omp_get_thread_num()]++;
            }
         }
         tempim[r*cols+c] = dot/sum;
	   stats[omp_get_thread_num()]++;
      }
   }

   /****************************************************************************
   * Blur in the y - direction.
   ****************************************************************************/
   if(VERBOSE) printf("   Bluring the image in the Y-direction.\n");
  
   #pragma omp parallel for private(c)
   for(c=0;c<cols;c++){
      for(r=0;r<rows;r++){
         sum = 0.0;
         dot = 0.0;
         for(rr=(-center);rr<=center;rr++){
            if(((r+rr) >= 0) && ((r+rr) < rows)){
               dot += tempim[(r+rr)*cols+c] * kernel[center+rr];
               sum += kernel[center+rr];
			   stats[omp_get_thread_num()]++; 
            }
         }
         (*smoothedim)[r*cols+c] = (short int)(dot*BOOSTBLURFACTOR/sum + 0.5);
	 stats[omp_get_thread_num()]++;
      }
   }

   free(tempim);
   free(kernel);
}

/*******************************************************************************
* PROCEDURE: make_gaussian_kernel
* PURPOSE: Create a one dimensional gaussian kernel.
* NAME: Mike Heath
* DATE: 2/15/96
*******************************************************************************/
void make_gaussian_kernel(float sigma, float **kernel, int *windowsize)
{
   int i, center;
   float x, fx, sum=0.0;

   *windowsize = 1 + 2 * ceil(2.5 * sigma);
   center = (*windowsize) / 2;

   if(VERBOSE) printf("      The kernel has %d elements.\n", *windowsize);
   if((*kernel = (float *) calloc((*windowsize), sizeof(float))) == NULL){
      fprintf(stderr, "Error callocing the gaussian kernel array.\n");
      exit(1);
   }
   
   #pragma omp parallel for schedule(dynamic) private(i)
   for(i=0;i<(*windowsize);i++){
      x = (float)(i - center);
      fx = pow(2.71828, -0.5*x*x/(sigma*sigma)) / (sigma * sqrt(6.2831853));
      (*kernel)[i] = fx;
      sum += fx;
	  stats[omp_get_thread_num()]++;
   }

   #pragma omp parallel for  private(i)
   for(i=0;i<(*windowsize);i++) {
   (*kernel)[i] /= sum;
      stats[omp_get_thread_num()]++;
   }

   if(VERBOSE){
      printf("The filter coefficients are:\n");
      for(i=0;i<(*windowsize);i++)
         printf("kernel[%d] = %f\n", i, (*kernel)[i]);
   }
}


#define VERBOSE 0

#define NOEDGE 255
#define POSSIBLE_EDGE 128
#define EDGE 0

/*******************************************************************************
* PROCEDURE: follow_edges
* PURPOSE: This procedure edges is a recursive routine that traces edgs along
* all paths whose magnitude values remain above some specifyable lower
* threshhold.
* NAME: Mike Heath
* DATE: 2/15/96
*******************************************************************************/
follow_edges(unsigned char *edgemapptr, short *edgemagptr, short lowval,
   int cols)
{
   short *tempmagptr;
   unsigned char *tempmapptr;
   int i;
   float thethresh;
   int x[8] = {1,1,0,-1,-1,-1,0,1},
       y[8] = {0,1,1,1,0,-1,-1,-1};

   #pragma omp parallel for private(i)	  
   for(i=0;i<8;i++){
      tempmapptr = edgemapptr - y[i]*cols + x[i];
      tempmagptr = edgemagptr - y[i]*cols + x[i];
	  stats[omp_get_thread_num()]++;
      if((*tempmapptr == POSSIBLE_EDGE) && (*tempmagptr > lowval)){
         *tempmapptr = (unsigned char) EDGE;
         follow_edges(tempmapptr,tempmagptr, lowval, cols);
      }
   }
}

/*******************************************************************************
* PROCEDURE: apply_hysteresis
* PURPOSE: This routine finds edges that are above some high threshhold or
* are connected to a high pixel by a path of pixels greater than a low
* threshold.
* NAME: Mike Heath
* DATE: 2/15/96
*******************************************************************************/
void apply_hysteresis(short int *mag, unsigned char *nms, int rows, int cols,
	float tlow, float thigh, unsigned char *edge)
{
   int r, c, pos, numedges, lowcount, highcount, lowthreshold, highthreshold,
       i, hist[32768], rr, cc; //give comments
   short int maximum_mag, sumpix;

   /****************************************************************************
   * Initialize the edge map to possible edges everywhere the non-maximal
   * suppression suggested there could be an edge except for the border. At
   * the border we say there can not be an edge because it makes the
   * follow_edges algorithm more efficient to not worry about tracking an
   * edge off the side of the image.
   ****************************************************************************/
  
   pos = 0;
   #pragma omp parallel for private(r)
   for(r=0;r<rows;r++){
      for(c=0;c<cols;c++,pos++){
	 if(nms[pos] == POSSIBLE_EDGE) edge[pos] = POSSIBLE_EDGE;
	 else edge[pos] = NOEDGE;
	 stats[omp_get_thread_num()]++;
      }
   }

   pos = 0;
   #pragma omp parallel for private(r)
   for(r=0;r<rows;r++){
      edge[pos] = NOEDGE;
      edge[pos+cols-1] = NOEDGE;
	  stats[omp_get_thread_num()]++;
      pos+=cols;
   }
   pos = (rows-1) * cols;
   
   #pragma omp parallel for private(c)
   for(c=0;c<cols;c++){
      edge[c] = NOEDGE;
      edge[pos] = NOEDGE;
	  stats[omp_get_thread_num()]++;
      pos++;
   }

   /****************************************************************************
   * Compute the histogram of the magnitude image. Then use the histogram to
   * compute hysteresis thresholds.
   ****************************************************************************/
  #pragma omp for private(r)
  for(r=0;r<32768;r++) hist[r] = 0;

   pos=0;
   #pragma omp parallel for private(r)
   for(r=0;r<rows;r++){
      for(c=0;c<cols;c++,pos++){
	 if(edge[pos] == POSSIBLE_EDGE) hist[mag[pos]]++;
	 stats[omp_get_thread_num()]++;
      }
   }

   /****************************************************************************
   * Compute the number of pixels that passed the nonmaximal suppression.
   ****************************************************************************/
   numedges=0;
   #pragma omp parallel for private(r)
   for(r=1;r<32768;r++){
      if(hist[r] != 0) maximum_mag = r;
      numedges += hist[r];
	  stats[omp_get_thread_num()]++;
   }

   highcount = (int)(numedges * thigh + 0.5);

   /****************************************************************************
   * Compute the high threshold value as the (100 * thigh) percentage point
   * in the magnitude of the gradient histogram of all the pixels that passes
   * non-maximal suppression. Then calculate the low threshold as a fraction
   * of the computed high threshold value. John Canny said in his paper
   * "A Computational Approach to Edge Detection" that "The ratio of the
   * high to low threshold in the implementation is in the range two or three
   * to one." That means that in terms of this implementation, we should
   * choose tlow ~= 0.5 or 0.33333.
   ****************************************************************************/
   r = 1;
   numedges = hist[1];
   while((r<(maximum_mag-1)) && (numedges < highcount)){
      r++;
      numedges += hist[r];
	  stats[omp_get_thread_num()]++;
   }
   highthreshold = r;
   lowthreshold = (int)(highthreshold * tlow + 0.5);

   if(VERBOSE){
      printf("The input low and high fractions of %f and %f computed to\n",
	 tlow, thigh);
      printf("magnitude of the gradient threshold values of: %d %d\n",
	 lowthreshold, highthreshold);
   }

   /****************************************************************************
   * This loop looks for pixels above the highthreshold to locate edges and
   * then calls follow_edges to continue the edge.
   ****************************************************************************/
   pos = 0;
   #pragma omp parallel for private(r)
   for(r=0;r<rows;r++){
      for(c=0;c<cols;c++,pos++){
	 if((edge[pos] == POSSIBLE_EDGE) && (mag[pos] >= highthreshold)){
            edge[pos] = EDGE;
			stats[omp_get_thread_num()]++;
            follow_edges((edge+pos), (mag+pos), lowthreshold, cols);
	 }
      }
   }

   /****************************************************************************
   * Set all the remaining possible edges to non-edges.
   ****************************************************************************/
   pos = 0;
   #pragma omp parallel for private(r)
   for(r=0;r<rows;r++){
      for(c=0;c<cols;c++,pos++) if(edge[pos] != EDGE) {
	 edge[pos] = NOEDGE;
	  stats[omp_get_thread_num()]++;
	}
   }
}

/*******************************************************************************
* PROCEDURE: non_max_supp
* PURPOSE: This routine applies non-maximal suppression to the magnitude of
* the gradient image.
* NAME: Mike Heath
* DATE: 2/15/96
*******************************************************************************/
non_max_supp(short *mag, short *gradx, short *grady, int nrows, int ncols,
    unsigned char *result) 
{
    int rowcount, colcount,count;
    short *magrowptr,*magptr;
    short *gxrowptr,*gxptr;
    short *gyrowptr,*gyptr,z1,z2;
    short m00,gx,gy;
    float mag1,mag2,xperp,yperp;
    unsigned char *resultrowptr, *resultptr;
    

   /****************************************************************************
   * Zero the edges of the result image.
   ****************************************************************************/

   resultrowptr=result;
   resultptr=result+ncols*(nrows-1);
   #pragma omp parallel for private(count)
	for(count=0; count<ncols; count++){
        *resultrowptr = *resultptr = (unsigned char) 0;
		stats[omp_get_thread_num()]++;
	resultptr++;
	resultrowptr++;
    }

   resultptr=result;
   resultrowptr=result+ncols-1;
   #pragma omp parallel for private(count)
    for(count=0; count<nrows; count++){
        *resultptr = *resultrowptr = (unsigned char) 0;
		stats[omp_get_thread_num()]++;
	resultptr+=ncols;
	resultrowptr+=ncols;
    }

   /****************************************************************************
   * Suppress non-maximum points.
   ****************************************************************************/
 
   magrowptr=mag+ncols+1;
   gxrowptr=gradx+ncols+1;
   gyrowptr=grady+ncols+1;
   resultrowptr=result+ncols+1;

   #pragma omp parallel for private(rowcount)
	for(rowcount=1;   rowcount<nrows-2; rowcount++){   
      for(colcount=1,magptr=magrowptr,gxptr=gxrowptr,gyptr=gyrowptr,
         resultptr=resultrowptr;colcount<ncols-2; 
         colcount++,magptr++,gxptr++,gyptr++,resultptr++){   
	if(magptr != NULL) 
         m00 = *magptr;
         if(m00 == 0){
            *resultptr = (unsigned char) NOEDGE;
			
         }
         else{
            if(m00==NULL)
            printf("\n Value of float is %d",m00);
            if(gxptr != NULL && m00 != NULL && m00 != 0) 
            xperp = -(gx = *gxptr)/((float)m00);
            if(gyptr != NULL && gyptr!=NULL && m00!=0) 
            yperp = (gy = *gyptr)/((float)m00);
         }	
	stats[omp_get_thread_num()]++;
         if(gx >= 0){
            if(gy >= 0){
                    if (gx >= gy)
                    {  
                        /* 111 */
                        /* Left point */
                        z1 = *(magptr - 1);
                        z2 = *(magptr - ncols - 1);

                        mag1 = (m00 - z1)*xperp + (z2 - z1)*yperp;
                        stats[omp_get_thread_num()]++;
                        /* Right point */
                        z1 = *(magptr + 1);
                        z2 = *(magptr + ncols + 1);

                        mag2 = (m00 - z1)*xperp + (z2 - z1)*yperp;
						stats[omp_get_thread_num()]++;
                    }
                    else
                    {    
                        /* 110 */
                        /* Left point */
                        z1 = *(magptr - ncols);
                        z2 = *(magptr - ncols - 1);

                        mag1 = (z1 - z2)*xperp + (z1 - m00)*yperp;
						stats[omp_get_thread_num()]++;

                        /* Right point */
                        z1 = *(magptr + ncols);
                        z2 = *(magptr + ncols + 1);

                        mag2 = (z1 - z2)*xperp + (z1 - m00)*yperp; 
						stats[omp_get_thread_num()]++;
                    }
                }
                else
                {
                    if (gx >= -gy)
                    {
                        /* 101 */
                        /* Left point */
                        z1 = *(magptr - 1);
                        z2 = *(magptr + ncols - 1);

                        mag1 = (m00 - z1)*xperp + (z1 - z2)*yperp;
						stats[omp_get_thread_num()]++;
            
                        /* Right point */
                        z1 = *(magptr + 1);
                        z2 = *(magptr - ncols + 1);

                        mag2 = (m00 - z1)*xperp + (z1 - z2)*yperp;
						stats[omp_get_thread_num()]++;
                    }
                    else
                    {    
                        /* 100 */
                        /* Left point */
                        z1 = *(magptr + ncols);
                        z2 = *(magptr + ncols - 1);

                        mag1 = (z1 - z2)*xperp + (m00 - z1)*yperp;
						stats[omp_get_thread_num()]++;

                        /* Right point */
                        z1 = *(magptr - ncols);
                        z2 = *(magptr - ncols + 1);

                        mag2 = (z1 - z2)*xperp  + (m00 - z1)*yperp; 
						stats[omp_get_thread_num()]++;
                    }
                }
            }
            else
            {
                if ((gy = *gyptr) >= 0)
                {
                    if (-gx >= gy)
                    {          
                        /* 011 */
                        /* Left point */
                        z1 = *(magptr + 1);
                        z2 = *(magptr - ncols + 1);

                        mag1 = (z1 - m00)*xperp + (z2 - z1)*yperp;
						stats[omp_get_thread_num()]++;

                        /* Right point */
                        z1 = *(magptr - 1);
                        z2 = *(magptr + ncols - 1);

                        mag2 = (z1 - m00)*xperp + (z2 - z1)*yperp;
						stats[omp_get_thread_num()]++;
                    }
                    else
                    {
                        /* 010 */
                        /* Left point */
                        z1 = *(magptr - ncols);
                        z2 = *(magptr - ncols + 1);

                        mag1 = (z2 - z1)*xperp + (z1 - m00)*yperp;
						stats[omp_get_thread_num()]++;

                        /* Right point */
                        z1 = *(magptr + ncols);
                        z2 = *(magptr + ncols - 1);

                        mag2 = (z2 - z1)*xperp + (z1 - m00)*yperp;
						stats[omp_get_thread_num()]++;
                    }
                }
                else
                {
                    if (-gx > -gy)
                    {
                        /* 001 */
                        /* Left point */
                        z1 = *(magptr + 1);
                        z2 = *(magptr + ncols + 1);

                        mag1 = (z1 - m00)*xperp + (z1 - z2)*yperp;
						stats[omp_get_thread_num()]++;

                        /* Right point */
                        z1 = *(magptr - 1);
                        z2 = *(magptr - ncols - 1);

                        mag2 = (z1 - m00)*xperp + (z1 - z2)*yperp;
						stats[omp_get_thread_num()]++;
                    }
                    else
                    {
                        /* 000 */
                        /* Left point */
                        z1 = *(magptr + ncols);
                        z2 = *(magptr + ncols + 1);

                        mag1 = (z2 - z1)*xperp + (m00 - z1)*yperp;
						stats[omp_get_thread_num()]++;

                        /* Right point */
                        z1 = *(magptr - ncols);
                        z2 = *(magptr - ncols - 1);

                        mag2 = (z2 - z1)*xperp + (m00 - z1)*yperp;
						stats[omp_get_thread_num()]++;
                    }
                }
	    magrowptr+=ncols;
	    gyrowptr+=ncols;
	    gxrowptr+=ncols;
	    resultrowptr+=ncols;
            } 

            /* Now determine if the current point is a maximum point */

            if ((mag1 > 0.0) || (mag2 > 0.0))
            {
                *resultptr = (unsigned char) NOEDGE;
				stats[omp_get_thread_num()]++;	
            }
            else
            {    
                if (mag2 == 0.0)
                    *resultptr = (unsigned char) NOEDGE;
                else
                    *resultptr = (unsigned char) POSSIBLE_EDGE;
					stats[omp_get_thread_num()]++;	
					
            }
        } 
    }
}

