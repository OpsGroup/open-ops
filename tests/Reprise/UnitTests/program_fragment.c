int main()
{
  int a[5];
  int b[5];

  int i;
  for (i = 0; i < 5; ++i)
  {
     a[i] += b[i];
     b[i] += a[i];
  }

  return 0;
}
