/*
	Simple conditional operator. Contains labeled source conditional operator
*/
int main()
{
	int a;
	int b;
	int c;
A:
	if(a && (b + a))
	{
		c = -a;
	}
	else
	{
		b = 0;
	}

	return 0;
}