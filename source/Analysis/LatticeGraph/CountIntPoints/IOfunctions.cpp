#include <fstream>
#include "IOfunctions.h"
#include "FunctionsVerge.h"

namespace OPS
{
namespace LatticeGraph
{
namespace CountIntPoints
{

void Read_MatrixFromFile(std::ifstream& fin,matrix *mat)
{
	int nbrows, nbcolumns, i, j ;
	//(void) scanf ("%d %d", &nbrows, &nbcolumns) ;
	fin>>nbrows>>nbcolumns;
	//fin.get(nbrows);fin.get(nbcolumns);
	Alloc_Matrix(mat, nbrows, nbcolumns); 
	for (i=0;i<nbrows;i++)
		for (j=0;j<nbcolumns;j++)
			fin>>mat->p[i][j];
			//(void) scanf ("%d", &(mat->p[i] [j] )); 

} /* Read.Matrix From File*/


	void Write_MatrixToFile(std::ofstream& fout, matrix *mat,int dim)
{
	int nbrows, i, j;
	//(void) printf("%d %d\n", nbrows=mat->nbrows, dim);
	nbrows=mat->nbrows;
	fout<<nbrows<<" "<<dim<<"\n";
	for (i=0;i<nbrows;i++)
	{ 
		for (j=0;j<dim;j++)
			fout<<mat->p[i][j]<<" ";
			//(void) printf ("%4d", mat->p[i] [j] );
		fout<<"\n";
	} 
} /* Write.Matrix */


}//end of namespace
}//end of namespace
}//end of namespace

