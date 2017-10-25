#pragma once 

namespace OPS
{
namespace LatticeGraph
{
namespace CountIntPoints
{

const int TOP = 2147483647;

typedef struct vector 
{
int size;
int *p; 
} vector;

typedef struct matrix 
{
int nbrows;
int nbcolumns;
int **p;
int *p_init; 
} matrix;

}//end of namespace
}//end of namespace
}//end of namespace
