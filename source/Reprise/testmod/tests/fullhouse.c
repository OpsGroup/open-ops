int i, j;
double f, e;
char c;
int A[10];
volatile int B[20];

int* func(int n)
{
	return n;
}

int main()
{
	int a, b, c, d, e, f;


//	a = (b + c * d - *func(f), a, b, c);
	i = i += 2;
	*func(1)++;
	&A[0]++;
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
    while (i < 10)
    {
        A[i] = i;
		continue;
		i += 1;
    }
	if (i < 20)
	{
		i = 1;
	}
	if (i > 0 && i < 10)
	{
		i = 0;
	}
	else
	{
		j = i + 2;
	}

l1:	switch (i)
	{
	case 0:
		j = 0;
		break;
	case 1:
		j = 1;
		break;
	case 2:
		j = 2;
		break;
	default:
		j = 12;
	}
	goto l1;

    return 0;
}