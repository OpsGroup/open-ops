#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

char *String1,*String2;	// исходные строки алфавита;
int penalty;
/*
int space;
int sim,dif,Sum;
*/
int **BLOSUM;
char *Letter;

typedef struct 
{
	char inf;
	struct String* next;
}String,*PString;

PString Start_alignment1,Start_alignment2;
PString Current_alignment1,Current_alignment2;

PString Cons( PString L,char x )
{
	PString T1;
	T1 = ( PString )malloc( sizeof(String));
	T1->inf = x;
	T1->next = L;
return T1;
}

unsigned int size_of_file( FILE* fileS1)
{
	unsigned int size_file;
	fseek(fileS1, 0, SEEK_END); // переместить указатель в конец файла
	size_file = ftell(fileS1); // получить текущую позицию
	fseek(fileS1, 0, SEEK_SET); // вернуть указатель на начало
	return size_file;
}

unsigned int read_file( FILE* file_string, char* String)
{
	char b;
	unsigned int i;
	i = 0;
	while(!feof(file_string)){
		b = fgetc(file_string);
		if (b)
		{
			String[i] = (toupper(b)-64)%5;
			//printf("%i",String[i]);
			i++;
		}
	}
return i;
}
FILE* write_to_file( FILE* file1, PString Str1,char *Letter)
{
	PString Str;
	int a;
	fprintf(file1," %c",'*'); 
	while( Str1 ){
		fprintf(file1,"%c",Letter[Str1->inf]);
		printf(" %c ",Letter[Str1->inf]);		
        Str = Str1;
		Str1 = Str1->next;
        free(Str);
    }
	fprintf(file1,"%c",'\n');
	return file1;
}


void Align(int i1,int j1,int i2,int j2)
{
	int i,j;
	int max,max2,max3,w;
	int **H;
	int Score, ScoreDiag,ScoreUp,ScoreLeft;
	PString AlignmentString1,AlignmentString2,P1,P2;

	// выделяем память для матрицы Н;
    H = (int**)malloc( (i2-i1+1)*sizeof(int*));
	for ( i = 0; i < (i2-i1+1);i ++)
		H[i] = (int*)malloc((j2-j1+1)*sizeof(int));

	for(i = 0; i <= i2-i1; i++){
	H[i][0] = penalty*i;
	}

	for( j = 1; j <= j2-j1; j++){
	H[0][j]=penalty*j;
	}		

// строим матрицу Н по следующему принципу:
// находим максимум среди элементов 
for (i = 1; i<= i2-i1; i++)
	for ( j = 1; j <= j2-j1; j++)
	{		
        w = BLOSUM[(int)String1[i1+i-1]][(int)String2[j1+j-1]] ;
		max = H[i-1][ j-1] + w;		
		max2 =  H[i][ j-1] + penalty;
		max3 = H[i-1][ j] + penalty;
		
		if (  max2 > max )
			max = max2;
			
		if (  max3 > max ) 
			max = max3;

		H[i][j] = max;
	}

 i = i2-i1;
 j = j2-j1;

 AlignmentString1 = NULL;
 AlignmentString2 = NULL;
 P1 = NULL;
 P2 = NULL;
 
  while (i > 0 && j > 0)
  {
    Score =  H[i][j];
    ScoreDiag = H[i - 1][ j - 1];
    ScoreUp = H[i][ j - 1];
    ScoreLeft = H[i - 1][ j];
    if (Score == ScoreDiag + BLOSUM[(int)String1[i1+i-1]][(int)String2[j1+j-1]])
    {
      AlignmentString1 = Cons( AlignmentString1,String1[i1+i-1]);
      AlignmentString2 = Cons( AlignmentString2,String2[j1+j-1]);
/*	 
	  if (AlignmentString1->inf == AlignmentString2->inf)
					sim++;
				else 
					dif++;
		Sum+= BLOSUM[(int)String1[i1+i-1]][(int)String2[j1+j-1]];
*/
      i = i - 1;
      j = j - 1;
    }
    else 
		if (Score == ScoreLeft + penalty)
    {
		AlignmentString1 = Cons( AlignmentString1,String1[i1+i-1]);
		AlignmentString2 = Cons( AlignmentString2,4);
		i = i - 1;
		//space++;
    }
		else 
			if(Score == ScoreUp + penalty)
				{
					AlignmentString1 = Cons( AlignmentString1,4);
					AlignmentString2 = Cons( AlignmentString2,String2[j1+j-1]);
					j = j - 1;
					//space++;
				}
	if ( P1 == NULL )
	{
		P1 = AlignmentString1;
		P2 = AlignmentString2;
	}
  }
  while (i > 0)
  {
	  AlignmentString1 = Cons( AlignmentString1,String1[i1+i-1]);
      AlignmentString2 = Cons( AlignmentString2,4);
	  i = i - 1;
	 // space++;
	  if ( P1 == NULL )
	{
		P1 = AlignmentString1;
		P2 = AlignmentString2;
	}
  }
  while (j > 0)
  {
	  AlignmentString1 = Cons( AlignmentString1,4);
      AlignmentString2 = Cons( AlignmentString2,String2[j1+j-1]);
	  //space++;
	  if ( P1 == NULL )
	{
		P1 = AlignmentString1;
		P2 = AlignmentString2;
	}
	  j = j - 1;
  }
  if ( Start_alignment1 == NULL ){
	  Start_alignment1 = AlignmentString1;
	  Start_alignment2 = AlignmentString2;
	  Current_alignment1 = P1;
	  Current_alignment2 = P2;
  }
  
  else
 {
	 Current_alignment1->next = AlignmentString1;
	 Current_alignment2->next = AlignmentString2;
	 Current_alignment1 = P1;
	 Current_alignment2 = P2;
 }

  for ( j = 0; j <= i2-i1;j++)
		free (H[j]);
free (H);
}


