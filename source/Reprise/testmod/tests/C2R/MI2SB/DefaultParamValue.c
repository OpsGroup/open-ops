//	Actually here: int f(int a = 12)
int f(int a)
{
	int bb = 12;
	return a + bb;
}

int main()
{
	return f(13);
}