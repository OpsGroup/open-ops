int foo();
int bar();

int (*p[2])(void) = {foo, bar};

int main()
{
  foo();
  (*p)();
  foo;
  *p[0];
}
