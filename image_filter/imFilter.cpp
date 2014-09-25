////////////////////////////////////////////////////////////////////////
//
// hello-world.cpp
//
// This is a simple, introductory OpenCV program. The program reads an
// image from a file, inverts it, and displays the result. 
//
////////////////////////////////////////////////////////////////////////


/* Errors/Not working - Divya
1. omp_get_num_threads()
2. core dump for #cores > 4
3. ask about nested for 
4. corner/borders
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <cv.h>
#include <highgui.h>
#include "omp.h"

double timerval() {
	struct timeval st;
	gettimeofday(&st, NULL);
	
 	return st.tv_sec + st.tv_usec*pow(10, -6);
}

int comp (const void * elem1, const void * elem2) 
{
    int f = *((int*)elem1);
    int s = *((int*)elem2);
    if (f >= s) return  1;
    if (f < s) return -1;
    return 0;
}

int findMedian(int x[]) 
{
    qsort (x, sizeof(x)/sizeof(*x), sizeof(*x), comp);
   /* int ind;
    for (ind = 0 ; ind < sizeof(x)/sizeof(*x) ; ind++)
        printf ("%d ", x[ind]);
*/
  // printf("\nmedian value is %d ", x[((sizeof(x)/sizeof(*x))-1)/2] );

    return    x[((sizeof(x)/sizeof(*x))-1)/2];
}

