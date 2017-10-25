#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#define T 1000
#define NULL 0
long Kol_block_v_stroke,Kol_block_v_stolbce;
char *String1,*String2;
long M,N;

typedef struct 
{
	int BLOCKI;
	int BLOCKJ;
	int maxi;
	int maxj;
	struct MaxElement* next;
}MaxElement,*PMaxElement;

typedef struct
{
	int max;
	int maxi;
	int maxj;
	struct MAXIMAL* next;
}MAXIMAL,*PMAX;

typedef struct 
{
		//int col[T];
		//int row[T];
		int* col;
		int* row;
		PMAX el_max;
}BLOCK,*PBLOCK;
	 

typedef struct 
{
	char inf;
	struct String* next;
}String,*PString;

PString Cons( char x, PString L)
{
	PString T1;
	T1 = ( PString )malloc( sizeof(String));
	T1->inf = x;
	T1->next = L;
return T1;
}

PMAX Cons_el( int x,int i, int j, PMAX L)
{
	PMAX T1;
	T1 = ( PMAX )malloc( sizeof(MAXIMAL));
	T1->max = x;
	T1->maxi = i;
	T1->maxj = j;
	T1->next = L;
return T1;
}

int max_from_two( int x, int y)
{
	return ((x>=y)?x:y);
}

int min_from_two( int x, int y)
{
	return ((x<=y)?x:y);
}

long size_of_file( FILE* fileS1)
{
	long size_file;
	fseek(fileS1, 0, SEEK_END); // переместить указатель в конец файла
	size_file = ftell(fileS1); // получить текущую позицию
	fseek(fileS1, 0, SEEK_SET); // вернуть указатель на начало
	return size_file;
}

int read_file( FILE* file_string, char* String)
{
	char b;
	int i;
	i = 0;
	while(!feof(file_string)){
		b = fgetc(file_string);
		if (isalpha(b))
		{
			String[i] = toupper(b);
			i++;
		}
	}
return i;
}

