int main()
{
    double A[10],b;
	int i;
	for (i = 0; i < 10; i++)
	{
		if (i>5)
			A[i] = 10 - A[i-1];
		else
			A[i] = 10 + A[i-1];
	}
	return 0;
}
