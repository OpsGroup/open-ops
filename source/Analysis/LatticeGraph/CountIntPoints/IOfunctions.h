#pragma once

#include "StructuresVerge.h"
#include <iostream>
#include <fstream>


namespace OPS
{
namespace LatticeGraph
{
namespace CountIntPoints
{

    void Read_MatrixFromFile(std::ifstream& fin,matrix *mat);

    void Write_MatrixToFile(std::ofstream& fout, matrix *mat,int dim);

}//end of namespace
}//end of namespace
}//end of namespace
