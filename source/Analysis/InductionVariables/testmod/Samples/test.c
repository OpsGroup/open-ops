#define N 100

int main()
{
  int i,j;
  int a,b;
  float A[N], B[N], C[N];
  float AA[N][N], BB[N][N], CC[N][N];
  a=0;

  // простой параллельный цикл
  for(i = 0; i < N; ++i)
    A[i] = i;

  // в этом цикле выходная зависимость
  for(i = 0; i < N; ++i)
    A[0] = i;

  // выходная зависимость по скалярной переменной
  for(i = 0; i < N; ++i)
  {
	a=A[i];
  }

  // циклически независимая антизависимость
  for(i = 0; i < N; ++i)
    A[i] = A[i] + B[i];

  // циклически порожденная антизависимость
  for(i = 0; i < N; ++i)
    A[i] = A[i-1] + B[i];

  for(i = 0; i < N; ++i)
    A[i] = B[i] + C[i];

  for(i = 0; i < N; ++i)
  {
    a = A[i];
    B[i] = A[i] + a;
  }

  for(i = 0; i < N; ++i)
    a = a + A[i];

  for(i = 0; i < N; ++i)
    for(j = 0; j < N; ++j)
      AA[i][j] = AA[i][j] + 1;

  for(i = 0; i < N; ++i)
    for(j = 0; j < N-1; ++j)
      AA[i][j] = AA[i][j+1] + 1;

  for(i = 0; i < N; ++i)
  {
    if ((i % 2) == 0)
    {
      a = A[i];
    }

    B[i] = a;
  }

  for(i = 1; i < a; ++i)
  {
  }

  return 0;
}
