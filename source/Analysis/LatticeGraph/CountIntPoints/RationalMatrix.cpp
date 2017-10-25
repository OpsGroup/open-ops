#include"RationalMatrix.h"
#include "OPS_Core/Exceptions.h"
namespace OPS
{

	namespace LatticeGraph
	{


		namespace CountIntPoints
		{



			int RationalMatrix::findNonZeroInColumn(size_t pos, size_t _ncolumn)
			{
				size_t k=pos;
				while(k<m_rows && column(_ncolumn).at(k).getNum()==0) k++;
				return k;
			}

			void RationalMatrix::swapRows(size_t iRow, size_t jRow)
			{
				//???
			}

			void RationalMatrix::divRowsOnRatNumber(size_t rows, size_t posInRow, RationalNumber& r)
			{
				size_t m=rows*m_columns;
				for(size_t i=posInRow;i<m_columns;i++)
					m_mat[m+i]=m_mat[m+i]/r;
			}

			void RationalMatrix::subRowsMulByRatNumber(int row1,int row2,int posInRow,RationalNumber& r)
			{
				int m1=row1*m_columns;
				int m2=row2*m_columns;
				RationalNumber rr;
				for(size_t i=posInRow;i<m_columns;i++)
				{
					m_mat[m1+i]=m_mat[m1+i]-(m_mat[m2+i]*r);
				}

			}


			RationalMatrix::RationalMatrix(const RationalMatrix& r)
			{
				//if ((r.m_columns!=m_columns)||(r.m_rows!=m_rows)) throw OPS::ArgumentError("dimensions in RationalMatrix not equal");
				m_columns=r.m_columns;
				m_rows=r.m_rows;
				m_mat.resize(m_columns*m_rows);
				m_mat=r.m_mat;

			}

			RationalMatrix::RationalMatrix(int indBeg,int size,int indEnd,bool lastCol, const matrix& unidirRows)//проекция из Матрицы uniRows из Verge matrix
			{
				int l=0;
				if (lastCol==true) l=1;
				int columns=unidirRows.nbrows;
				int rows=indEnd-indBeg+l;
				m_columns=columns;
				m_rows=rows;
				m_mat.resize(rows*columns);

				for(int i=0;i<columns;i++)
					for(int j=0;j<rows;j++)
					{
						m_mat[j*columns+i].setNum(unidirRows.p[i][j+indBeg]);

					}


					if (lastCol==true)
					{	
						for(int i=0 ;i<rows;i++) m_mat[columns*(rows-1)+i].setNum(unidirRows.p[i][size-1]);

					}


			}

			void RationalMatrix::resize(int new_m_rows,int new_m_columns)//значения теряются!
			{
				m_columns=new_m_columns;
				m_rows=new_m_rows;
				m_mat.resize(m_rows*m_columns);
			}

			RationalMatrix* RationalMatrix::getIdentityMatrix(int _nrows)
			{
				RationalMatrix* rt=new RationalMatrix(_nrows,_nrows);
				for(int i=0;i<_nrows;i++)
					rt->m_mat[i*_nrows+i].setNum(1);
				return rt;
			}

			RationalMatrix* RationalMatrix::getInverseByGauss()
			{
				if (m_rows!=m_columns) OPS::ArgumentError("невозможно обратить прямоугольную матрицу");
				RationalMatrix* idmat=getIdentityMatrix(m_rows);
				RationalMatrix* mat=new RationalMatrix(*this);
				RationalNumber rb;  
				int nonzero,index,buf,ind1,ind2;
				std::valarray<int> exchvec(m_rows);
				for(size_t i=0;i<m_rows;i++) exchvec[i]=i;


				for(size_t i=0;i<m_rows;i++)
				{
					nonzero=mat->findNonZeroInColumn(i,i);	

					if (nonzero==int(m_rows)) {delete idmat; delete mat; return 0;}
					buf=exchvec[i];exchvec[i]=exchvec[nonzero];exchvec[nonzero]=buf;

					index=exchvec[i];
					rb=mat->m_mat[index*m_rows+index];
					mat->divRowsOnRatNumber(index,i,rb);
					idmat->divRowsOnRatNumber(index,0,rb);

					for(size_t j=i+1;j<m_rows;j++)
					{
						ind1=exchvec[j];ind2=exchvec[i];
						rb=mat->m_mat[j*m_rows+i];
						mat->subRowsMulByRatNumber(ind1,ind2,i,rb);
						idmat->subRowsMulByRatNumber(ind1,ind2,0,rb);
					}

				}

				if (mat->m_mat[m_rows*m_rows-1].getNum()==0) {delete idmat; delete mat; return 0;}


				//obratnii hod
				for (int i=m_rows-1;i>0;i--)
				{
					index=exchvec[i];
					rb=mat->m_mat[index*m_rows+index];
					idmat->divRowsOnRatNumber(index,0,rb);
					//idmat->m_mat[index*m_rows+index].setNum(1);idmat->m_mat[index*m_rows+index].setDenom(1);
					for (int j=i-1;j>=0;j--)
					{
						ind1=exchvec[j];ind2=exchvec[i];
						rb=mat->m_mat[ind1*m_rows+ind2];
						idmat->subRowsMulByRatNumber(ind1,ind2,0,rb);
					}

				}
				idmat->divRowsOnRatNumber(exchvec[0],0,(mat->m_mat[0]));

				//prisvaivanie
				for(size_t i=0;i<m_rows;i++)
				{
					for(size_t j=0;j<m_rows;j++)
					{
						index=i*m_rows+j;
						ind1=exchvec[i]*m_rows+j;
						mat->m_mat[index]=idmat->m_mat[ind1];
						//mat->m_mat[index].setDenom(idmat->m_mat[ind1].getDenom());
					}
				}		
				delete idmat;

				return mat;
			}

