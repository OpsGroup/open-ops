#include "Transforms/DataDistribution/BlockAffineDataDistributionParameters.h"

#include "Reprise/Reprise.h"

#include <sstream>

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{
    bool BADDParameters::isValid()
    {
        if(P <= 0)
            return false;

        if(d.size() != dims.size() || d.size() + 1 != s.size())
            return false;

        for(ParametersContainer::size_type i = 0; i < dims.size(); ++i)
        {
            if(dims[i] <= 0 || d[i] <= 0)
                return false;
        }

        return true;
    }

    bool BADDParameters::isNormalized()
    {
        if(!isValid())
            return false;

        for(ParametersContainer::size_type i = 0; i < dims.size(); ++i)
        {
            if(s[i] <= 0 || s[i] > P)
                return false;

            if(d[i] > dims[i])
                return false;

            if(!(d[i] == dims[i] && s[i] == P || d[i] != dims[i] && s[i] != P))
                return false;
        }

        return true;
    }

    void BADDParameters::normalize()
    {
        OPS_ASSERT(isValid());

        for(ParametersContainer::size_type i = 0; i < dims.size(); ++i)
        {
            if(d[i] >= dims[i])
            {
                d[i] = dims[i];
                s[i] = P;
            }

            if(s[i] <= 0)
            {
                s[i] = (-s[i])%P == 0 ? P : s[i] + ((int)(-s[i]/P))*P + P;
            }
            OPS_ASSERT(s[i] > 0);

            if(s[i] > P)
            {
                s[i] = s[i]%P == 0 ? P : s[i] - ((int)(s[i]/P))*P;
            }
            OPS_ASSERT(s[i] > 0 && s[i] <= P)
            
            if(s[i] == P)
            {
                d[i] = dims[i];
            }
        }
    }

    BADDParameters::ParameterValue BADDParameters::getProcessNumberByBlockIndex(const BADDParameters::MultyIndex& multyIndex)
    {
        OPS_ASSERT(isNormalized());
        OPS_ASSERT(dims.size() == multyIndex.size());

        ParameterValue result = 0;

        for(MultyIndex::size_type i = 0; i < multyIndex.size(); ++i)
        {
            result = result + multyIndex[i] * s[i];
        }

        return result % P;
    }

    BADDParameters::MultyIndexList BADDParameters::getBlocksMultyIndecesInCell()
    {
        OPS_ASSERT(isNormalized());

        MultyIndexList result;

        ParameterValue blocksInCell = getBlocksCountInCell();
        for(ParameterValue i = 0; i < blocksInCell; ++i)
        {
            MultyIndex blockIndexInCell(dims.size());
            ParameterValue j = i;
            for(BADDParameters::ParametersContainer::size_type k = 0; k < dims.size(); ++k)
            {
                ParameterValue cellWidthInBlocks = getCellWidthInBlocks(dims.size() - k - 1);
                blockIndexInCell[dims.size() - k - 1] = j % cellWidthInBlocks;
                j = j / cellWidthInBlocks;
            }
            result.push_back(blockIndexInCell);
        }

        return result;
    }

    BADDParameters::MultyIndexList BADDParameters::getBlocksMultyIndecesOfProcessInCell(const BADDParameters::MultyIndexList& allMultyIndeces, BADDParameters::ParameterValue processNumber)
    {
        OPS_ASSERT(isNormalized());

        MultyIndexList result;

        for(MultyIndexList::const_iterator it = allMultyIndeces.begin(); it != allMultyIndeces.end(); ++it)
        {
            if(getProcessNumberByBlockIndex(*it) == processNumber)
            {
                result.push_back(*it);
            }
        }

        return result;
    }

    BADDParameters::ParameterValue BADDParameters::getElementsInBlockCount()
    {
        OPS_ASSERT(isNormalized());

        ParameterValue result = 1;

        for(BADDParameters::ParametersContainer::size_type i = 0; i < dims.size(); ++i)
        {
            result = result * d[i];
        }

        return result;
    }

    BADDParameters::ParameterValue BADDParameters::getWidthInCells(int dimention)
    {
        OPS_ASSERT(isNormalized());
        OPS_ASSERT((BADDParameters::ParametersContainer::size_type)dimention >= 0 && (BADDParameters::ParametersContainer::size_type)dimention < dims.size());

        return dims[dimention] / (d[dimention] * getCellWidthInBlocks(dimention));
    }

    BADDParameters::ParameterValue BADDParameters::getCellWidthInBlocks(int dimention)
    {
        OPS_ASSERT(isNormalized());
        OPS_ASSERT((BADDParameters::ParametersContainer::size_type)dimention >= 0 && (BADDParameters::ParametersContainer::size_type)dimention < dims.size());

        return P / getGreatestCommonDivisor(P, s[dimention]);
    }

    BADDParameters::ParameterValue BADDParameters::getCellsCount()
    {
        OPS_ASSERT(isNormalized());

        ParameterValue result = 1;

        for(BADDParameters::ParametersContainer::size_type i = 0; i < dims.size(); ++i)
        {
            result = result * getWidthInCells(i);
        }

        return result;
    }

    BADDParameters::ParameterValue BADDParameters::getElementsInEachProcessorCount(int leadingDimention)
    {
        OPS_ASSERT(isNormalized());
        OPS_ASSERT((BADDParameters::ParametersContainer::size_type)leadingDimention >= 0 && (BADDParameters::ParametersContainer::size_type)leadingDimention < dims.size());

        return getBlocksInEachProcessorCount(leadingDimention) * getElementsInBlockCount();
    }

    BADDParameters::ParameterValue BADDParameters::getBlocksCountInCell()
    {
        OPS_ASSERT(isNormalized());

        ParameterValue result = 1;
        for(BADDParameters::ParametersContainer::size_type i = 0; i < dims.size(); ++i)
        {
            result = result * getCellWidthInBlocks(i);
        }

        return result;
    }

    BADDParameters::ParameterValue BADDParameters::getGreatestCommonDivisor(BADDParameters::ParameterValue m, BADDParameters::ParameterValue n)
    {
        while(m != 0 && n != 0)
        {
            if(m >= n) m = m%n;
            else n = n%m;
        }
        return n + m; // Одно - ноль
    }

    bool BADDParameters::Parse( std::string str, BADDParameters& outputParameters )
    {
        if(str.length() == 0)
            return false;

        std::string tmpStr = str + ',';
        int semiIndex = tmpStr.find(',');
        if(semiIndex == -1)
            return false;

        int P = atoi(tmpStr.substr(0, semiIndex).c_str());
        tmpStr = tmpStr.substr(semiIndex + 1);

        outputParameters.P = P;
        
        semiIndex = tmpStr.find(',');
        if(semiIndex == -1)
            return false;

        int dimentionsCount = atoi(tmpStr.substr(0, semiIndex).c_str());
        tmpStr = tmpStr.substr(semiIndex + 1);

        outputParameters.a.resize(dimentionsCount);
        outputParameters.d.resize(dimentionsCount);
        outputParameters.dims.resize(dimentionsCount);
        outputParameters.s.resize(dimentionsCount + 1);

        // dims
        for (int i = 0; i < dimentionsCount; ++i)
        {
            semiIndex = tmpStr.find(',');
            if(semiIndex == -1)
                return false;

            outputParameters.dims[i] = atoi(tmpStr.substr(0, semiIndex).c_str());
            tmpStr = tmpStr.substr(semiIndex + 1);
        }

        // d
        for (int i = 0; i < dimentionsCount; ++i)
        {
            semiIndex = tmpStr.find(',');
            if(semiIndex == -1)
                return false;

            outputParameters.d[i] = atoi(tmpStr.substr(0, semiIndex).c_str());
            tmpStr = tmpStr.substr(semiIndex + 1);
        }
        // s
        for (int i = 0; i < dimentionsCount + 1; ++i)
        {
            semiIndex = tmpStr.find(',');
            if(semiIndex == -1)
                return false;

            outputParameters.s[i] = atoi(tmpStr.substr(0, semiIndex).c_str());
            tmpStr = tmpStr.substr(semiIndex + 1);
        }
        // a
        for (int i = 0; i < dimentionsCount; ++i)
        {
            semiIndex = tmpStr.find(',');
            if(semiIndex == -1)
                return false;

            outputParameters.a[i] = atoi(tmpStr.substr(0, semiIndex).c_str());
            tmpStr = tmpStr.substr(semiIndex + 1);
        }

        return true;
    }

    BADDParameters::ParametersContainer::size_type BADDParameters::getLeadingDimentionWithLessMemoryDemand()
    {
        OPS_ASSERT(isNormalized());

        ParametersContainer::size_type result = 0;
        ParameterValue minElementsInEachProcessorCount = getElementsInEachProcessorCount(0);
        
        for(ParametersContainer::size_type i = 1; i < dims.size(); ++i)
        {
            ParameterValue currentElementsInEachProcessorCount = getElementsInEachProcessorCount(i);
            if (currentElementsInEachProcessorCount < minElementsInEachProcessorCount)
            {
                result = i;
                minElementsInEachProcessorCount = currentElementsInEachProcessorCount;
            }
        }

        return result;
    }

    BADDParameters::ParameterValue BADDParameters::getBlocksInEachProcessorCount( int leadingDimention )
    {
        OPS_ASSERT(isNormalized());
        OPS_ASSERT((BADDParameters::ParametersContainer::size_type)leadingDimention >= 0 && (BADDParameters::ParametersContainer::size_type)leadingDimention < dims.size());

        if (dims.size() == 1)
        {
            return getCellsCount();
        }

        return getCellsCount() * getCellWidthInBlocks(leadingDimention);
    }

    bool BADDParametersFamily::areBelongToOneFamily( BADDParameters& parameter1, BADDParameters& parameter2 )
    {
        OPS_ASSERT(parameter1.isNormalized());
        OPS_ASSERT(parameter2.isNormalized());

        if(parameter1.P != parameter2.P)
            return false;

        if(parameter1.dims.size() != parameter2.dims.size())
            return false;

        for(BADDParametersContainer::size_type i = 0; i < parameter1.dims.size(); ++i)
        {
            if(!(parameter1.d[i] == parameter2.d[i] && parameter1.s[i] == parameter2.s[i] && parameter1.dims[i] == parameter2.dims[i]))
                return false;
        }

        if(parameter1.s[parameter1.dims.size()] != parameter2.s[parameter2.dims.size()])
            return false;

        return true;
    }

    bool BADDParametersFamily::areBelongToOneFamily( BADDParametersContainer& parameters )
    {
        OPS_ASSERT(parameters.size() > 0);
        OPS_ASSERT(parameters[0].isNormalized());

        for(BADDParametersContainer::size_type i = 0; i < parameters.size(); ++i)
        {
            OPS_ASSERT(parameters[i].isNormalized());

            if(!areBelongToOneFamily(parameters[0], parameters[i]))
                return false;
        }

        return true;
    }

    void BADDParametersFamily::makeShiftContribution( ParametersContainer& shiftParameters )
    {
		OPS_ASSERT(shiftParameters.size() == dims.size());
		OPS_ASSERT(leftOverlapping.size() == dims.size());
		OPS_ASSERT(rightOverlapping.size() == dims.size());

        for (BADDParametersContainer::size_type i = 0; i < dims.size(); ++i)
        {
            if(shiftParameters[i] > 0 && rightOverlapping[i] < shiftParameters[i])
            {	
                rightOverlapping[i] = shiftParameters[i];
            }
            else if(shiftParameters[i] < 0 && leftOverlapping[i] < -shiftParameters[i])
            {
                leftOverlapping[i] = -shiftParameters[i];
            }	
        }
    }

    BADDParametersFamily::BADDParametersFamily( BADDParametersContainer& parameters )
    {
        OPS_ASSERT(parameters.size() > 0);
        OPS_ASSERT(parameters[0].isNormalized());

        P = parameters[0].P;
        dims = parameters[0].dims;

        for(BADDParametersContainer::size_type i = 1; i < parameters.size(); ++i)
        {
            OPS_ASSERT(parameters[i].isNormalized());

            OPS_ASSERT(BADDParametersFamily::areBelongToOneFamily(parameters[0], parameters[i]));
        }
        
        d = parameters[0].d;
        s = parameters[0].s;
        
        leftOverlapping.resize(dims.size(), 0);
        rightOverlapping.resize(dims.size(), 0);

        makeShiftContribution(parameters[0].a);

        for(BADDParametersContainer::size_type i = 1; i < parameters.size(); ++i)
        {
            OPS_ASSERT(parameters[i].isNormalized());

            OPS_ASSERT(BADDParametersFamily::areBelongToOneFamily(parameters[0], parameters[i]));

            makeShiftContribution(parameters[i].a);
        }
    }

    BADDParametersFamily::ParameterValue BADDParametersFamily::getProcessNumberByBlockIndex(const BADDParametersFamily::MultyIndex& multyIndex)
    {
        OPS_ASSERT(dims.size() == multyIndex.size());

        ParameterValue result = 0;

        for(MultyIndex::size_type i = 0; i < multyIndex.size(); ++i)
        {
            result = result + multyIndex[i] * s[i];
        }

        return result % P;
    }

    BADDParametersFamily::MultyIndexList BADDParametersFamily::getBlocksMultyIndecesInCell()
    {
        MultyIndexList result;

        ParameterValue blocksInCell = getBlocksCountInCell();
        for(ParameterValue i = 0; i < blocksInCell; ++i)
        {
            MultyIndex blockIndexInCell(dims.size());
            ParameterValue j = i;
            for(ParametersContainer::size_type k = 0; k < dims.size(); ++k)
            {
                ParameterValue cellWidthInBlocks = getCellWidthInBlocks(dims.size() - k - 1);
                blockIndexInCell[dims.size() - k - 1] = j % cellWidthInBlocks;
                j = j / cellWidthInBlocks;
            }
            result.push_back(blockIndexInCell);
        }

        return result;
    }

    BADDParametersFamily::MultyIndexList BADDParametersFamily::getBlocksMultyIndecesOfProcessInCell(const BADDParametersFamily::MultyIndexList& allMultyIndeces, ParameterValue processNumber)
    {
        MultyIndexList result;

        for(MultyIndexList::const_iterator it = allMultyIndeces.begin(); it != allMultyIndeces.end(); ++it)
        {
            if(getProcessNumberByBlockIndex(*it) == processNumber)
            {
                result.push_back(*it);
            }
        }

        return result;
    }

    BADDParametersFamily::ParameterValue BADDParametersFamily::getElementsInBlockCount()
    {
        ParameterValue result = 1;

        for(BADDParametersFamily::ParametersContainer::size_type i = 0; i < dims.size(); ++i)
        {
            result = result * (d[i] + leftOverlapping[i] + rightOverlapping[i]);
        }

        return result;
    }

    BADDParametersFamily::ParameterValue BADDParametersFamily::getWidthInCells(int dimention)
    {
        OPS_ASSERT((BADDParametersFamily::ParametersContainer::size_type)dimention >= 0 && (BADDParametersFamily::ParametersContainer::size_type)dimention < dims.size());

        return dims[dimention] / (d[dimention] * getCellWidthInBlocks(dimention));
    }

    BADDParametersFamily::ParameterValue BADDParametersFamily::getCellWidthInBlocks(int dimention)
    {
        OPS_ASSERT((BADDParametersFamily::ParametersContainer::size_type)dimention >= 0 && (BADDParametersFamily::ParametersContainer::size_type)dimention < dims.size());

        return P / getGreatestCommonDivisor(P, s[dimention]);
    }

    BADDParametersFamily::ParameterValue BADDParametersFamily::getCellsCount()
    {
        ParameterValue result = 1;

        for(BADDParametersFamily::ParametersContainer::size_type i = 0; i < dims.size(); ++i)
        {
            result = result * getWidthInCells(i);
        }

        return result;
    }

    BADDParametersFamily::ParameterValue BADDParametersFamily::getElementsInEachProcessorCount(int leadingDimention)
    {
        OPS_ASSERT((BADDParametersFamily::ParametersContainer::size_type)leadingDimention >= 0 && (BADDParametersFamily::ParametersContainer::size_type)leadingDimention < dims.size());

        return getBlocksInEachProcessorCount(leadingDimention) * getElementsInBlockCount();
    }

    BADDParametersFamily::ParameterValue BADDParametersFamily::getBlocksCountInCell()
    {
        ParameterValue result = 1;
        for(BADDParametersFamily::ParametersContainer::size_type i = 0; i < dims.size(); ++i)
        {
            result = result * getCellWidthInBlocks(i);
        }

        return result;
    }

    BADDParametersFamily::ParameterValue BADDParametersFamily::getGreatestCommonDivisor(ParameterValue m, ParameterValue n)
    {
        while(m != 0 && n != 0)
        {
            if(m >= n) m = m%n;
            else n = n%m;
        }
        return n + m; // Одно - ноль
    }

    BADDParametersFamily::ParameterValue BADDParametersFamily::getBlocksInEachProcessorCount( int leadingDimention )
    {
        OPS_ASSERT((BADDParametersFamily::ParametersContainer::size_type)leadingDimention >= 0 && (BADDParametersFamily::ParametersContainer::size_type)leadingDimention < dims.size());

        if (dims.size() == 1)
        {
            return getCellsCount();
        }

        return getCellsCount() * getCellWidthInBlocks(leadingDimention);
    }

    std::string BADDParametersFamily::toString()
    {
        std::stringstream ss;
        ss<<P<<','<<dims.size()<<',';
		for(size_t i = 0; i < dims.size(); ++i)
        {
            ss<<dims[i]<<",";
        }
		for(size_t i = 0; i < d.size(); ++i)
        {
            ss<<d[i]<<",";
        }
		for(size_t i = 0; i < s.size(); ++i)
        {
            ss<<s[i]<<",";
        }
		for(size_t i = 0; i < leftOverlapping.size(); ++i)
        {
            ss<<leftOverlapping[i]<<",";
        }
		for(size_t i = 0; i < rightOverlapping.size() - 1; ++i)
        {
            ss<<rightOverlapping[i]<<",";
        }
        ss<<rightOverlapping[rightOverlapping.size() - 1];

        return ss.str();
    }

    bool BADDParametersFamily::Parse(std::string str, BADDParametersFamily& outputParameters)
    {
        if(str.length() == 0)
            return false;

        std::string tmpStr = str + ',';
        int semiIndex = tmpStr.find(',');
        if(semiIndex == -1)
            return false;

        int P = atoi(tmpStr.substr(0, semiIndex).c_str());
        tmpStr = tmpStr.substr(semiIndex + 1);

        outputParameters.P = P;

        semiIndex = tmpStr.find(',');
        if(semiIndex == -1)
            return false;

        int dimentionsCount = atoi(tmpStr.substr(0, semiIndex).c_str());
        tmpStr = tmpStr.substr(semiIndex + 1);

        outputParameters.leftOverlapping.resize(dimentionsCount);
        outputParameters.rightOverlapping.resize(dimentionsCount);
        outputParameters.d.resize(dimentionsCount);
        outputParameters.dims.resize(dimentionsCount);
        outputParameters.s.resize(dimentionsCount + 1);

        // dims
        for (int i = 0; i < dimentionsCount; ++i)
        {
            semiIndex = tmpStr.find(',');
            if(semiIndex == -1)
                return false;

            outputParameters.dims[i] = atoi(tmpStr.substr(0, semiIndex).c_str());
            tmpStr = tmpStr.substr(semiIndex + 1);
        }

        // d
        for (int i = 0; i < dimentionsCount; ++i)
        {
            semiIndex = tmpStr.find(',');
            if(semiIndex == -1)
                return false;

            outputParameters.d[i] = atoi(tmpStr.substr(0, semiIndex).c_str());
            tmpStr = tmpStr.substr(semiIndex + 1);
        }
        // s
        for (int i = 0; i < dimentionsCount + 1; ++i)
        {
            semiIndex = tmpStr.find(',');
            if(semiIndex == -1)
                return false;

            outputParameters.s[i] = atoi(tmpStr.substr(0, semiIndex).c_str());
            tmpStr = tmpStr.substr(semiIndex + 1);
        }
        // left
        for (int i = 0; i < dimentionsCount; ++i)
        {
            semiIndex = tmpStr.find(',');
            if(semiIndex == -1)
                return false;

            outputParameters.leftOverlapping[i] = atoi(tmpStr.substr(0, semiIndex).c_str());
            tmpStr = tmpStr.substr(semiIndex + 1);
        }
        // right
        for (int i = 0; i < dimentionsCount; ++i)
        {
            semiIndex = tmpStr.find(',');
            if(semiIndex == -1)
                return false;

            outputParameters.leftOverlapping[i] = atoi(tmpStr.substr(0, semiIndex).c_str());
            tmpStr = tmpStr.substr(semiIndex + 1);
        }

        return true;
    }
}
}
}
