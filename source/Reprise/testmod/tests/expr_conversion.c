volatile int B[20];


int* func(int n)
{
	return &n;
}

int main()
{
	int A[10];
	int i, j;

	j = i += 2;		// complex assign in expr stmt 
	j = i++;		// postfix in expr stmt

	(*func(1))++;	// posfix with deref
	//error: lvalue required as increment operand // *(func(1)++);	// posfix with deref
	&A[0]++;		// postfix array addr
	B[0]++;

	*func(1) += 5;
	&A[0] += 2;
	&B[0] += 2;
	B[0] += 2;
	i += i++ + 2;
	i *= 5;

    for (i = 0; i < 20; i += 2)
    {
		if (i > 50)
			break;
        B[i] = i;
    }

	for (i = 0; i < 20; i++)
    {
		if (i < 5)
			continue;
        B[i] = i;
    }

	for (i = 0; i < 20; ++i)
    {
        B[i] = i;
    }
	i = 0;

	return 0;
}