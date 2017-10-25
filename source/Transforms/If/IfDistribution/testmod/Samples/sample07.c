/*
	Simple conditional operator. Contains goto statement in second part
*/

int main()
{
	int a, b, c;

	if(a)
	{
		b = 0;
		c = 0;
		goto A;
	}
A:
	return 0;
}