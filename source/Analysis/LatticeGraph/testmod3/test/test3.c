int main()
{int i,j,x[10][10],n1,n2,n3,n4,n5;
for (i=0;i<10;i++)
  {
  for (j=0;j<10;j++)
     {
      x[i][j]=x[i-1][j];
      }
  }
}
