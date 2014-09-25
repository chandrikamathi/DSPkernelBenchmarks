#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h> 
#include<math.h>
#include"omp.h"
#include<time.h>
#include<sys/time.h>
double timerval() {
	struct timeval st;
	gettimeofday(&st, NULL);
	return st.tv_sec + st.tv_usec * pow(10,-6);
}

double X[1000][1000]; 
double Y[1000][1000];

int main(int argc, char *argv[])
{
	long long int N = atoi(argv[1]);
	long long int M = atoi(argv[2]);
	
	unsigned int i,k;
	int nr,nl;
	nr = 16;
	nl = 2;
	
	float *nsec;
	nsec = (float*) malloc(sizeof(float)*nr*nl);
	for(i=0;i<32;i++){
		nsec[i] = 0.5;
	}

	float c = 6.0;

	int dnr,dnl;

	dnr =16;
	dnl=2;

	float *dsec;
	dsec = (float*) malloc(sizeof(float)*dnr*dnl);
	for(i=0;i<32;i++){
		dsec[i] = 2.0;
	}

	
	for(i=0;i<M;i++)
        for(k=0;k<N;k++)
	{
		X[i][k] = 10.0;
	}

	float *u; // intermediate data
	u = (float*) calloc(dnr*2,sizeof(float));

	float unew;

	int j;
	float y = 0;	
	double st, et;
	st = timerval ();
	
	int stats[240];
	for(i = 0; i < omp_get_max_threads(); i++)
	stats[i] = 0;
	
	int total_stats = 0;
	
	for(i=0;i<M;i++)
	{
	    for(k=0;k<N;k++)
		{
			y = c * X[i][k];
			#pragma omp parallel for
			for(j=0;j<dnr;j++)
			{
				unew = X[i][k]- u[j*2]*dsec[j*2] - u[j*2+1]*dsec[j*2+1];	
				u[2*j+1]=u[2*j];	
				u[2*j]=unew;	
				y = y + u[j*2]*nsec[j*2] + u[j*2+1]*nsec[j*2+1];
				stats[omp_get_thread_num()] = stats[omp_get_thread_num()] + 4; 
			}
			Y[i][k] = y;	
			
		}
		
	}
	
        et = timerval ();
	printf ("\n\nIt took TIME of %f seconds \n", et - st);

	for(i=0;i<M;i++)
	{
	printf("\n");
        for(k=0;k<N;k++)
		{
		printf("%lf ", Y[i][k]);
		}
    }
    
	for(i = 0; i < omp_get_max_threads(); i++)
	{
	   printf("Stats[%d] = %d",i, stats[i]);
	   total_stats = total_stats + stats[i];
	}
	
	printf("\nOperations = %d, time = %f, ops = %lf ", total_stats, et-st, total_stats/(et-st));
	// release
	//free(X);
	//free(Y);
	free(nsec);
	free(dsec);
	free(u);

	return 0;
}
