#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h> 
#include<math.h>
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
	unsigned int M, N;
	 N = atol(argv[1]);
	 M = atol(argv[2]);
	//printf("Matrix input = %d,%d\n", M,N);

	// read numerator
	unsigned int i,k;
	int nr,nl;

	nr = 16;
	nl = 2;
	//printf("nsec row=%d col=%d\n", nr, nl);

	float *nsec;
	nsec = (float*) malloc(sizeof(float)*nr*nl);
	for(i=0;i<32;i++){
		nsec[i] = 0.5;
	}

	// read c 
	float c = 6.0;

	// read denominator
	int dnr,dnl;

	dnr =16;
	dnl=2;
	//printf("dsec row=%d col=%d\n", dnr, dnl);

	float *dsec;
	dsec = (float*) malloc(sizeof(float)*dnr*dnl);
	for(i=0;i<32;i++){
		dsec[i] = 2.0;
	}

	// allocate input/output data 
	//float *X;
	//float *Y;
	//X  = (float*) malloc(sizeof(float)*M*N);
	//Y  = (float*) calloc(M*N,sizeof(float));

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

    int oprn = 0;
	
	for(i=0;i<M;i++)
        for(k=0;k<N;k++)
	{
		y = c * X[i][k];
		for(j=0;j<dnr;j++)
		{
			unew = X[i][k]- u[j*2]*dsec[j*2] - u[j*2+1]*dsec[j*2+1];	
			u[2*j+1]=u[2*j];	
			u[2*j]=unew;	
			y = y + u[j*2]*nsec[j*2] + u[j*2+1]*nsec[j*2+1];
			oprn = oprn + 4;
		}
		Y[i][k] = y;	
		
	}

	      et = timerval ();
	printf ("\n\nIt took TIME of %f seconds and %d operations and ops = %lf\n ", et - st, oprn, oprn/(et-st));

	for(i=0;i<M;i++)
	{
	printf("\n");
        for(k=0;k<N;k++)
		{
		printf("%lf ", Y[i][k]);
		}
    }

	// release
	//free(X);
	//free(Y);
	free(nsec);
	free(dsec);
	free(u);

	return 0;
}
