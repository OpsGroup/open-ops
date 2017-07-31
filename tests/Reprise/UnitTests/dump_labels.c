int main()
{
  int i, j;
  i = 0; 
  j = 0;
  lab: i = i + 1;
  lab2: j = j + i;
  if (j < 10)
    goto lab;
  return 0; 
}
