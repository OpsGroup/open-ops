/*
	Simple conditional operator. Contains function call in second part of source conditional operator
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
		b = 0;
		c = f(0);
	}

	return 0;
}