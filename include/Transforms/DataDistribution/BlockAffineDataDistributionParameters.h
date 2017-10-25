#ifndef BLOCK_AFFINE_DATA_DISTRIBUTION_PARAMETERS_H
#define BLOCK_AFFINE_DATA_DISTRIBUTION_PARAMETERS_H

#include <list>
#include <vector>
#include <string>

/*!
    Block-Affine Data Distribution.

--- Abstract:
    
    Parameters:
        1)	P is a number of processors
        2)	A[0..dim(0), 0..dim(1), 0..dim(2), ..., 0..dim(N-1)] is a multi-dimensional array
            Where	a) N is a count of dimensions.
                    b) dim(k) is a size of k-dimension.
        3)	c(0), c(1), c(2), ..., c(N), d(0), d(1), d(2), ..., d(N-1) - 2*N+1 integer parameters,
        which specify block-affine distribution of array A by P processor units:
        4)  ro(0), ro(1), ..., ro(N-1), lo(0), lo(1), ..., lo(N-1) - 2*N integer parameters, 
        which specify overlapping between blocks of neightboing processes (ro - right overlapping, lo - left overlapping)

        Array element with N-dimensional index (i(0), i(1), i(2), ..., i(N-1)) will be local for
        processor with number:
            p = ( ]i(0)/d(0)[*c(0) + ]i(1)/d(1)[*c(1) + ... + ]i(N-1)/d(N-1)[*c(N-1) + c(N) ) mod P
            
    Packaging of distributed arrays:
        After distribution of the multydimentional array A it is nesessary to transform all occurences of A.
        It is suitable to change the occurences A[i(0), i(1), i(2), ..., i(N-1)] 
        by occurence of onedimentional array AA[LocIndex(i(0), i(1), i(2), ..., i(N-1))].

        1) Length of A: ELEMENTS_IN_BLOCK * PROCESSOR_BLOCKS_IN_CELL * CELL_COUNT
        2) Transformation of indexes:
            LocIndex(i(0), i(1), i(2), ..., i(N-1)) = 
                OrdinalNumberOfCell(i(0), i(1), i(2), ..., i(N-1)) * PROCESSOR_BLOCKS_IN_CELL * ELEMENTS_IN_BLOCK +
                OrdinalNumberOfBlockInCell(i(0), i(1), i(2), ..., i(N-1)) * ELEMENTS_IN_BLOCK + 
                OrdinalNumberOfElementInBlock(i(0), i(1), i(2), ..., i(N-1))

        where:
            ELEMENTS_IN_BLOCK = d(0) * d(1) * ... * d(N-1)
            PROCESSOR_BLOCKS_IN_CELL = P/NOD(P, c[LEADING_DIMENTION])
            CELL_COUNT = dim[0]*NOD(P, c[0])/(d[0]*P) * dim[1]*NOD(P, c[1])/(d[1]*P) * ... * dim[N-1]*NOD(P, c[N-1])/(d[N-1]*P)
            LEADING_DIMENTION - parameter

            OrdinalNumberOfCell(i(0), i(1), i(2), ..., i(N-1)) = 
                i(N-1)/(CellWidthInBlocks(N-1) * d[N-1]) + 
                i(N-2)/(CellWidthInBlocks(N-2) * d[N-2]) * GetWidthInCells(N-1) +
                i(N-3)/(CellWidthInBlocks(N-3) * d[N-3]) * GetWidthInCells(N-2) * GetWidthInCells(N-1) +
                ... +
                i(0)/(CellWidthInBlocks(0) * d[0]) * GetWidthInCells(1) * ... * GetWidthInCells(N-1)

            GetWidthInCells(dimentions) = dim[dimention]/(CellWidthInBlocks(dimention) * d[dimention])
            
            OrdinalNumberOfBlockInCell(i(0), i(1), i(2), ..., i(N-1)) = (i(LEADING_DIMENTION) % (i(N-1)/(CellWidthInBlocks(LEADING_DIMENTION) * d[LEADING_DIMENTION]))) / d[LEADING_DIMENTION]

            OrdinalNumberOfElementInBlock(i(0), i(1), i(2), ..., i(N-1)) = 
                i(N-1)%d[N-1] +
                i(N-2)%d[N-2] * d[N-1] +
                i(N-3)%d[N-3] * d[N-2] * d[N-1] +
                ... +
                i(0)%d[0] * d[1] * d[2] * ... * d[N-1]

---- Technical details:		
    
---- Requirements:		
    
*/

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{
    class BADDParameters
    {
    public:
        typedef int ParameterValue;
        typedef std::vector<int> ParametersContainer;
        typedef std::vector<ParameterValue> MultyIndex;
        typedef std::list<MultyIndex> MultyIndexList;

        int P;
        ParametersContainer d;
        ParametersContainer a;
        ParametersContainer s;
        ParametersContainer dims;

    public:
        // Параметры валидны:
        // 1) P > 0
        // 2) Все d[i] > 0
        // 3) Все dim[i] > 0
        // 4) Размерности всех параметров совпадают
        bool isValid();

        // Параметы в "нормальной" форме. Это валидные параметры с определенными требованиями для корректной работы некоторых формул
        // 1) Параметры валидны
        // 2) Все d[i] <= dim[i]
        // 3) Все 0 < s[i] <= P
        // 4) d[i] == dim[i] <=> s[i] == P
        bool isNormalized();

        // Процедура нормализации
        // 1) Если d[i] > dim[i] => d[i] = dim[i]. Не влияет на размещение
        // 2) s[i] = s[i] + k*p, чтобы 0 < s[i] <= P. Не влияет на размещение, т.к. в формуле есть mod p
        // 3) d[i] == dim[i] <=> s[i] == P. Оба этих условия по отдельности гарантируют, что i-я координата не влияет на размещение
        //    чтобы в этом случае применять один набор формул, нужно синхронизировать эти 2 условия
        void normalize();

        MultyIndexList getBlocksMultyIndecesInCell();

        MultyIndexList getBlocksMultyIndecesOfProcessInCell(const MultyIndexList& allMultyIndeces, ParameterValue processorNumber);

        ParameterValue getProcessNumberByBlockIndex(const MultyIndex& multyIndex);

        ParameterValue getElementsInBlockCount();

        ParameterValue getWidthInCells(int dimention);

        ParameterValue getCellWidthInBlocks(int dimention);

        ParameterValue getCellsCount();

        ParameterValue getElementsInEachProcessorCount(int leadingDimention);

        ParameterValue getBlocksInEachProcessorCount(int leadingDimention);

        ParameterValue getBlocksCountInCell();

        ParametersContainer::size_type getLeadingDimentionWithLessMemoryDemand();

        static bool Parse(std::string str, BADDParameters& outputParameters);

    private:
        static ParameterValue getGreatestCommonDivisor(ParameterValue m, ParameterValue n);
    };

    class BADDParametersFamily
    {
    public:
        typedef int ParameterValue;
        typedef std::vector<ParameterValue> ParametersContainer;
        typedef std::vector<ParameterValue> MultyIndex;
        typedef std::list<MultyIndex> MultyIndexList;
        typedef std::vector<BADDParameters> BADDParametersContainer;

        ParameterValue P;
        ParametersContainer d;
        ParametersContainer s;
        ParametersContainer dims;
        ParametersContainer leftOverlapping;
        ParametersContainer rightOverlapping;

    public:
        BADDParametersFamily() {}
        BADDParametersFamily(BADDParametersContainer& parameters);
        
        static bool areBelongToOneFamily(BADDParameters& parameter1, BADDParameters& parameter2);
        
        static bool areBelongToOneFamily(BADDParametersContainer& parameters);

        MultyIndexList getBlocksMultyIndecesInCell();

        MultyIndexList getBlocksMultyIndecesOfProcessInCell(const MultyIndexList& allMultyIndeces, ParameterValue processorNumber);

        ParameterValue getProcessNumberByBlockIndex(const MultyIndex& multyIndex);

        ParameterValue getElementsInBlockCount();

        ParameterValue getWidthInCells(int dimention);

        ParameterValue getCellWidthInBlocks(int dimention);

        ParameterValue getCellsCount();

        ParameterValue getElementsInEachProcessorCount(int leadingDimention);

        ParameterValue getBlocksInEachProcessorCount(int leadingDimention);

        ParameterValue getBlocksCountInCell();

        std::string toString();
        static bool Parse(std::string str, BADDParametersFamily& outputParameters);

    private:
        static ParameterValue getGreatestCommonDivisor(ParameterValue m, ParameterValue n);

        void makeShiftContribution(ParametersContainer& shiftParameters);
    };
}
}
}


#endif // BLOCK_AFFINE_DATA_DISTRIBUTION_PARAMETERS_H
