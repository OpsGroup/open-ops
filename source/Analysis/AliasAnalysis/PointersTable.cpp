#include "Analysis/AliasAnalysis/PointersTable.h"

namespace OPS
{
namespace AliasAnalysis
{

void EntriesPointersTable::SetSAMC(ReferenceExpression* referenceExpression, SetAbstractMemoryCell samc) 
{ 
	m_entriesPointerMap[referenceExpression]=samc;
};

SetAbstractMemoryCell EntriesPointersTable::GetSAMC(ReferenceExpression* referenceExpression) 
{ 
	return m_entriesPointerMap[referenceExpression]; 
};

void EntriesPointersTable::UnionSAMC(ReferenceExpression* referenceExpression, SetAbstractMemoryCell samc2) 
{
	SetAbstractMemoryCell samc_src = m_entriesPointerMap[referenceExpression];
	samc_src.insert(samc_src.end(), samc2.begin(), samc2.end());
	m_entriesPointerMap[referenceExpression] = samc_src;
};

void EntriesPointersTable::Clear() 
{ 
	m_entriesPointerMap.clear();
};

} // end namespace AliasAnalysis
} // end namespace OPS