int main(int argc, char *argv[])
{
  IplImage* processedImg = 0; 
  IplImage* img = 0;
  int height,width,step,channels;
  uchar *data, *processedData, *oprn;
  int i,j,k, my_oprn, sum, threadId, stats[240], total_stats=0;

  if(argc<3){
    printf("Usage: main <image-file-name> <operation>\n\7");
    exit(0);
  }

  // load an image  
  img=cvLoadImage(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
  if(!img){
    printf("Could not load image file: %s\n",argv[1]);
    exit(0);
  }

  //Convert rgb to gray
  //cv::cvtColor(rgbimg, img, CV_RGB2GRAY);
  
  if (!(strcmp(argv[2],"mean")))
  my_oprn = 1; //mean filtering
  else if (!(strcmp(argv[2],"median")))
  my_oprn = 2; //median filtering
  else
  my_oprn = 3; //Gaussian filtering

  // get the image data
  height    = img->height;
  width     = img->width;
  step      = img->widthStep;
  channels  = img->nChannels;
  data      = (uchar *)img->imageData;
  processedImg = cvCloneImage(img);
  processedData      = (uchar *)processedImg->imageData;
  printf("Processing a %dx%d image with %d channels\n",height,width,channels); 

/*  // create a window
  cvNamedWindow("mainWin", CV_WINDOW_AUTOSIZE); 
  cvMoveWindow("mainWin", 100, 100);

  // show the original image
  cvShowImage("mainWin", img );

  // wait for a key
  cvWaitKey(0);
*/
//doesn't seem to be working

printf("\nmax threads is %d", omp_get_max_threads());


    double st, et;
	st = timerval ();
    for(i =0; i<omp_get_max_threads(); i++)
    stats[i] = 0;


switch(my_oprn) {

case 1:

  // operate on the image
  #pragma omp parallel for shared(i,data, stats) private (j,sum)//schedule(static) num_threads(4) 
  //for non-border pixels
  for(i=1;i<height-1;i++) 
  {
     for(j=1;j<width-1;j++) 
     {
  	//operation    
        sum = 0;
        sum = data[(i-1)*step+j-1]+data[(i-1)*step+j]+data[(i-1)*step+j+1]
             +data[i*step+j-1]+data[i*step+j]+data[i*step+j+1]
             +data[(i+1)*step+j-1]+data[(i+1)*step+j]+data[(i+1)*step+j+1];                        
	processedData[i*step+j]= sum/9;
stats[omp_get_thread_num()]++;	
// threadId = omp_get_thread_num(); //working
//      printf("\nhello from %d", threadId); //working
     }
  }

  if(omp_get_thread_num() == 0) {	
  //for border pixels
  //First column
  
     for(j=1;j<width-1;j++)   	
     {	
        i=0;	
	//operation    
        sum = 0;
        sum = data[(i-1)*step+j]+data[(i-1)*step+j+1]
             +data[i*step+j]+data[i*step+j+1]
             +data[(i+1)*step+j]+data[(i+1)*step+j+1];                        
	processedData[i*step+j]= sum/9;
        stats[omp_get_thread_num()]++;	
     }	  
  
  
  //Last column
   
  
     for(j=1;j<width-1;j++)   	
     {	
	i=width;	
	//operation    
        sum = 0;
        sum = data[(i-1)*step+j-1]+data[(i-1)*step+j]
             +data[i*step+j-1]+data[i*step+j]
             +data[(i+1)*step+j-1]+data[(i+1)*step+j];                        
	processedData[i*step+j]= sum/9;
        stats[omp_get_thread_num()]++;	
     }	  
  
  
  
  //First row
  
     for(i=1;i<height-1;i++)   	
     {	
        j=0; 	
        //operation    
        sum = 0;
        sum = data[i*step+j-1]+data[i*step+j]+data[i*step+j+1]
             +data[(i+1)*step+j-1]+data[(i+1)*step+j]+data[(i+1)*step+j+1];                        
	processedData[i*step+j]= sum/9;
        stats[omp_get_thread_num()]++;	
     }	  
  

  //Last row
  
 
     for(i=1;i<height-1;i++)   	
     {	
        j=width; 	
        //operation    
        sum = 0;
        sum = data[(i-1)*step+j-1]+data[(i-1)*step+j]+data[(i-1)*step+j+1]
             +data[i*step+j-1]+data[i*step+j]+data[i*step+j+1];                       
	processedData[i*step+j]= sum/9;
	stats[omp_get_thread_num()]++;	
     }	  
 

  //Corner pixels
  	sum = 0;
        sum = data[0]+data[1]+data[step]+data[step+1];                       
	processedData[0]= sum/9;
        stats[omp_get_thread_num()]++;	

	sum = 0;
        sum = data[step-2]+data[step-1]+data[2*step-1]+data[2*step-2];                
	processedData[step]= sum/9;
	stats[omp_get_thread_num()]++;	

	sum = 0;
        sum = data[height*step]+data[height*step+1]+data[height*step-step]+data[height*step-step+1];                       
	processedData[height*step]= sum/9;
	stats[omp_get_thread_num()]++;	

	sum = 0;
        sum = data[height*step+step]+data[height*step+step-1]+data[height*step-step]+data[height*step-step+1];                       
	processedData[height*step+step]= sum/9;
	stats[omp_get_thread_num()]++;	

  //get num..not working
  //  for(i =0; i<omp_get_num_threads(); i++)
  //  printf("\nstats[%d] %d", i, stats[i]);

}

break;
//Median
case 2:

 #pragma omp parallel for shared(i,data, stats) private (j,sum)//schedule(static) num_threads(4) 
  //for non-border pixels
  for(i=1;i<height-1;i++) 
  {
     for(j=1;j<width-1;j++) 
     {
  	
        int passVal[] = {data[(i-1)*step+j-1],data[(i-1)*step+j],data[(i-1)*step+j+1],
                         data[i*step+j-1],data[i*step+j],data[i*step+j+1],
			 data[(i+1)*step+j-1],data[(i+1)*step+j],data[(i+1)*step+j+1]};
        
	processedData[i*step+j]= findMedian(passVal);  
        stats[omp_get_thread_num()]++;	
// threadId = omp_get_thread_num(); //working
//      printf("\nhello from %d", threadId); //working
     }
  }

  if(omp_get_thread_num() == 0) {	
  //for border pixels
  //First column

     for(j=1;j<width-1;j++)   	
     {	
        i=0;	
        int passVal1[] = {data[(i-1)*step+j],data[(i-1)*step+j+1],data[i*step+j],data[i*step+j+1],data[(i+1)*step+j],data[(i+1)*step+j+1]};
	//operation    
        processedData[i*step+j]= findMedian(passVal1);                        
	
        stats[omp_get_thread_num()]++;	
     }	  
  
  
  //Last column
   
  
     for(j=1;j<width-1;j++)   	
     {	
	i=width;
        int passVal2[] = {data[(i-1)*step+j-1],data[(i-1)*step+j],data[i*step+j-1],data[i*step+j],data[(i+1)*step+j-1],data[(i+1)*step+j]};	
	//operation    
        processedData[i*step+j]= findMedian(passVal2);
        stats[omp_get_thread_num()]++;	
     }	  
  
  
  
  //First row
  
     for(i=1;i<height-1;i++)   	
     {	
        j=0; 	
        //operation    
        int passVal3[] = {data[i*step+j-1],data[i*step+j],data[i*step+j+1],data[(i+1)*step+j-1],data[(i+1)*step+j],data[(i+1)*step+j+1]};
	processedData[i*step+j]= findMedian(passVal3);
        stats[omp_get_thread_num()]++;	
     }	  
  

  //Last row
  
 
     for(i=1;i<height-1;i++)   	
     {	
        j=width; 	
        //operation    
        int passVal4[] = {data[(i-1)*step+j-1],data[(i-1)*step+j],data[(i-1)*step+j+1],data[i*step+j-1],data[i*step+j],data[i*step+j+1]};
        processedData[i*step+j]= findMedian(passVal4);
	stats[omp_get_thread_num()]++;	
     }	  
 

  //Corner pixels

        int passVal5[] = {data[0],data[1],data[step],data[step+1]};
	processedData[0]= findMedian(passVal5);                       
        stats[omp_get_thread_num()]++;	
        
        int passVal6[] = {data[step-2],data[step-1],data[2*step-1],data[2*step-2]}; 
	processedData[step]= findMedian(passVal6);
	stats[omp_get_thread_num()]++;	
                   
        int passVal7[] = {data[height*step],data[height*step+1],data[height*step-step],data[height*step-step+1]};
	processedData[height*step]=findMedian(passVal7);    
	stats[omp_get_thread_num()]++;	

        int passVal8[] = {data[height*step+step],data[height*step+step-1],data 						[height*step-step],data[height*step-step+1]};
	processedData[height*step+step]= findMedian(passVal8);
	stats[omp_get_thread_num()]++;	

}

break;

//Gaussian
case 3:

break;

} //end of switch
  et = timerval ();
	
    for(i = 0; i < omp_get_max_threads(); i++)
	{
	   printf("Stats[%d] = %d",i, stats[i]);
	   total_stats = total_stats + stats[i];
	}
	
	printf("\nOperations = %d, time = %f, ops = %lf ", total_stats, et-st, total_stats/(et-st));
/*  // show the image
  cvShowImage("mainWin", processedImg );

  // wait for a key
  cvWaitKey(0);
*/
  // release the image
  cvReleaseImage(&img );
cvReleaseImage(&processedImg);
  return 0;
}
