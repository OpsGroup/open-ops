#include "Analysis/AliasAnalysis/VariablesClassifier.h"
#include <vector>
#include <algorithm>
		

namespace OPS
{
namespace AliasAnalysis
{

VariablesClassifier::VariablesClassifier(TranslationUnit* aTranslationUnit)
	: m_TranslationUnit(aTranslationUnit) 
{ 
	SetGlobalPtr(); 
	Build(); 
}

void VariablesClassifier::SetGlobalPtr()
{
	m_GlobalPtr.clear();
	for (Declarations::VarIterator iter = m_TranslationUnit->getGlobals().getFirstVar(); iter.isValid(); ++iter)
	{
		if ((iter->getType().is_a<PtrType>()) ||
			(iter->getType().is_a<ArrayType>()))
		{
			m_GlobalPtr.push_back(iter->getName());
		}
	}
}

void VariablesClassifier::Build()
{
	for (Declarations::SubrIterator iter = m_TranslationUnit->getGlobals().getFirstSubr(); iter.isValid(); ++iter)
	{
		if (iter->hasImplementation())
		{
			ParamCall paramCall;
			ExprName procName = iter->getName();
			paramCall.ConvertParam = GetConvertParam(procName);
			if (!iter->getBodyBlock().isEmpty())
			{
				ClassifierWalker classifierWalker(&paramCall, m_GlobalPtr);
				classifierWalker.visit(iter->getBodyBlock());
			}
			m_ClassifVar[procName] = paramCall;
		}
	}
}

bool VariablesClassifier::IsModifiedVar(const ExprName& procName, const ExprName& exprName)
{
	return (m_ClassifVar[procName].varAccess[exprName] == VAR_GLOB_MODIFIED);
}

bool VariablesClassifier::IsReadOnlyVar(const ExprName& procName, const ExprName& exprName)
{
	return (m_ClassifVar[procName].varAccess[exprName] == VAR_GLOB_READONLY);
}

bool VariablesClassifier::IsModifiedLoc(const ExprName& procName, size_t iNum)
{
	ExprName exprName = m_ClassifVar[procName].ConvertParam[iNum];
	return (m_ClassifVar[procName].varAccess[exprName] == VAR_LOCAL_MODIFIED);
}

bool VariablesClassifier::IsReadOnlyLoc(const ExprName& procName, size_t iNum)
{
	ExprName exprName = m_ClassifVar[procName].ConvertParam[iNum];
	return (m_ClassifVar[procName].varAccess[exprName] == VAR_LOCAL_READONLY);
}

bool VariablesClassifier::IsPointer(ReferenceExpression& referenceExpression)
{
	if (referenceExpression.getReference().getType().is_a<PtrType>())
		return true;
	return false;	
}

VariablesList VariablesClassifier::GetConvertParam(const ExprName& subProcName)
{
	VariablesList convertParam;
	convertParam.clear();			
	for (Declarations::SubrIterator iter = m_TranslationUnit->getGlobals().getFirstSubr(); iter.isValid(); ++iter)
	{
		if (iter->getName() == subProcName)
		{
			Declarations& declarations = iter->getDeclarations();
			for (Declarations::VarIterator iter = declarations.getFirstVar(); iter.isValid(); ++iter)
			{
				if (iter->hasParameterReference())
				{
					if ((iter->getType().is_a<PtrType>()) ||
						(iter->getType().is_a<ArrayType>()))
					{
						convertParam.push_back(iter->getName());
					}
				}
			}
			break;
		}
	}		
	return convertParam;
}


//****************** ClassifierWalker ******************

void ClassifierWalker::SetTypeMod(ReferenceExpression& referenceExpression, bool isReadOnly)
{
	ExprName exprName = referenceExpression.getReference().getName();
	if (IsGlobalPtr(exprName)) 
	{
		if (m_ParamCall->varAccess[exprName] != VAR_GLOB_MODIFIED)
		{
			m_ParamCall->varAccess[exprName] = (isReadOnly) ? VAR_GLOB_READONLY : VAR_GLOB_MODIFIED;
		}
	}
	else if (IsPointer(referenceExpression)) 
	{
		if (m_ParamCall->varAccess[exprName] != VAR_LOCAL_MODIFIED)
		{
			m_ParamCall->varAccess[exprName] = (isReadOnly) ? VAR_LOCAL_READONLY : VAR_LOCAL_MODIFIED;
		}
	}
}

void ClassifierWalker::visit(ReferenceExpression& referenceExpression)
{	
	SetTypeMod(referenceExpression, true);
}

void ClassifierWalker::visit(BasicCallExpression& basicCallExpr)
{
	if ((basicCallExpr.getKind() == BasicCallExpression::BCK_ASSIGN) &&
		(basicCallExpr.getArgument(0).is_a<ReferenceExpression>()))
	{
		SetTypeMod(basicCallExpr.getArgument(0).cast_to<ReferenceExpression>(), false);		
	}
	DeepWalker::visit(basicCallExpr);
}

bool ClassifierWalker::IsGlobalPtr(const ExprName& exprName)
{
	VariablesList::iterator itCurr = find(m_GlobalPtr.begin(), m_GlobalPtr.end(), exprName);
	return (itCurr != m_GlobalPtr.end());
}

bool ClassifierWalker::IsPointer(ReferenceExpression& referenceExpression)
{
	if (referenceExpression.getReference().getType().is_a<PtrType>())
		return true;
	return false;
}

} // end namespace AliasAnalysis
} // end namespace OPS
