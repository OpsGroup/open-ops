int a;
int k;
int *p;

int* fn(int *i)
{
  i=&k;
  p=&a;
  return p;
}

int main()
{
  int b;
  int c;
  int m[10];
  int *q;

  q=&b;
  p=&c;
  q=fn(q);
 
  a=*p;
  b=*q; 
}