FILE* write_to_file( FILE* file1, PString Str1,int x, int y)
{
	PString Str;
	fprintf(file1," %c",'*'); 
	while( Str1 ){
      // printf(" %c",Str1->inf);
		fprintf(file1,"%c",Str1->inf);
        Str = Str1;
		Str1 = Str1->next;
        free(Str);
    }
	fprintf(file1," { %i  %i }",x,y);
	fprintf(file1,"%c",'\n');
	//printf("\n");
	return file1;
}

 PMaxElement maxel ( BLOCK **mas, PMaxElement B,int* p )
{ 
	int i,j,maximal,maximalI,maximalJ;
	PMaxElement m;
	maximal = 0;
	maximalI = 0;
	maximalJ = 0;

	for ( i = 0; i < Kol_block_v_stolbce; i++ )
		for ( j = 0; j < Kol_block_v_stroke; j++ )
			if ( mas[i][j].el_max->max >= maximal ){
				maximal = mas[i][j].el_max->max;
				maximalI = mas[i][j].el_max->maxi;
				maximalJ = mas[i][j].el_max->maxj;
			}
			printf(" TT %i ",maximal);
	for ( i = 0; i < Kol_block_v_stolbce; i++)
		for ( j = 0; j < Kol_block_v_stroke; j++ )
			if ( mas[i][j].el_max->max == maximal ){
				while ( mas[i][j].el_max )
				{
					m = ( PMaxElement )malloc( sizeof ( MaxElement ));
					m -> BLOCKI = i;
					m -> BLOCKJ = j;
					m -> maxi = mas[i][j].el_max->maxi-1;
					m -> maxj =  mas[i][j].el_max->maxj-1;
					m -> next = B;
					B = m;
					mas[i][j].el_max = mas[i][j].el_max->next;
			}
			}
			*p = maximal;
	return m;
}


 int differences(char S1, char S2, int penalty, int** BLOSUM)
 {
	 int difference;
	 if(( S2 == 'N')||(S1 == 'N'))
                        difference = penalty/2;
					if( S2 == 'A') {
						if ( S1 == 'G')
							difference = BLOSUM [1][0];
						if ( S1 == 'C')
							difference = BLOSUM [2][0];
						if ( S1 == 'T')
							difference = BLOSUM [3][0];
						if ( S1 == '-')
							difference = BLOSUM [4][0];
					}
					if( S2 == 'G'){
						if ( S1 == 'A')
							difference = BLOSUM [1][0];
						if ( S1 == 'C')
							difference = BLOSUM [2][1];
						if ( S1 == 'T')
							difference = BLOSUM [3][1];
						if ( S1 == '-')
							difference = BLOSUM [4][1];
					}
					if ( S2 == 'C') {
						if ( S1 == 'A')
							difference = BLOSUM [2][0];
						if ( S1 == 'G')
							difference = BLOSUM [2][1];
						if ( S1 == 'T')
							difference = BLOSUM [3][2];
						if ( S1 == '-')
							difference = BLOSUM [4][2];
					}
					if ( S2 == 'T'){
						if ( S1 == 'A')
							difference = BLOSUM [3][0];
						if ( S1 == 'G')
							difference = BLOSUM [3][1];
						if ( S1 == 'C')
							difference = BLOSUM [3][2];
						if ( S1 == '-')
							difference = BLOSUM [4][3];
					}
					if( S2 == '-') {
						if ( S1 == 'G')
							difference = BLOSUM [4][1];
						if ( S1 == 'C')
							difference = BLOSUM [4][2];
						if ( S1 == 'T')
							difference = BLOSUM [4][3];
						if ( S1 == 'A')
							difference = BLOSUM [4][0];
					}
return difference;
 }

 int similarities(char S2,int** BLOSUM)
 {
	 int similarity;
	 if ( S2 == 'A') similarity = BLOSUM [0][0];
  	 if ( S2 == 'G') similarity = BLOSUM [1][1];
	 if ( S2 == 'C') similarity = BLOSUM [2][2];
	 if ( S2 == 'T') similarity = BLOSUM [3][3];
	 if ( S2 == '-') similarity = BLOSUM [4][4];
return similarity;
 }

