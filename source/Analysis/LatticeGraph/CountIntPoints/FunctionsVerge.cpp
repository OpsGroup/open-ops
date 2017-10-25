#include "FunctionsVerge.h"
#include <malloc.h>
#include <stdio.h>
#include <cstdlib>

namespace OPS
{
namespace LatticeGraph
{
namespace CountIntPoints
{

void EXCH(matrix& mat, int i1, int i2) 
{
    int* aux;
    aux = mat.p[i1];
    mat.p[i1] = mat.p[i2];
    mat.p[i2] = aux;
}


char * My_Alloc(int nbelement, int element_size)
{
	char *p;
    p = new char[nbelement*element_size];
	return p; 

} /* My.Alloc */


void Alloc_Vector(vector *vec, int size) 
{
	vec->size=size;
	vec->p=(int *)My_Alloc(size, sizeof(int)); 

}/* Alloc.Vector */


void Alloc_Matrix(matrix *mat,int nbrows,int nbcolumns)
{
	int i, *p, **q;
	mat->nbrows=nbrows;
	mat->nbcolumns=nbcolumns;
	q=(int **)My_Alloc(nbrows, sizeof(int *));
	mat->p=q;
	p=(int *)My_Alloc(nbrows*nbcolumns, sizeof(int));
	mat->p_init=p;
	for (i=0;i<nbrows;i++) 
	{
		mat->p[i]=p;
		p=p+nbcolumns; 
	} 
} /* Alloc.Matrix */



void Free_Vector(vector *vec) 
{
	delete vec->p;
} /* Free.Vector */



void Free_Matrix(matrix *mat) 
{
	delete mat->p; delete mat->p_init; 

} /* Free_Matrix */


void Read_Matrix(matrix *mat)
{
	int nbrows, nbcolumns, i, j ;
	(void) scanf ("%d %d", &nbrows, &nbcolumns) ;
	Alloc_Matrix(mat, nbrows, nbcolumns); 
	for (i=0;i<nbrows;i++)
		for (j=0;j<nbcolumns;j++)
			(void) scanf ("%d", &(mat->p[i] [j] )); 

} /* Read.Matrix */


void Write_Matrix( matrix *mat,int dim)
{
	int nbrows, i, j;
	(void) printf("%d %d\n", nbrows=mat->nbrows, dim); 
	for (i=0;i<nbrows;i++)
	{ 
		for (j=0;j<dim;j++)
			(void) printf ("%4d", mat->p[i][j] ); (void) printf("\n"); 
	} 
} /* Write.Matrix */



void Gcd(int a,int b,int *g)
{
	int aux;
	if ((a==b) && (b==0)) *g=0;
	else 
	{
		a=abs(a);
		b=abs(b);
		while (a) { aux=b%a; b=a; a=aux; }
		*g=b; 
	}
}/* Gcd */



/* finds the minimum (absolute value) of vector vec */ 
void Vector_Min_Not_0(vector *vec,int *min,int *index)
{
	int m, size, i, ind, aux;
	size=vec->size; m=TOP; ind=-1;
	for (i=0;i<size;i++) 
		if (vec->p[i]!=0)
			if (m>(aux=abs(vec->p[i])))
			{
				ind=i; m=aux;
			}
			
			if (ind==-1)
				*min=1;
			else 
				*min=m;
			*index=ind; 
} /* Vector_Min_Not_0 */




/* computes the gcd of all components of vec */ 
void Vector_Gcd( int *vec,int size,int *g)
{
	vector vec_A;
	int min, index, *p, not_all_zero, i;
	Alloc_Vector(&vec_A, size);
	p=vec;
	for (i=0;i<size;i++)
		vec_A.p[i]=*p++;
	do 
	{
		Vector_Min_Not_0(&vec_A, &min, &index);
		if (min==1) break;
		not_all_zero=false;
		for (i=0;i<size;i++)
			if (i!=index)
				not_all_zero |= (vec_A.p[i]%=min) ;
	} while (not_all_zero);
	*g=min;
	Free_Vector(&vec_A); 
}/* Vector.Gcd */



/* normalize a vector in such a way that gcd(vec) = l */ 
void Vector_Normalize(int *vec,int size,int dimension)
{
	int gcd, i, *p;
	Vector_Gcd(vec, dimension, &gcd);
	if (gcd>=2) 
	{
		p=vec;
		for (i=0;i<size;i++)
			*p++ /= gcd; 
	}
}/* Vector_Normalize */


/* computes r3 as combination of rl and r2 in such a way that r3[k]=0 */ 
void combine(int *r1,int *r2,int *r3,int k,int nbcolumns,int dim)
{
	int aux, a1, a2, j;
	Gcd(r1[k], r2[k], &aux);
	a1=*(r1+k)/aux;
	a2=*(r2+k)/aux;
	for (j=0;j<nbcolumns;j++)
		r3[j]=a2*r1[j]-a1*r2[j]; 
	Vector_Normalize(r3, nbcolumns, dim); 
} /* combine */



int chernikova(matrix *bid,matrix * uni,int nbbidray,int nbray,vector *ineq)
//matrix *bid, *uni; /* matrices of bidirectional and unidirectional rays */ 
//int nbbidray, nbray; /* number of bidirectional and unidirectional rays */
//vector *ineq; /* vector of flags inequality/equation */
{
	vector commonzero, inequality;
	matrix ray, bidray;
	int nbcolumns, index_non_zero, nbcommonconstraints, max_rays;
	int i, j , k, l, m, *p, bound, inf_bound, equal_bound, sup_bound;//, redundant;
	bool redundant;
	int dimension; /* dimension of the space */
	bidray=*bid;
	ray=*uni;
	inequality=*ineq;
	nbcolumns=bidray.nbcolumns; /* width of both matrices uni and bid */
	max_rays=ray.nbrows;       /* size of matrix uni */
	dimension=nbbidray;
	Alloc_Vector(&commonzero, nbcolumns);
	for (k=dimension;k<nbcolumns;k++) 
	{


		/* finds if exists, a bidirectional ray which does not saturate the
		current constraint */ 
		index_non_zero=-1; 
		for (i=0;i<nbbidray;i++) 
		{
			if (bidray.p[i][k]!=0) 
			{
				index_non_zero=i;
				break; 
			}

		}
		if (index_non_zero!=-1) 
		{

			/* discards index_non_zero bidirectional ray */ 
			nbbidray--;
			if (nbbidray!=index_non_zero) 
			{
				p=bidray.p[nbbidray];
				bidray.p[nbbidray]=bidray.p[index_non_zero]; 
				bidray.p[index_non_zero]= p;
			}

			/* Computes the new lineality space */ 
			for (i=0;i<nbbidray;i++) 
				if (bidray.p[i][k]!=0)
					combine(bidray.p[i], bidray.p[nbbidray], bidray.p[i], k,nbcolumns, dimension); 

			/* add the positive part of index_non_zero bidirectional ray to the set of unidirectional rays */ 
			if (nbray==max_rays)
				return 1; 
			if (bidray.p[nbbidray][k]<0)
				for (j=0;j<nbcolumns;j++)
					ray.p[nbray][j]=-bidray.p[nbbidray][j]; 
			else
				for (j=0;j<nbcolumns;j++)
					ray.p[nbray][j]=bidray.p[nbbidray][j];

			/* Computes the new pointed cone */
			for (i=0;i<nbray; i++) 
				if (ray.p[i][k]!=0)
					combine(ray.p[i], ray.p[nbray], ray.p[i], k, nbcolumns, dimension); 
			if (inequality.p[k])
				nbray++; 
		} 
		else
		{
			/* 
			sort rays 
			: 0 <= i < equal_bound : saturates the constraint
			: equal_bound <= i < sup_bound : verifies the constraint 
			: sup_bound <= i < bound : does not verify 
			*/ 
			equal_bound=0;
			sup_bound=0;
			inf_bound=nbray; 
			while (inf_bound>sup_bound)
			{
				if (ray.p[sup_bound][k]==0)
				{
					EXCH(ray, equal_bound, sup_bound); 
					equal_bound++;
					sup_bound++; 
				}
				else
					if (ray.p[sup_bound][k]<0)
					{ 
						inf_bound--;
						EXCH(ray, inf_bound, sup_bound);
					}
					else 
						sup_bound++; 
			}


			/* Computes only the new pointed cone */ 

			bound=nbray;
			for (i=equal_bound;i<sup_bound;i++) 
				for(j=sup_bound;j<bound;j++) 
				{

					/* computes the set of common saturated constraints */ 
					nbcommonconstraints=0;
					for (l=dimension;l<k;l++)
					{
						if ((ray.p[i][l]==0) && (ray.p[j][l]==0))
						{ 
							commonzero.p[nbcommonconstraints]=l;///??? 1 or l
							nbcommonconstraints++; 
						}
					}
					if (nbcommonconstraints+nbbidray>=dimension-2)
					{

						/* check whether a ray m saturates the same set of constraints */
						redundant=false;
						for (m=0;m<bound;m++)
							if ((m!=i) && (m!=j)) 
							{
								for (l=0;l<nbcommonconstraints; l++)
									if (ray.p[m][commonzero.p[l]]!=0)
										break;
								if (l==nbcommonconstraints) 
								{

									/* the combination of ray i and j will generate a
									non extremal ray so ... */ 
									redundant=true;
									break;
								}
							}

							if (!redundant)
							{
								if (nbray==max_rays)
									return 1;

								/* computes the new ray */
								combine(ray.p[j], ray.p[i], ray.p[nbray], k, nbcolumns,dimension); 
								nbray++;

							}
					}
				}

				/* Eliminates all non extremal rays */ 
				if (inequality.p[k])
					j=sup_bound;
				else
					j=equal_bound; 
				i=nbray;
				while ((j<bound)&&(i>bound))
				{
					i--;
					EXCH(ray, i, j);
					j++;
				}
				if (j==bound) 
					nbray=i;
				else
					nbray=j;
		}
	}
	Free_Vector(&commonzero);
	bid->nbrows=nbbidray;
	uni->nbrows=nbray;
	return 0; 
	/* chernikova */
}






void getIncidenceMatrix(int dimension,const matrix & _rays,const vector& _inequality,matrix* _incidenceMat)
{
	//input from Chernikova algrorithm!!!
    
	int inequality=0;
	int raycol=_rays.nbcolumns;
	
		for(int i=dimension;i<_inequality.size;i++)
		if (_inequality.p[i]) inequality++;
	
	
	int incidcolumns=_rays.nbrows;
	int incidrows=inequality; 
     
	
	Alloc_Matrix(_incidenceMat,incidrows,incidcolumns);

	
	
	 int k=0;    
	for(int j=dimension;j<raycol;j++)
			if (_inequality.p[j])
			{
				for(int i=0;i<incidcolumns;i++)
				{

				if (_rays.p[i][j]==0)
					_incidenceMat->p[k][i]=1;
				else
                    _incidenceMat->p[k][i]=0;
				}
				k++;
			}
}




void partritionEqualAndInequal(matrix * mat)
{
   int i=0;
   int j=mat->nbrows-1;
   while(i<j)
   {
	   if ((mat->p[i][0]==1)&&(mat->p[j][0]==0))
	   {
		   EXCH((*mat),i,j);
		   i++;j--;
	   }
	   else
		   if (mat->p[i][0]==0) i++;
		   else j--;
   }
}




}//end of namespace
}//end of namespace
}//end of namespace


