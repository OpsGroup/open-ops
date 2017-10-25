int main()
{
	int i, k=0, p = 5, A[100];
	int j = 0;
	i=0;	
	while(i<10) 
	{	
		j = i + 1;
		A[k] = A [p];
		k = k + 1;
		p = k + j;
		A[k] = A [p];
		i = j + 1;
	}
return i;
}