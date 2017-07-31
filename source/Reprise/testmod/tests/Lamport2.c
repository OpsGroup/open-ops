int b[50][50],a[50],x[50],k;

int main()
{ 
 int i,j;
 for(i=1;i<10;i=i+1)
 {for ( j=1 ;j <10; j=j+1)
 {
  a[i+j]=b[i][j-1]+b[i-1][j];
  b[i][j]=a[i+j-2]+b[i][j];
 }
 }

}

