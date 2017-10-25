#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

char *String1,*String2;	// исходные строки алфавита;
int penalty;
/*
int space;
int sim,dif,Sum;
*/
int BLOSUM[4][4];
char Letter[5] = {'T', 'A', 'G', 'C', '-'};

typedef struct tagString
{
	char inf;
	struct tagString* next;
}String,*PString;

typedef struct tagNode
{
    int i1, j1;
	int i2, j2;
	PString alignments[2];
	struct tagNode* left;
	struct tagNode* right;
	struct tagNode* next;
} Node, *PNode;

Node* root;

PString Cons( PString L,char x )
{
	PString T1;
	T1 = ( PString )malloc( sizeof(String));
	T1->inf = x;
	T1->next = L;
	return T1;
}

unsigned int size_of_file(FILE* fileS1)
{
	unsigned int size_file;
	fseek(fileS1, 0, SEEK_END); // переместить указатель в конец файла
	size_file = ftell(fileS1); // получить текущую позицию
	fseek(fileS1, 0, SEEK_SET); // вернуть указатель на начало
	return size_file;
}

unsigned int read_file(const char* fileName, char** pString)
{
	char b, *String1;
	unsigned int i, size_file;
	FILE* file_string;

	file_string = fopen(fileName,"rb");
	size_file = size_of_file(file_string);
	String1 = (char*)malloc( size_file * sizeof(char));

	i = 0;
	while(!feof(file_string))
	{
		b = fgetc(file_string);
		if (b)
		{
			String1[i] = (toupper(b)-64)%5;
			i++;
		}
	}
	*pString = String1;
	fclose(file_string);
	return i;
}

FILE* write_to_file(FILE* file1, Node* node, int alignNum, char *Letter)
{
	PString Str, Str1;
	int a;
	Str1 = node->alignments[alignNum];

	while( Str1 )
	{
		fprintf(file1,"%c",Letter[Str1->inf]);
		printf("%c",Letter[Str1->inf]);		
        Str = Str1;
		Str1 = Str1->next;
        free(Str);
    }

	if (node->left)
		write_to_file(file1, node->left, alignNum, Letter);
	if (node->right)
		write_to_file(file1, node->right, alignNum, Letter);
	return file1;
}

void Align(Node* node)
{
	int i,j;
	int max,max2,max3,w;
	int **H;
	int Score, ScoreDiag,ScoreUp,ScoreLeft;
	int i1 = node->i1, i2 = node->i2,
	    j1 = node->j1, j2 = node->j2;

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
	{
		for ( j = 1; j <= j2-j1; j++)
		{
			w = BLOSUM[String1[i1+i-1]][String2[j1+j-1]];
			max = H[i-1][ j-1] + w;		
			max2 =  H[i][ j-1] + penalty;
			max3 = H[i-1][ j] + penalty;
		
			if (max2 > max)
				max = max2;
			
			if (max3 > max)
				max = max3;

			H[i][j] = max;
		}
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
			i = i - 1;
			j = j - 1;
		}
		else if (Score == ScoreLeft + penalty)
		{
			AlignmentString1 = Cons( AlignmentString1,String1[i1+i-1]);
			AlignmentString2 = Cons( AlignmentString2,4);
			i = i - 1;
			//space++;
		}
		else if (Score == ScoreUp + penalty)
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

	node->alignments[0] = AlignmentString1;
	node->alignments[1] = AlignmentString2;

	for ( j = 0; j <= i2-i1;j++)
		free (H[j]);
	free (H);
}

Node* CreateNode(int i1, int j1, int i2, int j2)
{
	Node* new = (Node*)malloc(sizeof(Node));
	memset(new, 0, sizeof(Node));
	new->i1 = i1;
	new->j1 = j1;
	new->i2 = i2;
	new->j2 = j2;
	return new;
}

void Hirschberg(int *Low_alignment,int *Top_alignment, int M, int N)
{
	int i,j;
	int s,c,w,max2,max3;
	int mid,jmax;

	PNode current, end;

	root = CreateNode(0,0,M,N);
	end = root;
	current = root;

	while(current != NULL)
	{
	    int i1 = current->i1, i2 = current->i2,
		    j1 = current->j1, j2 = current->j2;
			
		printf("%");

		if ((i2-i1)<= 1 || (j2-j1) <= 1)
		{
			Align(current);
		}
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
					w = BLOSUM[String1[i-1]][String2[j-1]] ;	
					max2 = Low_alignment[j] + penalty;
					max3 = c + penalty;
					c = s + w;

					if (max2 > c)
						c = max2;
					if (max3 > c) 
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
					w = BLOSUM[String1[i]][String2[j]] ;
					max2 = Top_alignment[j] + penalty;
					max3 = c + penalty;
					c = s + w;	

					if (max2 > c)
						c = max2;
					if (max3 > c) 
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

			current->left = CreateNode(i1,j1,mid,jmax);
			current->right = CreateNode(mid,jmax,i2,j2);
			end->next = current->left;
			end->next->next = current->right;
			end = current->right;
		}
		current = current->next;
	}
}

int main(int argc, char** argv)
{
	int i,j;
	int c;

	int M,N;	// длины строк S1 и S2;
	int *Low, *Top;
	FILE *fileS1,*fileS2;// файл fileS1 содержит строку S1 длины M,
							 // файл fileS2 содержит строку S2 длины N, 
							 // файл file2 хранит матрицу сравнений 
							 // нуклеотидов BLOSUM;

	if (argc < 3)
	{
	   printf("Usage: hirschberg <file1> <file2>\n");
	   return 1;
	}

	M = read_file(argv[1], &String1);
	N = read_file(argv[2], &String2);

	printf("\n Length of First string is  %i\n",M);
	printf("\n Length of Second string is  %i\n",N);

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

	Hirschberg(Low,Top,M,N);

	printf("\n First string : \n");
	fileS1 = fopen("newS1.txt","w");
	fileS1 = write_to_file(fileS1, root, 0, Letter);
	fclose(fileS1);

	// записываем полученную выравненную строку S2 в файл
	// newS2.txt, каждую новую выравненную строку записываем 
	// с новой строки в файле и ставим перед ней '*'
	printf("\n Second string : \n");
	fileS2 = fopen("newS2.txt","w");
	fileS2 = write_to_file(fileS2, root, 1, Letter);
	fclose(fileS2);

	free(String1);
	free(String2);

	free(Low);
	free(Top);

	system("pause");
    return 0;
}