int i;
int A[10];
int B[10];
int X[10];
int Y[10];
int Z[10];
int W[10];

int main()
{
  for (i=100; i>0; i=i-1)
  {
    X[i] = A[i]+1;
	Y[i] = B[i]+2;
	Z[i] = X[i]+3;
    W[i] = Y[i]+4;
  }

  return 0;
}
