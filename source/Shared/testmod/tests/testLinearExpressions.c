int main()
{
	int m;
	int A[10][10];
	int B[10][20];
	int C[10];
	int i;
	int j;
	int k;

	for(i=0;i<10;i=i+1)
		for(j=0;j<i+2*j+10;j=j+1)
			k=A[4*i+3*j+2][j-3];
	return 0;
}