void blocks( int *rows, int *cols,int diag,int M_bl, int N_bl, PBLOCK A1,int penalty, int** BLOSUM)
{	
	int **h;
	int wp1,wp2;
	int i,j,Size_colomn,Size_row;
	int w,max,maximum;
	
	if ( N % T != 0 &&  N_bl == Kol_block_v_stroke -1 )	
			Size_row = N % T;
	else 
		Size_row = T;
	if ( M % T != 0 &&  M_bl == Kol_block_v_stolbce - 1 )	
			Size_colomn = M % T;
	else
		Size_colomn = T;	

	h = (int**)malloc(( Size_colomn + 1 )*sizeof(int*));
	for ( i = 0; i < Size_colomn+1;i ++)
		h[i] = (int*)malloc(( Size_row + 1 )*sizeof(int));

	h[0][0] = diag;
	for ( i = 1; i <= Size_colomn; i++)
		h[i][0] = cols[i-1];
	
	for ( j = 1; j <= Size_row; j++)
			h[0][j] = rows[j-1];
	
	maximum = 0;
	A1->row = (int*)calloc(T,sizeof(int));
	A1->col = (int*)calloc(T,sizeof(int));
	for ( i = 1; i <= Size_colomn; i++)
		for ( j = 1; j <= Size_row; j++)
		{
		max = 0;

		if( String1[i-1 + M_bl*T]==String2[j-1+ N_bl*T])
		{
			switch (String2[j-1+ N_bl*T])
			{
			case 'A':
				w = BLOSUM [0][0];
				break;
			case 'G':
				w = BLOSUM [1][1];
				break;
			case 'C':
				w = BLOSUM [2][2];
				break;
			case 'T':
				w = BLOSUM [3][3];
				break;
			case '-':
				w = BLOSUM [4][4];
				break;
			}
		}
		else
		{
			if(( String2[j-1+ N_bl*T] == 'N')||(String1[i-1 + M_bl*T] == 'N'))
                        w = penalty/2;
					if( String2[j-1+ N_bl*T] == 'A') {
						switch (String1[i-1 + M_bl*T])
						{
						case 'G':
							w = BLOSUM [1][0];
							break;
						case 'C':
							w = BLOSUM [2][0];
							break;
						case 'T':
							w = BLOSUM [3][0];
							break;
						case '-':
							w = BLOSUM [4][0];
							break;
						}
					}
					if( String2[j-1+ N_bl*T] == 'G'){
						switch (String1[i-1 + M_bl*T])
						{
						case 'A':
							w = BLOSUM [1][0];
							break;
						case 'C':
							w = BLOSUM [2][1];
							break;
						case 'T':
							w = BLOSUM [3][1];
							break;
						case '-':
							w = BLOSUM [4][1];
							break;
						}
					}
					if ( String2[j-1+ N_bl*T] == 'C') {
						switch (String1[i-1 + M_bl*T])
						{
						case 'A':
							w = BLOSUM [2][0];
							break;
						case 'G':
							w = BLOSUM [2][1];
							break;
						case 'T':
							w = BLOSUM [3][2];
							break;
						case '-':
							w = BLOSUM [4][2];
							break;
						}
					}
					if ( String2[j-1+ N_bl*T] == 'T'){
						switch (String1[i-1 + M_bl*T])
						{
						case 'A':
							w = BLOSUM [3][0];
							break;
						case 'G':
							w = BLOSUM [3][1];
							break;
						case 'C':
							w = BLOSUM [3][2];
							break;
						case '-':
							w = BLOSUM [4][3];
							break;
						}
					}
					if( String2[j-1+ N_bl*T] == '-') {
						switch (String1[i-1 + M_bl*T])
						{
						case 'G':
							w = BLOSUM [4][1];
							break;
						case 'C':
							w = BLOSUM [4][2];
							break;
						case 'T':
							w = BLOSUM [4][3];
							break;
						case 'A':
							w = BLOSUM [4][0];
							break;
						}
					}
		}
		if (String1[i-1+ M_bl*T] != '-')
			switch (String1[i-1+ M_bl*T])
			{
			case 'G':
				wp1 = BLOSUM [4][1];
				break;
			case 'C':
				wp1 = BLOSUM [4][2];
				break;
			case 'T':
				wp1 = BLOSUM [4][3];
				break;
			case 'A':
				wp1 = BLOSUM [4][0];		
				break;
			}
		else 
			 wp1 = BLOSUM [4][4];
		
		if (String2[j-1+ N_bl*T] != '-')
			switch (String2[j-1+ N_bl*T])
			{
			case 'G':
				wp2 = BLOSUM [4][1];
				break;
			case 'C':
				wp2 = BLOSUM [4][2];
				break;
			case 'T':
				wp2 = BLOSUM [4][3];
				break;
			case 'A':
				wp2 = BLOSUM [4][0];	
				break;
			}
		else wp2 = BLOSUM [4][4];
	
		/*
		if( String1[i-1 + M_bl*T]==String2[j-1+ N_bl*T])
			w = match;
		else
			w = mismatch;

		if (String1[i-1+ M_bl*T] != '_')
			wp1 = mismatch;
		else 
			wp1 = match;
		
		if (String2[j-1+ N_bl*T] != '_')
			wp2 = mismatch;
		else 
			wp2 = match;
			*/

		if (( h[i-1][ j-1] + w )> max ) 
			max = h[i-1][ j-1] + w ;
		
		if ( ( h[i][ j-1] + wp2 )> max )
			max = h[i][ j-1] + wp2 ;
			
		if ( ( h[i-1][ j] + wp1 )> max ) 
			max = h[i-1][ j] + wp1 ;

		h[i][j] = max;
		
		if ( h[i][j] >= maximum )
			maximum = h[i][j];
	
		if ( i == T )
						A1->row[j-1] = h[i][j];
					if ( j == T )
						A1->col[i-1] = h[i][j];
		}
		A1->el_max = NULL;
		for ( i = 1; i <= Size_colomn; i++)
			for ( j = 1; j <= Size_row; j++)
					if ( h[i][j] == maximum )
						A1->el_max = Cons_el(h[i][j],i,j,A1->el_max);
															
for ( j=0; j<Size_colomn;j++)
		free (h[j]);
free (h); 
}
void clear (int M_bl, int N_bl,int i2, int j2, BLOCK** A1)
{
	int i,j;
	for ( i = M_bl+1; i <= i2; i++)
		for ( j = 0 ; j <= j2; j++)
	{
		free(A1[i][j].col);
		free(A1[i][j].row);
	}
	
	for ( j = N_bl+1; j <= j2; j++)
		for ( i = 0; i <= M_bl; i++)
	{
		free(A1[i][j].col);
		free(A1[i][j].row);
	}
}
unsigned char** recollect( int *rows, int *cols,int diag,int M_bl, int N_bl,unsigned char **P,int penalty, int** BLOSUM)
{
	int **h;
	unsigned char previous;
	int wp1,wp2;
	int i,j,Size_colomn,Size_row;
	int w,max;
	
	if ( N % T != 0 &&  N_bl == Kol_block_v_stroke -1 )	
			Size_row = N % T;
	else 
		Size_row = T;
	if ( M % T != 0 &&  M_bl == Kol_block_v_stolbce - 1 )	
			Size_colomn = M % T;
	else
		Size_colomn = T;	

	h = (int**)malloc(( Size_colomn + 1 )*sizeof(int*));
	for ( i = 0; i < Size_colomn+1;i ++)
		h[i] = (int*)malloc(( Size_row + 1 )*sizeof(int));

	h[0][0] = diag;
	for ( i = 1; i <= Size_colomn; i++)
		h[i][0] = cols[i-1];
	
	for ( j = 1; j <= Size_row; j++)
			h[0][j] = rows[j-1];
		
	for ( i = 1; i <= Size_colomn; i++)
		for ( j = 1; j <= Size_row; j++)
		{
		max = 0;
		previous = 0;
		/*
		if( String1[i-1 + M_bl*T]==String2[j-1+ N_bl*T])
			w = match;
		else
			w = mismatch;

		if (String1[i-1+ M_bl*T] != '_')
			wp1 = mismatch;
		else 
			wp1 = match;
		
		if (String2[j-1+ N_bl*T] != '_')
			wp2 = mismatch;
		else 
			wp2 = match;
			*/

		if( String1[i-1 + M_bl*T]==String2[j-1+ N_bl*T])
			w = similarities(String2[j-1+ N_bl*T],BLOSUM);
		else
			w = differences(String1[i-1 + M_bl*T],String2[j-1+ N_bl*T],penalty,BLOSUM);

		if (String1[i-1+ M_bl*T] != '-')
			wp1 = differences(String1[i-1 + M_bl*T],'-',penalty,BLOSUM);
		else wp1 = similarities('-',BLOSUM);
		
		if (String2[j-1+ N_bl*T] != '-')
			wp2 = differences('-',String2[j-1+ N_bl*T],penalty,BLOSUM);
		else wp2 = similarities('-',BLOSUM);

		if (( h[i-1][ j-1] + w )> max ) {
			max = h[i-1][ j-1] + w ;
			previous = 2;
		}
		if ( ( h[i][ j-1] + wp2 )> max ){
			max = h[i][ j-1] + wp2 ;
			previous = 1;
		}
		
		if ( ( h[i-1][ j] + wp1 )> max ) {
			max = h[i-1][ j] + wp1 ;
			previous = 3;
		}

		h[i][j] = max;
		P[i-1][j-1] = previous;
		}

for ( j=0; j<Size_colomn;j++)
		free (h[j]);
free (h);

return P;
}



