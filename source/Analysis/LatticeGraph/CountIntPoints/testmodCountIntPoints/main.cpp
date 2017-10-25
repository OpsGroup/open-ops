#include <stdlib.h>
#include "../FunctionsVerge.h"
#include "../IOfunctions.h"
#include "../RationalMatrix.h"


using namespace OPS::LatticeGraph::CountIntPoints;

int main(int argc,char** argv)
{

	matrix mat, ray, bidray,incmat;
	vector inequality; /* vector of flags equation/inequality */
	int nbconstraints, dimension, nbcolumns, nbcolumns2, i, j;

	
	std::ifstream filein;
	filein.open("infile.dat");
	if (!filein) std::cout<<"not open infile";
	//Read_Matrix(&mat);
	Read_MatrixFromFile(filein,&mat);
	filein.close();
	partritionEqualAndInequal(&mat);
	nbconstraints=mat.nbrows;
	nbcolumns=mat.nbcolumns;
	dimension=nbcolumns-1;
	nbcolumns2=dimension+nbconstraints;

	std::ofstream fileout;
	fileout.open("outfile.dat");
    if (!fileout) std::cout<<"can't open out file\n";

	//Alloc_Matrix(&ray, atoi(argv[1]), nbcolumns2);
	Alloc_Matrix(&ray, 50, nbcolumns2);
	Alloc_Matrix(&bidray, dimension, nbcolumns2);
	/* set the identity matrix as bidirectional rays */

	for (i=0;i<dimension;i++)
	{
		for (j=0;j<dimension;j++)
			bidray.p[i][j]=0;
		bidray.p[i][i]=1;
	}

	for (i=0;i<nbconstraints;i++)
		for (j=1;j<nbcolumns;j++)
			bidray.p[j-1][i+dimension]=mat.p[i][j] ;

	Alloc_Vector(&inequality, nbcolumns2);

	for (i=0;i<nbconstraints;i++)
		inequality.p[i+dimension]=mat.p[i][0] ;

	int res=chernikova(&bidray, &ray, dimension, 0,&inequality);
	if (res!=0)
	{
		printf("not enough rays available\n"); //exit(2);
	}

	
	//printf("bidirectional rays\n");
	//Write_Matrix(&bidray, mat.nbcolumns-1);
    Write_MatrixToFile(fileout,&mat,mat.nbcolumns);
	fileout<<"bidirectional rays\n";
	Write_MatrixToFile(fileout,&bidray,mat.nbcolumns-1);
	
	
	
	fileout << "unidirectional rays\n";
    Write_MatrixToFile(fileout,&ray,mat.nbcolumns-1);
	//printf("unidirectional rays\n");
	//Write_Matrix(&ray, mat.nbcolumns-1);
	
	
	
	getIncidenceMatrix(dimension,ray,inequality,&incmat);
	//RationalMatrix* rt=new RationalMatrix(0,dimension,2,true,ray);	
	std::vector<int> indv(3);indv[0]=0;indv[1]=1;indv[2]=4;
	//RationalMatrix* rt1=rt->getRows(indv).getInverseByGauss();
	/*
	RationalMatrix* rt1=&rt->getColumns(indv);
	RationalMatrix* rtr=rt1->getInverseByGauss();
	RationalMatrix *rt2=&((*rt1)*(*rtr));
	Write_MatrixToFile(fileout,&incmat,incmat.nbcolumns);
*/
	
		
	Free_Matrix(&bidray); 
    
    Free_Matrix(&ray);
	
	Free_Vector(&inequality);
	
	
	
	fileout.close();
	system("PAUSE");
//	delete rt;
//	delete rt1;
	return 0;
	/* main */

    

   

//scanf(0);
}

