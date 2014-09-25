#include <stdio.h>
#include "omp.h"
#include <math.h>
#include <sys/time.h>
#include <time.h>

#define PI 3.14159265
//#define N 1000

double in[100][100]; 
double out[100][100];
double out1[100][100], out2[100][100];

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
	
	#pragma omp parallel //for schedule(static) num_threads(8) 
	{
	#pragma omp for
	// row operation 
	for(p=0; p < N; p++) {	
			for(n=0; n<N; n++){		
			c = (PI/N)*p*(n+0.5);
			if(p == 0)                
				row = cos(c)*1/sqrt(N);
			else
				row = cos(c)*sqrt(2)/sqrt(N);
			out1[p][n] = row;// cos term
			printf("sum calculated by thread %d= %f for q = %d.\n",omp_get_thread_num(),out1[p][n],p);
	        }
		}
	}
    #pragma omp parallel //for schedule(static) num_threads(8) 
	{
	#pragma omp for
	//column operation
	for(q=0; q < M; q++) {
			for(m=0; m<N; m++){		
			d = (PI/M)*q*(m+0.5);
			if(q == 0)                
				column = cos(d)*1/sqrt(M);
			else
				column = cos(d)*sqrt(2)/sqrt(M);
			out2[q][m] = column;//cos term
			printf("sum calculated by thread %d= %f for q = %d.\n",omp_get_thread_num(),out2[q][m],q);
        	}
			}
	}
	
	#pragma omp parallel //for schedule(static) num_threads(8) 
	{
	#pragma omp for
		for(p=0; p < N; p++)
	{
		printf("\n");
        for(q=0; q < M; q++)		
		{	
            for(n=0; n<N; n++) 
            for(m=0;m<M; m++)
            out[p][q]+= in[n][m]*out1[p][n]*out2[q][m];
            //printf("[%d][%d]= %f \t",p,q, out[p][q]);
	}}
	}
 
	et = timerval ();
	printf ("\n\nIt took TIME of %f seconds \n", et - st);
 
 for(p=0; p < N; p++)
{
 printf("\n");
        for(q=0; q < M; q++)
                {
            
            printf("[%d][%d]= %f \t",p,q, out[p][q]);
        }}
}