int main()
{
	FILE * fileS1,*fileS2;
	long size_file;
	int c;
	int i,j,i1,j1,i2,j2,i3,j3;
	int **BLOSUM;
	int *row1;
	unsigned char **Prev;
	
	int space,dif,sim,score ;
	int *p;
	int penalty ; // штраф за разрыв
	int endI,endJ; //  указывают на позиции в исходных строках, на которых 
				   //  было закончено выравнивание;
	int beginI,beginJ;// указывают позиции в строках, начиная с которых 
				  // найдено оптимальное выравнивание;
	int difference, similarity;
	PString Str1,Str2;
	BLOCK ** mat;
	PMaxElement L;
	time_t time1,time2;

time1 = clock();

fileS1=fopen("string1.txt","rb");
size_file = size_of_file(fileS1);
String1 = (char*)malloc( size_file * sizeof(char));
M = read_file(fileS1,String1);
fclose(fileS1);

fileS2=fopen("string2.txt","rb");
size_file = size_of_file(fileS2);
String2 = (char*)malloc( size_file * sizeof(char));
N = read_file(fileS2,String2);
fclose(fileS2);

printf("\n Length of First string is  %i\n",M);
printf("\n Length of Second string is  %i\n",N);
// выделяем память для двумерной матрицы BLOSUM;
BLOSUM = (int**)malloc( 5 * sizeof(int*));
	for ( i = 0; i < 5;i ++)
		BLOSUM[i] = (int*)malloc( 5 * sizeof(int));

// считываем матрицу BLOSUM из файла BLOSUM.txt; поставь пробел в матрице после всех чисел
fileS1 = fopen("BLOSUM.txt","r");
fscanf(fileS1," %i",&penalty);
	for(i = 0; i < 5; i++)
		for(j = 0; j < 5; j++){
			fscanf(fileS1," %i",&c);
			while((c!=' ')&&(!feof(fileS1))){
				BLOSUM[i][j] = c;
				c = fgetc(fileS1);
			}

		}

fclose(fileS1);

if ( N%T == 0)
	Kol_block_v_stroke = N/T;
else
	Kol_block_v_stroke = N/T + 1;
	
if ( M%T == 0)
	Kol_block_v_stolbce = M/T;
else
	Kol_block_v_stolbce = M/T + 1;

mat = ( BLOCK ** )malloc( Kol_block_v_stolbce * sizeof(BLOCK *));//M_block
	for ( i = 0; i < Kol_block_v_stolbce ;i ++)
		mat[i] = ( BLOCK *)malloc( Kol_block_v_stroke * sizeof(BLOCK));//N_block

row1 = (int*)malloc( T * sizeof(int));
for ( i = 0; i < T; i++){
row1[i] = 0;
}
//for ( i = 0; i < Kol_block_v_stolbce;i++)
//for ( j = 0; j < Kol_block_v_stroke; j++)
for (j = 0; j <= Kol_block_v_stolbce + Kol_block_v_stroke - 2; j = j + 1)
  {
	 #pragma omp parallel for		
	for (i = max_from_two(0, j - Kol_block_v_stroke + 1); i <= min_from_two(Kol_block_v_stolbce-1, j); i = i + 1)
		  if ( i == 0)
			  if ( j-i == 0)
				  blocks(row1,row1,0,i,j-i,&mat[i][j-i],penalty,BLOSUM);
			  else
				  blocks(row1,mat[i][j-i-1].col,0,i,j-i,&mat[i][j-i],penalty,BLOSUM);
		  else
			  if ( j-i == 0)
				  blocks(mat[i-1][j-i].row,row1,0,i,j-i,&mat[i][j-i],penalty,BLOSUM);
			  else
				  blocks(mat[i-1][j-i].row,mat[i][j-i-1].col,mat[i-1][j-i-1].col[T-1],i,j-i,&mat[i][j-i],penalty,BLOSUM);
}
	
	

/*
if ( Kol_block_v_stroke-1 < Kol_block_v_stolbce)
  {
      for (j = 0; j <= Kol_block_v_stroke-2; j = j + 1)
      {
	    #pragma omp parallel for 
        for (i = 0; i <= j; i = i + 1)
		{ 
			if ( i == 0)
				if ( j-i == 0)
					blocks(row1,row1,0,i,j-i,&mat[i][j-i]);
				else
					 blocks(row1,mat[i][j-i-1].col,0,i,j-i,&mat[i][j-i]);
			else
				if ( j-i == 0)
					blocks(mat[i-1][j-i].row,row1,0,i,j-i,&mat[i][j-i]);//mat[i][j] = *
				else
					blocks(mat[i-1][j-i].row,mat[i][j-i-1].col,mat[i-1][j-i-1].col[T-1],i,j-i,&mat[i][j-i]);
		}
      }
  }
  else
  {
      for (j = 0; j < Kol_block_v_stolbce-1; j = j + 1)
      {
        #pragma omp parallel for 
  	    for (i = 0; i <= j ; i = i + 1)
        {
			if ( i == 0)
				if ( j-i == 0)
					blocks(row1,row1,0,i,j-i,&mat[i][j-i]);
				else
					blocks(row1,mat[i][j-i-1].col,0,i,j-i,&mat[i][j-i]);
			else
				if ( j-i == 0)
					blocks(mat[i-1][j-i].row,row1,0,i,j-i,&mat[i][j-i]);//mat[i][j] = *
				else
					blocks(mat[i-1][j-i].row,mat[i][j-i-1].col,mat[i-1][j-i-1].col[T-1],i,j-i,&mat[i][j-i]);
        }
      }

      for (j = Kol_block_v_stolbce-1; j < Kol_block_v_stroke-1; j = j + 1)
      {
	    #pragma omp parallel for 
        for (i = 0; i < Kol_block_v_stolbce; i = i + 1)
        {
			if ( i == 0)
				if ( j-i == 0)
					blocks(row1,row1,0,i,j-i,&mat[i][j-i]);
				else
					blocks(row1,mat[i][j-1-i].col,0,i,j-i,&mat[i][j-i]);
			else
				if ( j-i== 0)
					blocks(mat[i-1][j-i].row,row1,0,i,j-i,&mat[i][j-i]);//mat[i][j] = *
				else
					blocks(mat[i-1][j-i].row,mat[i][j-i-1].col,mat[i-1][j-i-1].col[T-1],i,j-i,&mat[i][j-i]);
		 }
      }
  }
  if ( Kol_block_v_stroke > Kol_block_v_stolbce)
      for (j = Kol_block_v_stroke-1; j <= Kol_block_v_stolbce + Kol_block_v_stroke - 2; j = j + 1)
      {
	    #pragma omp parallel for 
        for (i = j - Kol_block_v_stroke+1 ; i < Kol_block_v_stolbce; i = i + 1)
        {
			if ( i == 0)
				if ( j-i == 0)
					blocks(row1,row1,0,i,j-i,&mat[i][j-i]);
				else
					blocks(row1,mat[i][j-i-1].col,0,i,j-i,&mat[i][j-i]);
			else
				if ( j-i == 0)
					blocks(mat[i-1][j-i].row,row1,0,i,j-i,&mat[i][j-i]);//mat[i][j] = *
				else
					blocks(mat[i-1][j-i].row,mat[i][j-i-1].col,mat[i-1][j-i-1].col[T-1],i,j-i,&mat[i][j-i]);
        }
      }
  else
  {
      for (j = Kol_block_v_stroke-1; j < Kol_block_v_stolbce-1; j = j + 1)
      {
	    #pragma omp parallel for 
        for (i = j - Kol_block_v_stroke+1 ; i <= j; i = i + 1)
        {	
			if ( i == 0)
				if ( j-i == 0)
					blocks(row1,row1,0,i,j-i,&mat[i][j-i]);
				else
					blocks(row1,mat[i][j-i-1].col,0,i,j-i,&mat[i][j-i]);
			else
				if ( j-i== 0)
					blocks(mat[i-1][j-i].row,row1,0,i,j-i,&mat[i][j-i]);//mat[i][j] = *
				else
					blocks(mat[i-1][j-i].row,mat[i][j-i-1].col,mat[i-1][j-i-1].col[T-1],i,j-i,&mat[i][j-i]);
		}
      }

      for (j = Kol_block_v_stolbce-1; j <= Kol_block_v_stolbce - 2 + Kol_block_v_stroke; j = j + 1)
      {
	    #pragma omp parallel for 

        for (i = j - Kol_block_v_stroke+1; i < Kol_block_v_stolbce; i = i + 1)
        {
               if ( i == 0)
					if ( j-i == 0)
						blocks(row1,row1,0,i,j-i,&mat[i][j-i]);
					else
						blocks(row1,mat[i][j-i-1].col,0,i,j-i,&mat[i][j-i]);
				else
					if ( j-i == 0)
						blocks(mat[i-1][j-i].row,row1,0,i,j-i,&mat[i][j-i]);
					else
						blocks(mat[i-1][j-i].row,mat[i][j-i-1].col,mat[i-1][j-i-1].col[T-1],i,j-i,&mat[i][j-i]); 
        }
      }
  }
*/
L = NULL;
p = &score;
L = maxel(mat,L,p);
printf("SCORE %i ",score);
printf("Alignment\n");
fileS1 = fopen("newS1.txt","w");
fileS2 = fopen("newS2.txt","w");

Prev = (unsigned char**)calloc(T,sizeof(unsigned char*));
	for ( i = 0; i < T;i ++)
		Prev[i] = (unsigned char*)calloc(T,sizeof(unsigned char));	

while( L ){ 
	space = 0;
	dif = 0; 
	sim = 0;

if ( L ){ 
	Str1 = NULL;
	Str2 = NULL;
	i = L->maxi + T * L->BLOCKI;
	j = L->maxj + T * L->BLOCKJ;
	printf(" %i %i ",i,j);
	i1 = L->BLOCKI;
	j1 = L->BLOCKJ;
	if ( L->BLOCKI == 0)
		if ( L->BLOCKJ == 0)
		{
			//clear(i1,j1,Kol_block_v_stolbce-1,Kol_block_v_stroke-1,mat);
			Prev = recollect(row1,row1,0,i1,j1,Prev,penalty,BLOSUM);
		}
		else
		{
			//clear(i1,j1,Kol_block_v_stolbce-1,Kol_block_v_stroke-1,mat);
			Prev = recollect(row1,mat[i1][j1-1].col,0,i1,j1,Prev,penalty,BLOSUM);
		}
	else
		if ( L->BLOCKJ == 0)
		{
			//clear(i1,j1,Kol_block_v_stolbce-1,Kol_block_v_stroke-1,mat);
			Prev = recollect(mat[i1-1][j1].row,row1,0,i1,j1,Prev,penalty,BLOSUM);
		}
		else
		{
			//clear(i1,j1,Kol_block_v_stolbce-1,Kol_block_v_stroke-1,mat);
			
			Prev = recollect(mat[i1-1][j1].row,mat[i1][j1-1].col,mat[i1-1][j1-1].col[T-1],i1,j1,Prev,penalty,BLOSUM);
		}
	
	endI = i+1;
	endJ = j+1;
	i2 = L->maxi;
	j2 = L->maxj;
	printf("Alignment\n");
	while( Prev[i2][j2] != 0 && i2 != -1 && j2 != -1 && i>=0 && j>=0){	
		
			beginI = i+1;
			beginJ = j+1;
			if(i2>=0 && j2>=0 &&  Prev[i2][j2] == 3){ 
				Str1 = Cons('-',Str1);
            	Str2 = Cons(String1[i],Str2);

				space++;
				i = i - 1;
				i2 = i2 - 1;
			}
			
			if(i2>=0 && j2>=0 &&  Prev[i2][j2] == 2 ){
			
				Str1 = Cons(String2[j],Str1);
				Str2 = Cons(String1[i],Str2);
				i = i - 1;
				j = j - 1;
				i2 = i2 - 1;
				j2 = j2 - 1;
				if (Str1->inf == Str2->inf)
					sim++;
				else 
					dif++;
			
			}
				if (i2>=0 && j2>=0 && Prev[i2][j2] == 1) { 
					Str2 = Cons('-',Str2);
           
					Str1 = Cons(String2[j],Str1);
					space++;
					j = j - 1;
					j2 = j2 -1;
				}
	
	if (i2 == -1 || j2 == -1){
		i3 = i1;
		j3 = j1;
		i1 = i/T;
		j1 = j/T;
		
		if ( i>=0 && j>=0 ){
		if ( i1 == 0)
			if ( j1 == 0)
			{
				//clear(i1,j1,i3,j3,mat);
				Prev = recollect(row1,row1,0,i1,j1,Prev,penalty,BLOSUM);
			}
			else
			{
				//clear(i1,j1,i3,j3,mat);
				Prev = recollect(row1,mat[i1][j1-1].col,0,i1,j1,Prev,penalty,BLOSUM);
			}
		else
			if ( j1 == 0)
			{
				//clear(i1,j1,i3,j3,mat);
				Prev = recollect(mat[i1-1][j1].row,row1,0,i1,j1,Prev,penalty,BLOSUM);
			}
			else
			{
				//clear(i1,j1,i3,j3,mat);
				Prev = recollect(mat[i1-1][j1].row,mat[i1][j1-1].col,mat[i1-1][j1-1].col[T-1],i1,j1,Prev,penalty,BLOSUM);
			}
		}
	}
	if ( i2 == -1 )
		i2 = T-1;
	if ( j2 == -1 )
		j2 = T-1;
}

// записываем полученную выравненную строку S1 в файл
// newS1.txt, каждую новую выравненную строку записываем 
// с новой строки в файле и ставим перед ней '*'
printf("\n First string : \n");
fileS1 = write_to_file(fileS1, Str2,beginI,endI);

// записываем полученную выравненную строку S2 в файл
// newS2.txt, каждую новую выравненную строку записываем 
// с новой строки в файле и ставим перед ней '*'
printf("\n Second string : \n");
fileS2 = write_to_file(fileS2,Str1,beginJ,endJ);

printf("\n Score = %i ",score);
printf(" \n Simularities = %i", sim);
printf("\n Differences = %i ",dif);
printf(" \n Spaces = %i \n", space);
printf(" Position of alignment at First string %i %i",beginI,endI);
printf(" \n Position of alignment at Second string %i %i \n",beginJ,endJ);
L = L ->next;  
}
}
time2 = clock();
printf("\n Time =  %i  ", ( time2-time1 ));
  
fclose(fileS1);
fclose(fileS2);

for ( i=0; i< 4;i++)
		free (BLOSUM[i]);
free (BLOSUM);

for ( j=0; j<T;j++)
		free (Prev[j]);
free (Prev);

for ( j=0; j< Kol_block_v_stolbce;j++)
		free (mat[j]);
free (mat);

system("pause");

    return 0;
}