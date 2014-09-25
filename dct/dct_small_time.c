#include <stdio.h>
#include "omp.h"
#include <math.h>
#include <sys/time.h>
#include <time.h>

#define PI 3.14159265
//#define N 1000

double in[10000][10000]; 
double out[10000][10000];
double out1[10000][10000], out2[10000][10000];

//function to get the time of day. same as code given in HW4 doc
double timerval() {
	struct timeval st;
	gettimeofday(&st, NULL);
	return st.tv_sec + st.tv_usec * pow(10,-6);
}

int main(int argc, char*argv[]) {
	int p, q, m, n, i,j,k;
	double c, d,row, column;
	long long int N = atoi(argv[1]);
	long long int M = atoi(argv[2]);

	double st, et;
	st = timerval ();
	int stats[240];
	for(i = 0; i < omp_get_max_threads(); i++)
	stats[i] = 0;
	
	int total_stats = 0;
	
	k = 0;
	for (i=0; i<M; i++)
	{ 
		printf("\n");
		for (j=0; j<N; j++)
		{
			//for (k=0; k<4; k++)
			{
				k++;			
				in[i][j] = k;
				printf("\t in[%d][%d] = %d", i,j,k);
			}
		}
	}
	
    int ctr = 0;
	for(ctr = 0; ctr < 1000; ctr++) {
	#pragma omp parallel //for schedule(static) num_threads(8)
	{	
		#pragma omp for

		for(p=0; p < N; p++)
        for(q=0; q < M; q++)		
		{	
            for(m=0;m<M; m++)
			{	
				for(n=0; n<N; n++)
				{
					c = (PI/N)*p*(n+0.5);
					stats[omp_get_thread_num()]++;
					if(p == 0)                
						row = cos(c)*1/sqrt(N);
					else
						row = cos(c)*sqrt(2)/sqrt(N);
					out1[p][n] = row;// cos term
					//printf("sum calculated by thread %d= %f for q = %d.\n",omp_get_thread_num(),out1[p][n],p);
						
					d = (PI/M)*q*(m+0.5);
					stats[omp_get_thread_num()]++;
					if(q == 0)                
						column = cos(d)*1/sqrt(M);
					else
						column = cos(d)*sqrt(2)/sqrt(M);
					out2[q][m] = column;//cos term
					//printf("sum calculated by thread %d= %f for q = %d.\n",omp_get_thread_num(),out2[q][m],q);
					out[p][q]+= in[n][m]*out1[p][n]*out2[q][m];
					//printf("out%d %d= %f by thread = %d.\n",p,q, out[p][q],omp_get_thread_num());
				}
			}	
		}
	}	
	
}
	et = timerval ();
	printf ("\n\nIt took TIME of %f seconds \n", et - st);
	for(i = 0; i < omp_get_max_threads(); i++)
		{
		   printf("Stats[%d] = %d",i, stats[i]);
		   total_stats = total_stats + stats[i];
		}
	
	printf("\nOperations = %d, time = %f, ops = %lf ", total_stats, (et-st)/1000, total_stats/(1000*(et-st)));

	for(p=0; p < N; p++)
	{
		printf("\n");
        for(q=0; q < M; q++)
        {
            printf("[%d][%d]= %f \t",p,q, out[p][q]);
        }
	}
	
}