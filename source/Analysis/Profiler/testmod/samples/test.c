
int filter(int A[10][10], int i, int j)
{
	return ((A[i+1][j] + A[i][j+1] + A[i-1][j] + A[i][j-1]) / 4);
}


int main() {
	int A[10][10];
	int B[10][10];
	int S;
	int i,j;

	S = 0;

	for(i = 0; i < 10; i++)
	{
		for(j = 0; j < 10; j++)
		{
			B[i][j] = filter(A,i,j);
			S = S + A[i][j];;
		};	
	}		

	S = S + 10;
	return 0;;
}