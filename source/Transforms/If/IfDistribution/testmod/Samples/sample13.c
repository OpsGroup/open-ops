/*
	Simple conditional operator. Contains antidependence from source conditional operator to operator in first part
*/

int main()
{
	int a, b, c;
	
	if(a = b)
	{
		b = 0;
		c = 0;
	}

	return 0;
}