			RationalMatrix & RationalMatrix::getRows(const std::vector<int>& rowsInd)
			{
				int rows=rowsInd.size();
				RationalMatrix* rt=new RationalMatrix(rows,m_columns);
				size_t a[]={1,m_columns};
				size_t b[]={m_columns,1};
				std::valarray<size_t> v1(a,2);
				std::valarray<size_t> v2(b,2);
				std::valarray<RationalNumber> v3(m_columns+1);
				std::valarray<RationalNumber> v4(rows*m_columns);
				//RationalNumber *p1=&v4[0];
				for(int i=0;i<rows;i++)
				{
					v3=m_mat[std::gslice(rowsInd[i]*m_columns,v1,v2)];
					std::copy(&v3[0],&v3[m_columns],&v4[i*m_columns]);    
				}
				rt->m_mat=v4;

				return *rt;



			}

			RationalMatrix & RationalMatrix::getColumns(const std::vector<int>& columInd)
			{
				int col=columInd.size();
				RationalMatrix* rt=new RationalMatrix(m_rows,col);
				size_t a[]={m_rows,1};
				size_t b[]={m_columns,1};
				std::valarray<size_t> v1(a,2);
				std::valarray<size_t> v2(b,2);
				std::valarray<RationalNumber> v3(m_rows);
				std::valarray<RationalNumber> v4(col*m_rows);
				//RationalNumber *p1=&v4[0];
				for(size_t i=0;i<m_rows;i++)
				{
					//v3=m_mat[std::gslice(columInd[i],v1,v2)];
					//std::copy(&v3[0],&v3[m_columns],&v4[i*m_columns]);
					//for(int k=0;k<m_columns;k++)v4[k*col+i]=v3[k];
					for(int k=0;k<col;k++)
						v4[i*col+k]=m_mat[i*m_columns+columInd[k]];
				}
				rt->m_mat=v4;

				return *rt;
			}

			RationalMatrix & operator *(const RationalMatrix& intMat,const RationalMatrix& rtMat)
			{

				int arows=intMat.m_rows;
				int acol=intMat.m_columns;
				int brows=rtMat.m_rows;
				int bcol=rtMat.m_columns;
				if (acol!=brows) throw ArgumentError("перемножение матриц невозможно ,размерности не совпадают");
				RationalNumber buf;
				int ind,ind1;
				RationalMatrix* rtm=new RationalMatrix(arows,bcol);

				for(int i=0;i<arows;i++)
					for(int j=0;j<bcol;j++)
					{   
						ind=i*bcol+j;
						ind1=i*acol;
						buf.setZero();
						for(int k=0;k<brows;k++)
							buf=buf+intMat.m_mat[ind1+k]*rtMat.m_mat[k*bcol+j];

						rtm->m_mat[ind]=buf;
					}

					return *rtm;
			}

			Sl_ItRutNum  RationalMatrix::row(size_t i)
			{
				return Sl_ItRutNum(&m_mat,std::slice(i,m_columns,m_rows));
			}

			Sl_ItRutNum   RationalMatrix::column(size_t j)
			{
				return Sl_ItRutNum(&m_mat,std::slice(j*m_columns,m_rows,1));
			}


			matrix* RationalMatrix::addIdentityAndTransposeToMatrix(const RationalMatrix& rtmat)
			{
				matrix* bidray=new matrix;
				/*int rows=rtmat.m_rows;
				int columns=rtmat.m_columns;

				Alloc_Matrix(bidray,,);
				for (i=0;i<dimension;i++)
				{
					for (j=0;j<dimension;j++)
						bidray.p[i][j]=0;
					bidray.p[i][i]=1;
				}

				for (i=0;i<nbconstraints;i++)
					for (j=1;j<nbcolumns;j++)
						bidray.p[j-1][i+dimension]=mat.p[i][j] ;

                   */
				return bidray;
			}


		}//end of namespaces
	}//-//-
}//-//-
