int main()
{
  signed char sc;
  unsigned char uc;
  int i;
  unsigned int ui;
  float f;
  double d;
  int* pi;

  -sc; // 0 int
  +sc; // 1 int
  !sc; // 2 int
  -uc; // 3 int
  +uc; // 4 int
  !uc; // 5 int
  uc << sc; // 6 int
  uc >> sc; // 7 int

  i + i; // 8 int
  uc - i; // 9 int
  i + f; // 10 float
  f + d; // 11 double
  pi - sc; // 12 int*
  i + pi; // 13 int*
  i + ui; // 14 uint
  sc & uc; // 15 int
}
