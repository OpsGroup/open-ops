#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

const int inf = 1000;
#define attemptsCount 20

void PrintMatr(int size,int **x)
{
	int i,j;
	for (i=0; i<size; i++)
	{
		printf("\n");
		for (j=0; j<size; j++)
			printf ("%4d ",x[i][j]);

	}
}
int MinNumb(int a, int b)
{
	int min1;
		if (a>b) min1=b;
		if (a<b) min1=a;
		if (a==b) min1=a;
	return min1;
}
void MinMatr(int size, int **x,int **y,int **z)
{
	int i,j;
	
	for (i=0;i<size;i++)
		for (j=0;j<size;j++)
			z[i][j]=MinNumb(x[i][j],y[i][j]);
}


void PrintVec(int size,int * vec)
{	
	int i;
	for (i=0; i<size; i++)
		printf("%d ",vec[i]);
}

void SendInfToFile (FILE *pFile,int n, int **x)
{
	int i,j;
	for (i=0; i<n; i++)
	{
		fprintf(pFile,"\n");
		for (j=0; j<n; j++)
		   fprintf (pFile, "%4d ",x[i][j]);

	}
	fclose(pFile);
}



void CreateMatrW (int n, int **a)
{
	
    int i,j;
	
	for (i=0; i<n; i++)
		for (j=0; j<n; j++)
			{
				if (i==j) a[i][j]=0;
				else a[i][j]=rand()%2;
			}
	
	for (i=0; i<n; i++)
		{
		for (j=0; j<n; j++)
			{
				if (a[i][j] == 1) a[i][j]=rand()%101;
				if ((a[i][j] == 0) && (i!=j)) a[i][j] = inf;
			}
				
		}
		
}

int Min(int n,int *vec)
{
	int i;
	int min=100000;

	for (i=0; i<n; i++)
	  if (vec[i]<min) min=vec[i];
	return min;
}



void CreateBlockLine(int **input,int **out, int size,int param,int indexi,int indexk) 
{//вытаскивает из строки матрицы input блок a[i][k]
    int i,j;

    for (i = indexi*param ; i<param*(indexi+1) ; i++ )
        for ( j = indexk*param ; j<param*(indexk+1) ; j++)
            out[i-indexi*param][j-indexk*param] = input[i][j];
}

void CreateBlockColumn(int **input,int **out, int size,int param,int indexk,int indexj) 
{//вытаскивает из строки матрицы input блок a[k][j]
    int i,j;

    for (i = indexk*param ; i<param*(indexk+1) ; i++ )
        for ( j = indexj*param ; j<param*(indexj+1) ; j++)
            out[i-indexk*param][j-indexj*param] = input[i][j];
}

void BringBlockBack(int **big,int **small, int size,int param,int indexi,int indexj)
{//возвращает блок в матрицу big
    int i,j;

    for (i = indexi*param ; i<param*(indexi+1) ; i++ )
        for ( j = indexj*param ; j<param*(indexj+1) ; j++)
            big[i][j] = small[i-indexi*param][j-indexj*param];
}


void CreateBlockD(int param, int **input1,int **input2, int **out) 
{
    int i,j,k;
	int *vec;
	vec = (int*) malloc(param * sizeof(int));

	for ( i = 0 ; i<param ; i++)
        for ( j = 0 ; j<param ; j++)
            for ( k = 0 ; k<param ; k++)
			{
				vec[k] = input1[i][k] + input2[k][j]; 
				if (k==(param-1)) out[i][j] = Min(param,vec);
			}		
			
	free(vec);
}



