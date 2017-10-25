//Алгоритм построения матрицы достижимостей графа, основанный на блочном булевом перемножении матриц.
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#define attemptsCount 2

int show(int **input,int size) //print the massive "input" on the screen
{
    int i,j;
    for ( i = 0 ; i<size ; i++)
        {
        for ( j = 0 ; j<size ; j++)
            {
                printf("%d",input[i][j]);
                printf(" ");
            }
            printf("\n");
        }
    return 0;
}


int generator(int **input, int size)
{//generator makes input which exists from {0;1}
    clock_t timec;

    int i,j;
    for (i=0; i<size; i++)
       input[i]=(int*)malloc(size*sizeof(int));
    srand(timec);
    for (i=0; i<size;i++)
    {
        timec = clock();
        for (j=0; j<size; j++)
        {
            timec = (int)((clock() - timec)/CLOCKS_PER_SEC);
            input[i][j]=rand()%2;
        }
        input[i][i] = 1;
    }
    show(input,size);
    printf("mission completed\n");
    return 0;
}

int create_block_line(int **input,int **out, int size,int param,int indexi,int indexk) //param is p = n/d, where d in[40..100]; step is number of block
{//вытаскивает из строки матрицы input блок a[i][k]
    int i,j;

    for (i = indexi*param ; i<param*(indexi+1) ; i++ )
        for ( j = indexk*param ; j<param*(indexk+1) ; j++)
            out[i-indexi*param][j-indexk*param] = input[i][j];
    return 0;
}


int create_block_column(int **input,int **out, int size,int param,int indexk,int indexj) //param is p = n/d, where d in[40..100]; step is number of block
{//вытаскивает из строки матрицы input блок a[k][j]
    int i,j;

    for (i = indexk*param ; i<param*(indexk+1) ; i++ )
        for ( j = indexj*param ; j<param*(indexj+1) ; j++)
            out[i-indexk*param][j-indexj*param] = input[i][j];
    return 0;
}

int bring_block_back(int **big,int **small, int size,int param,int indexi,int indexj)
{//возвращает блок в матрицу big
    int i,j;

    for (i = indexi*param ; i<param*(indexi+1) ; i++ )
        for ( j = indexj*param ; j<param*(indexj+1) ; j++)
            big[i][j] = small[i-indexi*param][j-indexj*param];
    return 0;
}


int sum(int **input1, int **input2,int **out,int size) //суммирует матрицы и возвращает результат в out
{
    int i,j;
    int **temp =(int**)malloc(size*sizeof(int*));
    for (i=0; i<size; i++)
            temp[i]=(int*)malloc(size*sizeof(int));
    for ( i = 0 ; i<size ; i++)
        for ( j = 0 ; j<size ; j++)
        temp[i][j] = 0;

    for ( i = 0 ; i<size ; i++)
        for ( j = 0 ; j<size ; j++)
                temp[i][j] = input1[i][j] + input2[i][j];

     for ( i = 0 ; i<size ; i++)
        for ( j = 0 ; j<size ; j++)
        out[i][j] = !(temp[i][j]==0);

    for (i=0; i<size; i++)
        free(temp[i]);
    free(temp);
     return 0;
}

int multy(int **input1, int **input2,int **out,int size) //multyplies massive input1 on input2 and returns out
{
    int i,j,k;
    int **temp =(int**)malloc(size*sizeof(int*));
    for (i=0; i<size; i++)
            temp[i]=(int*)malloc(size*sizeof(int));
    for ( i = 0 ; i<size ; i++)
        for ( j = 0 ; j<size ; j++)
        temp[i][j] = 0;

    for ( i = 0 ; i<size ; i++)
        for ( j = 0 ; j<size ; j++)
            for ( k = 0 ; k<size ; k++)
                temp[i][j] = temp[i][j] + input1[i][k]*input2[k][j];

     for ( i = 0 ; i<size ; i++)
        for ( j = 0 ; j<size ; j++)
        out[i][j] = !(temp[i][j]==0);
    for (i=0; i<size; i++)
        free(temp[i]);
    free(temp);
     return 0;
}


int todegree(int **input,int **out,int size, int degree,int param) //input to deree and returns out
{
    int i,j;
    int **temp =(int**)malloc(size*sizeof(int*));
    for (i=0; i<size; i++)
            temp[i]=(int*)malloc(size*sizeof(int));
    for ( i = 0 ; i<size ; i++)
        for ( j = 0 ; j<size ; j++)
        temp[i][j] = input[i][j];

    multy(temp,temp,temp,size);

    for ( i = 0 ; i<size ; i++)
        for ( j = 0 ; j<size ; j++)
            out[i][j] = temp[i][j];

    for (i = 1; i <= degree ; i++)
        block_mult_ed(temp,out,out,size,param);

    for (i=0; i<size; i++)
        free(temp[i]);
    free(temp);
    return 0;
}


