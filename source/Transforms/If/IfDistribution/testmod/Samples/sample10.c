/*
	Simple conditional operator. Contains function call in condition of source conditional operator
*/

int f(int a)
{
	return a;
}

int main()
{
	int a, b, c;
	
	if(f(a))
	{
		b = 0;
		c = 0;
	}

	return 0;
}