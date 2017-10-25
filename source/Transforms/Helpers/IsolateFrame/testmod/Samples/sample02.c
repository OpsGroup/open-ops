
int  main()
{
	int i, j, k;
	const int N = 100;
	int X[N];
	int Y[N];
	int Z[N];


	for (i = 0; i < N; i++)
		for(j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				Z[i] = X[k] * Y[j];
	return 0;
	
}

