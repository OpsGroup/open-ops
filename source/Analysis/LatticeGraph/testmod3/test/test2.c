int i, j, k;
int uf[365][195][8], vf[365][195][8], fsm[365][195];

int main()
{
  for(k=0; k < 8; ++k)
  {
	for(j=0; j < 195; ++j)
	{
	  uf[365][j][k] = 1.e-10;
	  vf[365][j][k] = 1.e-10;
	  uf[1][j][k] = 1.e-10;
	  vf[1][j][k] = 1.e-10;
	}
	for(j=0; j < 195; ++j)
	{
	  for(i=0; i < 365; ++i)
	  {
		uf[i][j][k] = uf[i][j][k]*fsm[i][j];
		vf[i][j][k] = vf[i][j][k]*fsm[i][j];
	  }
	}
  }
  
  return 0;
}