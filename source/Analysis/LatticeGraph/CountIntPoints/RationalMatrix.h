#pragma once
#include "FunctionsVerge.h"
//#include "../../../../include/Analysis/LatticeGraph/RationalNumber.h"
#include "Analysis/LatticeGraph/RationalNumber.h"
#include <valarray>
#include<vector>


//typedef LatticeGraph::RationalNumber RationalNumber;

namespace OPS
{

	namespace LatticeGraph
	{

typedef	std::valarray<RationalNumber> ValRatNum;
		namespace CountIntPoints
		{

			template<class T>
			struct Slice_iter
			{
				std::valarray<T> * mat;
				std::slice curSlice;
				int cur;

				T& ref(int i)const
				{return (*mat)[curSlice.start()+i*curSlice.stride()];}

				Slice_iter(std::valarray<T>* mat_in,std::slice slice_in):mat(mat_in),curSlice(slice_in),cur(0){}
				Slice_iter end()
				{
					Slice_iter t=*this;
					t.cur=curSlice.size();
					return t;
				}

				Slice_iter& inc(){cur++; return *this;}
				Slice_iter& dec(){cur--; return *this;}
				T& at(int i){return ref(cur=i);}


			};

			template <class T>
			bool isEqual(const Slice_iter<T>& it1,const Slice_iter<T>& it2)
			{
				return it1.cur==it2.cur && it1.curSlice.stride()=it2.curSlice.stride() && it1.curSlice.start()==it2.curSlice.start();
			}

			template <class T>
			bool firstLessSecond(const Slice_iter<T>& it1,const Slice_iter<T>& it2)
			{
				return it1.cur<it2.cur && it1.curSlice.stride()=it2.curSlice.stride() && it1.curSlice.start()==it2.curSlice.start();
			}


			//двухмерная матрица на основе valarray
			template<class T>
			class matrixVal
			{
				typedef std::valarray<T> Matr;
				typedef Slice_iter<T> SlIter;
				int m_rows;
				int m_columns;
				Matr m_mat;
			public:

				matrixVal(int rows,int columns):m_rows(rows),m_columns(columns),m_mat(rows*columns)
				{}

				matrixVal(const matrixVal<T> & mat)
				{
					resize(mat.m_rows,mat.m_columns);
					m_mat=mat.m_mat;
				}

				matrixVal(Matr & mat,int rows,int columns)
				{
					resize(rows,columns);
					m_mat=mat;
				
				}

				void resize(int rows,int col)
				{
				    m_rows=rows;
					m_columns=col;
					m_mat.resize(m_rows*m_columns);
				}

				SlIter  row(int i)
				{
					return SlIter(&m_mat,std::slice(i,m_columns,m_rows));
				}


				SlIter column(int j)
				{
					return SlIter(&m_mat,std::slice(j*m_columns,m_rows,1));
				}

				matrixVal<T>& getSubmatrix(const std::vector<int>& rowsInd,int colBeg,int colEnd)
				{

					int rows=rowsInd.size();
					int columns=colEnd-colBeg;
					matrixVal<T>* mat=new matrixVal<T>(rows*columns);


					for(int i=0;i<rows;i++)
						for(int j=0;j<columns;j++)
						{
							mat->m_mat[i*columns+j]=m_mat[rowsInd[i]*columns+j+colBeg];

						}

						return *mat;

				}

				matrix & addIdentityAndTranspose()
				{
					int dimension=m_columns;
					int columns2=m_rows+m_columns;
					matrix* bidray=new matrix;
					int i,j;

					Alloc_Matrix(bidray,dimension,columns2);
					
					for (i=0;i<dimension;i++)
					{
						for (j=0;j<dimension;j++)
							bidray->p[i][j]=0;
						bidray->p[i][i]=1;
					}

					for (i=0;i<m_rows;i++)
						for (j=1;j<m_columns;j++)
							bidray->p[j-1][i+dimension]=m_mat[i*columns2+j];
                   
					return *bidray;

				}


			};




			typedef Slice_iter<RationalNumber> Sl_ItRutNum;


			//2-х мерная квадратная рац матрица
			class RationalMatrix
			{



				ValRatNum m_mat;
				size_t m_rows;
				size_t m_columns;


				// void addIdentityMatrix();
				int findNonZeroInColumn(size_t pos, size_t _ncolumn);
				void swapRows(size_t iRow,size_t jRow);
				void divRowsOnRatNumber(size_t rows,size_t posInRow,RationalNumber& r);
				void subRowsMulByRatNumber(int row1,int row2,int posInRow,RationalNumber& r);


			public:

				RationalMatrix(int _m_rows=0,int _m_columns=0):m_mat(_m_rows*_m_columns),m_rows(_m_rows),m_columns(_m_columns)
				{}

				RationalMatrix(const RationalMatrix& r);
				RationalMatrix(int indBeg,int size,int indEnd,bool lastCol, const matrix& unidirRows);//проекция из Матрицы uniRows из Verge matrix

				void resize(int new_m_rows,int new_m_columns);//значения теряются!


				RationalMatrix* getInverseByGauss();

				RationalMatrix & getRows(const std::vector<int>& rowsInd);
				RationalMatrix & getColumns(const std::vector<int>& columInd); 

				friend RationalMatrix & operator *(const RationalMatrix& intMat,const RationalMatrix& rtMat);

				Sl_ItRutNum  row(size_t i);
				Sl_ItRutNum  column(size_t j);

				static RationalMatrix* getIdentityMatrix(int _nrows); 
				static matrix* addIdentityAndTransposeToMatrix(const RationalMatrix& rtmat);//для алгоритма Черниковой ,считается что матрица целая


			};














		}//end of mamespace
	}//end of mamespace
}//end of mamespace