void BlockMulty(int **Result,int **input,int **out,int size, int param) 
{
    int i,j,k,n,m;
    int p =(int)(size/param);
	int **line,**column,**out1,**min;

	line = (int**)malloc(param*sizeof(int));
    for (i=0; i<param; i++)
            line[i]=(int*)malloc(param*sizeof(int));

    column = (int**)malloc(param*sizeof(int));
    for (i=0; i<param; i++)
            column[i]=(int*)malloc(param*sizeof(int));

	out1 = (int**)malloc(param*sizeof(int));
    for (i=0; i<param; i++)
            out1[i]=(int*)malloc(param*sizeof(int));

	min = (int**)malloc(param*sizeof(int));
			for (i=0; i<param; i++)
					 min[i]=(int*)malloc(param*sizeof(int));

	
	for ( i = 0 ; i<p ; i++)
        for ( j = 0 ; j<p ; j++)
		{
			
            for ( k = 0 ; k<p-1 ; k++)
				{
					CreateBlockLine(input,line,size,param,i,k); 
					CreateBlockColumn(input,column,size,param,k,j);
					CreateBlockD(param,line,column,out); 
					
					CreateBlockLine(input,line,size,param,i,k+1);
					CreateBlockColumn(input,column,size,param,k+1,j);
					CreateBlockD(param,line,column,out1);
					
					if (k==0)
					for ( n = 0; n < param; n++)
							for ( m = 0; m < param; m++)
								min[n][m] = out[n][m];

					
					MinMatr(param,min,out1,min);
					BringBlockBack(Result,min,size,param,i,j);
				}
		}

    for (i=0; i<param; i++)
		free(line[i]);
    free(line);

    for (i=0; i<param; i++)
        free(column[i]);
    free(column);

	for (i=0; i<param; i++)
        free(out1[i]);
    free(out1);

 }


void Return(int **Result,int **input,int **out,int size, int degree,int param) 
{
    int i;
	
	for (i = 0; i<degree; i++)
	BlockMulty(Result,input,out,size,param);
}    


int main ()
{
	clock_t t1;
	float res[attemptsCount],avg,min;
	int i,size,param,r;
	int **a,**d,**Result;
	int *vec;
	double p1;
	
	FILE *MatrD;
	MatrD = fopen ("MatrD.txt" , "w");
	
	printf("Input the order of the matrix: ");
	//scanf_s("%d", &size);
	
	printf("Input the order of the blocks: ");
	//scanf_s("%d", &param);
	
	p1 = (log((double)size)/log((double)2));
	
	vec = (int*) malloc(size * sizeof(int));

	a = (int**) malloc(size * sizeof(int));
	for (i = 0; i<size; i++)
	a[i] = (int*) malloc(size * sizeof(int));
	
	d = (int**) malloc(param * sizeof(int));
	for (i = 0; i<param; i++)
	d[i] = (int*) malloc(param * sizeof(int));

	Result = (int**)malloc(size*sizeof(int));
    for (i=0; i<size; i++)
    Result[i]=(int*)malloc(size*sizeof(int));

	srand (time(NULL) );

	CreateMatrW(size,a);
	
	 for (r = 0; r < attemptsCount; r++)
	 {	
		 t1 = clock();
		 Return(Result,a,d,size,(int)p1,param);
		 t1 = clock() - t1;
		 res[r] = (float)t1/CLOCKS_PER_SEC;
	 }

	
	fprintf(MatrD,"Distance matrix: \n");
	SendInfToFile(MatrD,size,Result);

	printf("Dimension of massive: %d\n",size);
	printf("Dimension of block: %d\n",param);
    printf("Time in milliseconds:");

    avg = 0;
    min = res[0];
    for (r = 0; r < attemptsCount; r++)
    {
        printf("\n%2d) %f", r+1, res[r]);
        if (min > res[r])
            min = res[r];
        avg+=res[r];
    }
    avg /= attemptsCount;
    printf("\nMean value : %f", avg);
    printf("\nMinimum value: %f", min);
    printf("\n============================\n\n");
    printf("\n");

	
	for (i=0; i<size; i++)
        free(a[i]);
    free(a);

	for (i=0; i<param; i++)
        free(d[i]);
    free(d);
	
	free(vec);
	
	fclose(MatrD);
	return 0;
}