int block_mult_ed(int **input1, int **input2,int **out,int size, int param) //like multy, but makes matrix block and sent it to multy
{
    int i,j,k,a,b;
    int p = size/param;
    int **temp =(int**)malloc(size*sizeof(int*));
    for (i=0; i<size; i++)
            temp[i]=(int*)malloc(size*sizeof(int));
    for ( i = 0 ; i<size ; i++)
        for ( j = 0 ; j<size ; j++)
        temp[i][j] = 0;

    int **temp_min =(int**)malloc(param*sizeof(int*));
    for (i=0; i<param; i++)
            temp_min[i]=(int*)malloc(param*sizeof(int));
    for ( i = 0 ; i<param ; i++)
        for ( j = 0 ; j<param ; j++)
        temp_min[i][j] = 0;

    int **line =(int**)malloc(param*sizeof(int*));
    for (i=0; i<param; i++)
            line[i]=(int*)malloc(param*sizeof(int));
    for ( i = 0 ; i<param ; i++)
        for ( j = 0 ; j<param ; j++)
        line[i][j] = 0;

    int **column=(int**)malloc(param*sizeof(int*));
    for (i=0; i<param; i++)
            column[i]=(int*)malloc(param*sizeof(int));
    for ( i = 0 ; i<param ; i++)
        for ( j = 0 ; j<param ; j++)
        column[i][j] = 0;

//в цикле должны перемножиться все блоки и вернуться в новую матрицу temp
    for ( i = 0 ; i<p ; i++)
        for ( j = 0 ; j<p ; j++)
            {
           for ( k = 0 ; k<p ; k++)
            {
                create_block_line(input1,line,size,param,i,k); //создаем блок по строке [i][k]
                create_block_column(input2,column,size,param,k,j); //создаем блок по столбцу [k][j]
                multy(line,column,line,param); //input1[i][k]*input2[k][j];
                sum(temp_min,line,temp_min,param); //temp[i][j] = temp[i][j] + input1[i][k]*input2[k][j];
            }
            bring_block_back(temp,temp_min,size,param,i,j);//возвращаем полученный блок [i][j] в temp
        for ( a = 0 ; a<param ; a++)
        for ( b = 0 ; b<param ; b++)
        temp_min[a][b] = 0;
            }

     for ( i = 0 ; i<size ; i++)
        for ( j = 0 ; j<size ; j++)
        out[i][j] = temp[i][j];
    for (i=0; i<size; i++)
        free(temp[i]);
    free(temp);
    for (i=0; i<param; i++)
        free(temp_min[i]);
    free(temp_min);
    for (i=0; i<param; i++)
        free(line[i]);
    free(line);
    for (i=0; i<param; i++)
        free(column[i]);
    free(column);
     return 0;
}

int main()
{
    clock_t t1, t2;
    int res[attemptsCount];
    int n, deg,i,logarifm,param,r,avg,min;
    int **a,**out; //based massive and result matrix

    printf("Hello world!\n");
    printf("Enter the dimension of massive\n");
    scanf("%d",&n);

    a=(int**)malloc(n*sizeof(int*));
    out=(int**)malloc(n*sizeof(int*)); // вспомогательная матрица для возведения в степень
    for (i=0; i<n; i++)
       out[i]=(int*)malloc(n*sizeof(int));

    printf("Enter the parametr of break up\n");
    scanf("%d",&param);
    printf("\n");
    logarifm = (int)floor((log(n)/log(2)));//максимально требуемая достижимость/длина пути
    printf("We had to do ");
    printf("%d",logarifm);
    printf(" steps\n");

    generator(a,n); //makes massive random
    for (r = 0; r < attemptsCount; r++)
    {
        t1 = clock()/ CLOCKS_PER_SEC;
        todegree(a,out,n,logarifm,param);
        t2 = clock()/ CLOCKS_PER_SEC;
        printf("Achieve matrix in bool looks ...\n");
      //  show(out,n);

        res[r] = t2 - t1;
        printf("\n %d", r+1, res[r]);
    }

    printf("Dimension of massive: %d\n",n);
    printf("Time in milliseconds:");

    avg = 0;
    min = res[0];
    for (r = 0; r < attemptsCount; r++)
    {
        printf("\n%2d) %d", r+1, res[r]);
        if (min > res[r])
            min = res[r];
        avg+=res[r];
    }
    avg /= attemptsCount;
    printf("\nMean value : %d", avg);
    printf("\nMinimum value: %d", min);
    printf("\n============================\n\n");
    printf("\n");
	
    for (i=0; i<n; i++)
            free(a[i]);
    free(a);
    for (i=0; i<n; i++)
            free(out[i]);

    free(out);
    return 0;
}