void Hirschberg( int *Low_alignment,int *Top_alignment,int i1,int j1,int i2,int j2)
{
	int i,j;
	int s,c,w,max2,max3;
	int mid,jmax;
	
	if ((( i2 - i1)<= 1)||((j2-j1) <= 1))
		Align(i1,j1,i2,j2);
	else
	{
		mid = (i1 + i2)/2;
		Low_alignment[j1] = 0;
		for ( j = j1+1; j <= j2; j++)
			Low_alignment[j] = Low_alignment[j-1]+ penalty;
		for ( i = i1 + 1; i <= mid; i++)
		{
			s = Low_alignment[j1];
			c = Low_alignment[j1] + penalty;
			Low_alignment[j1] = c;
			for ( j = j1+1; j <= j2; j++)
			{ 
				w = BLOSUM[(int)String1[i-1]][(int)String2[j-1]] ;	
				max2 = Low_alignment[j] + penalty;
				max3 = c + penalty;
				c = s + w;

				if (  max2 > c )
					c = max2;
				if ( max3 > c ) 
					c = max3;
				s = Low_alignment[j];
				Low_alignment[j] = c;
			}
		}
		Top_alignment[j2] = 0;
		for ( j = j2 - 1; j >= j1; j--)
			Top_alignment[j] = Top_alignment[j+1] + penalty;
		for ( i = i2 - 1; i >= mid; i--)
		{
			s = Top_alignment[j2];
			c = Top_alignment[j2] + penalty;
			Top_alignment[j2] = c;
			for ( j = j2 - 1; j >= j1; j--)
			{
				w = BLOSUM[(int)String1[i]][(int)String2[j]] ;
				max2 = Top_alignment[j] + penalty;
				max3 = c + penalty;
				c = s + w;	

				if (  max2 > c )
					c = max2;
				if ( max3 > c ) 
					c = max3;
				s = Top_alignment[j];
				Top_alignment[j] = c;
			}
		}
		max2 = Top_alignment[j1] + Low_alignment[j1];
		jmax = j1;
		for ( j = j1 + 1; j <= j2; j++)
		{
			max3 = Top_alignment[j] + Low_alignment[j];
			if ( max3 > max2 ) 
			{
				max2 = max3;
				jmax = j;
			}
		}
	
Hirschberg(Low_alignment,Top_alignment,i1,j1,mid,jmax);
Hirschberg(Low_alignment,Top_alignment,mid,jmax,i2,j2);
}
}

int main()
{
int i,j;
int c;

int M,N;	// длины строк S1 и S2;
int *Low, *Top;
FILE *fileS1,*fileS2;// файл fileS1 содержит строку S1 длины M,
							 // файл fileS2 содержит строку S2 длины N, 
							 // файл file2 хранит матрицу сравнений 
							 // нуклеотидов BLOSUM;
int size_file;


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
BLOSUM = (int**)malloc( 4 * sizeof(int*));
	for ( i = 0; i < 4;i ++)
		BLOSUM[i] = (int*)malloc( 4 * sizeof(int));

// считываем матрицу BLOSUM из файла BLOSUM.txt; поставьте пробел в матрице после всех чисел
fileS1 = fopen("BLOSUM.txt","r");
fscanf(fileS1," %i",&penalty);
	for(i = 0; i < 4; i++){
		for(j = 0; j < 4; j++){
			fscanf(fileS1," %i",&c);
				BLOSUM[i][j] = c;
		}
	}
fclose(fileS1);

Low = (int*)malloc((N+1) * sizeof(int));
Top = (int*)malloc((N+1) * sizeof(int));
Start_alignment1 = NULL;
Start_alignment2 = NULL;
Letter = (char*)malloc( 5 * sizeof(char));
Letter[0] = 'T';
Letter[1] = 'A';
Letter[2] = 'G';
Letter[3] = 'C';
Letter[4] = '-';
/*
space = 0;
sim = 0;
dif = 0;
Sum=0;
*/
Hirschberg(Low,Top,0,0,M,N);


printf("\n First string : \n");
fileS1 = fopen("newS1.txt","w");
fileS1 = write_to_file(fileS1,  Start_alignment1,Letter);
fclose(fileS1);

// записываем полученную выравненную строку S2 в файл
// newS2.txt, каждую новую выравненную строку записываем 
// с новой строки в файле и ставим перед ней '*'
printf("\n Second string : \n");
fileS2 = fopen("newS2.txt","w");
fileS2 = write_to_file(fileS2,Start_alignment2,Letter);
fclose(fileS2);
  
for ( i=0; i< 4;i++)
		free (BLOSUM[i]);
free (BLOSUM);

free(String1);
free(String2);

free(Low);
free(Top);

system("pause");
    return 0;
}