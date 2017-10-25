/*
	Simple conditional operator. Contains goto statement in first part
*/

int main()
{
	int a, b, c;

	if(a)
	{
		goto A;
		c = 0;
	}
A:
	return 0;
}