#include "Analysis/AliasAnalysis/SetAbstractMemoryCell.h"

namespace OPS
{
namespace AliasAnalysis
{

//Проверка на псевдоним
AliasResult SAMCCompare::IsNamesAlias(SetAbstractMemoryCell& samcFirst, SetAbstractMemoryCell& samcSecond)
{
	AliasResult isAlias = MAY_ALIAS;
	if (samcFirst.size()!=0 && samcSecond.size()!=0)
	{		
		if (this->IsFullMatch(samcFirst, samcSecond)) isAlias = MUST_ALIAS;
		else if (this->IsNotFullMatch(samcFirst, samcSecond)) isAlias = NO_ALIAS;
	}
	else isAlias = NO_ALIAS;
	return isAlias;	
}

//Проверка на полное совпадение
bool SAMCCompare::IsFullMatch(SetAbstractMemoryCell& samcFirst, SetAbstractMemoryCell& samcSecond)
{
	size_t samcFirstSize = samcFirst.size(); 
	size_t samcSecondSize = samcSecond.size();
	for (size_t i=0; i<samcFirstSize; i++)
		for (size_t j=0; j<samcSecondSize; j++)
			if ((samcFirst[i].GetName()!=samcSecond[j].GetName()) ||
				(samcFirst[i].GetOffset()!=samcSecond[j].GetOffset()))
				return false;
	return true;		
}

//Проверка на полное не совпадение
bool SAMCCompare::IsNotFullMatch(SetAbstractMemoryCell& samcFirst, SetAbstractMemoryCell& samcSecond)
{
	size_t samcFirstSize = samcFirst.size(); 
	size_t samcSecondSize = samcSecond.size();
	for (size_t i=0; i<samcFirstSize; i++)
		for (size_t j=0; j<samcSecondSize; j++)
			if ((samcFirst[i].GetName()==samcSecond[j].GetName()) &&
				(samcFirst[i].GetOffset()==samcSecond[j].GetOffset()))
				return false;
	return true;		
}

} // end namespace AliasAnalysis
} // end namespace OPS
