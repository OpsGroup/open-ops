#include "Analysis/AliasAnalysis/MemoryManager.h"

namespace OPS
{
namespace AliasAnalysis
{

//Разбор пространства имен
void MemoryManager::Build()
{
	m_CurrentOffset = 0;
	m_varLocation.clear();		
	for (Declarations::VarIterator iter = m_TranslationUnit->getGlobals().getFirstVar(); iter.isValid(); ++iter)
	{
		ParamLocation paramLoc(m_CurrentOffset, 
							   GetSizeOf(iter->getType(), true), 
							   GetSizeOf(iter->getType(), false));
		m_varLocation[iter->getName()] = paramLoc;
		m_CurrentOffset += paramLoc.GetSize();
	}
}

//Установка текущего блока
void MemoryManager::SetBlock(BlockStatement* block) 
{ 
	m_block = block;
	m_CurrentOffset += 1000;
	for (Declarations::VarIterator iter = block->getDeclarations().getFirstVar(); iter.isValid(); ++iter)
	{
		ParamLocation paramLoc(m_CurrentOffset, 
							   GetSizeOf(iter->getType(), true), 
							   GetSizeOf(iter->getType(), false));
		m_varLocation[iter->getName()] = paramLoc;
		m_CurrentOffset += paramLoc.GetSize();
	}
}

bool MemoryManager::CheckType(ReferenceExpression* firstPointer, ReferenceExpression* secondPointer)
{
	TypeBase& typeBaseFirst = firstPointer->getReference().getType();
	TypeBase& typeBaseSecond = secondPointer->getReference().getType();
	if (typeBaseFirst.is_a<PtrType>() && typeBaseSecond.is_a<PtrType>())
	{
		PtrType& ptrTypeFirst = typeBaseFirst.cast_to<PtrType>();
		PtrType& ptrTypeSecond = typeBaseSecond.cast_to<PtrType>();
		if (ptrTypeFirst.getPointedType().is_a<BasicType>() && 
			ptrTypeSecond.getPointedType().is_a<BasicType>())
		{
			BasicType& basicTypeFirst = ptrTypeFirst.getPointedType().cast_to<BasicType>();
			BasicType& basicTypeSecond = ptrTypeSecond.getPointedType().cast_to<BasicType>();
			if (basicTypeFirst.getKind() == basicTypeSecond.getKind())
				return true;
			if ((basicTypeFirst.getKind() == OPS::Reprise::BasicType::BT_CHAR) ||
				(basicTypeSecond.getKind() == OPS::Reprise::BasicType::BT_CHAR))
				return true;
		}
	}
	return false;
}

std::string MemoryManager::GetFuncName(ReferenceExpression* refExpr)
{
	RepriseBase* pParent = refExpr->getParent();
	while (pParent && !(pParent->is_a<SubroutineDeclaration>()))
		pParent = pParent->getParent();
	if (pParent && pParent->is_a<SubroutineDeclaration>())
	{
		SubroutineDeclaration &subroutineDeclaration = pParent->cast_to<SubroutineDeclaration>();
		return subroutineDeclaration.getName();
	}
	return "";
}

SetAbstractMemoryCell MemoryManager::GetSAMCExprValue(SetAbstractMemoryCell samcSrc, long_long_t iOffset)
{
	SetAbstractMemoryCell samcTmp;
	samcTmp.clear();
	for (size_t i=0; i< samcSrc.size(); i++)
	{
		if (samcSrc[i].GetName() != "")
		{
			//смещение
			ParamLocation paramLocSrc = m_varLocation[samcSrc[i].GetName()];
			long_long_t globOffset = paramLocSrc.GetOffset();			
			globOffset += (samcSrc[i].GetOffset() * paramLocSrc.GetTypeSize()); //для массивов			
			globOffset += (iOffset * paramLocSrc.GetTypeSize()); //внешнее смещение
			//поиск
			ExprName newName = "";
			long_long_t newOffset = 0;
			VariablesLocation::iterator itCurr = m_varLocation.begin();
			for(; itCurr != m_varLocation.end(); ++itCurr)
			{
				if ((itCurr->second.GetOffset() <= globOffset) && (itCurr->second.GetOffset() + itCurr->second.GetSize() > globOffset))
				{
					newName = itCurr->first;
					break;
				}
			}
			if (newName != "")
			{
				ParamLocation paramLocDst = m_varLocation[newName];
				if ((paramLocDst.GetSize() != paramLocDst.GetTypeSize()) && (paramLocDst.GetTypeSize() != 0)) //массив
					newOffset = (globOffset - paramLocDst.GetOffset())/paramLocDst.GetTypeSize();
			}
			samcTmp.push_back(AbstractMemoryCell(newName,newOffset));
		}
	}
	return samcTmp;	
}

long_long_t MemoryManager::GetIntLiteralValue(LiteralExpression* literalExpression, char iAction)
{
	if (literalExpression->is_a<StrictLiteralExpression>())
	{
		StrictLiteralExpression* strictLiteralExpression = literalExpression->cast_ptr<StrictLiteralExpression>();
		if (strictLiteralExpression->isInteger())
			return strictLiteralExpression->getInt32() * iAction;
	}
	else if (literalExpression->is_a<BasicLiteralExpression>())
	{
		BasicLiteralExpression* basicLiteralExpression = literalExpression->cast_ptr<BasicLiteralExpression>();
		if (basicLiteralExpression->getLiteralType() == BasicLiteralExpression::LT_INTEGER)
			return basicLiteralExpression->getInteger() * iAction;
	}
	return 0;
}

long_long_t MemoryManager::GetArrayOffset(CallExpressionBase& basicCallExprArray)
{
	long_long_t iOffset = 0;
	if (basicCallExprArray.getArgument(0).is_a<ReferenceExpression>())
	{		
		ReferenceExpression& arrayData = basicCallExprArray.getArgument(0).cast_to<ReferenceExpression>();
	
		std::vector<int> aLim;
		OPS::Shared::getArrayLimits(&(arrayData.getReference().getType()), aLim);		
		
		int aLimSize = (int)aLim.size();		
		int dim = basicCallExprArray.getArgumentCount() - 1;
		if (aLimSize >= dim)
		{
			for(int i = 0; i < dim; ++i)
			{
				if (basicCallExprArray.getArgument(i+1).is_a<LiteralExpression>())
				{
					long_long_t iVal = GetIntLiteralValue(basicCallExprArray.getArgument(i+1).cast_ptr<LiteralExpression>(), 1);					
					int iMultN = 1;
					for (int j= i+1; j < aLimSize; j++) 
						iMultN *= aLim[j];
					iOffset += iVal * iMultN;
				}
			}
		}		
	}
	return iOffset;
}

//Возвращает размер области памяти для переменной
int MemoryManager::GetSizeOf(TypeBase& typeBase, bool isFullSize)
{
	int size = 0;	
	if (typeBase.is_a<BasicType>())
	{
		BasicType &basicType = typeBase.cast_to<BasicType>();
		size = basicType.getSizeOf();
	}
	else if (typeBase.is_a<PtrType>())
	{
		size = 4;
	}
	else if (typeBase.is_a<ArrayType>())
	{
		ArrayType &arrayType = typeBase.cast_to<ArrayType>();
		size = GetSizeOf(arrayType.getBaseType(), isFullSize);
		if (isFullSize)
			size *= arrayType.getElementCount();
	}
	return size;
}

} // end namespace AliasAnalysis
} // end namespace OPS
