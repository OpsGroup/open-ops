#pragma once


#include <stdio.h>
#include <math.h>
#include "StructuresVerge.h"
#include <vector>
#include <valarray>


namespace OPS
{
namespace LatticeGraph
{
namespace CountIntPoints
{

void EXCH(matrix& mat, int i1, int i2, int* aux);

char * My_Alloc(int nbelement,int element_size);
/* My.Alloc */


void Alloc_Vector(vector *vec, int size); 
/* Alloc.Vector */
	

void Alloc_Matrix(matrix *mat,int nbrows,int nbcolumns);
 /* Alloc.Matrix */



void Free_Vector(vector *vec); 
 /* Free.Vector */



void Free_Matrix(matrix *mat); 
 /* Free_Matrix */


void Read_Matrix(matrix *mat);
/* Read.Matrix */


void Write_Matrix( matrix *mat,int dim);
 /* Write.Matrix */



void Gcd(int a,int b,int *g);
/* Gcd */



/* finds the minimum (absolute value) of vector vec */ 
void Vector_Min_Not_0(vector *vec,int *min,int *index);


/* computes the gcd of all components of vec */ 
void Vector_Gcd(int *vec,int size,int *g);

/* normalize a vector in such a way that gcd(vec) = l */ 
void Vector_Normalize(int *vec,int size,int dimension);


/* computes r3 as combination of rl and r2 in such a way that r3[k]=0 */ 
void combine(int *rl,int *r2,int *r3,int k,int nbcolumns,int dim);


int chernikova(matrix *bid,matrix * uni,int nbbidray,int nbray,vector *ineq);

void getIncidenceMatrix(int dimension,const matrix & _rays,const vector& _inequality,matrix* _incidenceMat);

void partritionEqualAndInequal(matrix * mat);


}//end of namespace
}//end of namespace
}//end of namespace
