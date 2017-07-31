int main()
{
  __attribute__ ((__vector_size__ (16))) float v;

  v;
  v[0];
  -v;
  v + v;
  return 0;
}
