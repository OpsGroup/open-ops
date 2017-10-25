/*
	Simple conditional operator. Contains function call in first part of source conditional operator
*/

int f(int a)
{
	return a;
}

int main()
{
	int a, b, c;
	
	if(a)
	{
		b = f(0);
		c = 0;
	}

	return 0;
}