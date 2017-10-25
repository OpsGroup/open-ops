#include "Analysis/AliasAnalysis/AliasAnalyzer.h"

namespace OPS
{
	namespace AliasAnalysis
	{
		AliasAnalyzer::AliasAnalyzer(ProgramUnit* aProgramUnit): m_ProgramUnit(aProgramUnit)
		{
			if (!IsParameters())
				throw RuntimeError("Main function not found.");
			m_CallGraphEx = new CallGraphEx(m_MainUnit);
			m_MemManager = new MemoryManager(m_MainUnit, m_CallGraphEx);
			m_ProcAnalyzer = new ProcAnalyzer(m_CallGraphEx, m_MemManager);
			Make();
		}

		AliasAnalyzer::~AliasAnalyzer(void)
		{
			delete m_ProcAnalyzer;
			delete m_MemManager;
			delete m_CallGraphEx;
		}

		bool AliasAnalyzer::Make(void)
		{
			m_ProcAnalyzer->Run("main", m_CallGraphEx->GetExprCallExMain());
			return 0;
		}

		AliasResult AliasAnalyzer::IsAlias(ReferenceExpression* firstPointer, SubroutineCallExpression* firstExprCall
										  ,ReferenceExpression* secondPointer, SubroutineCallExpression* secondExprCall)
		{
			if (firstPointer && secondPointer)
			{
				if (m_MemManager->CheckType(firstPointer, secondPointer))
				{
					ExprCallEx *firstExprCallEx, *secondExprCallEx;

					if (firstExprCall)
						firstExprCallEx = m_MemManager->GetCallGraphEx()->GetSubProcExprCallEx(firstExprCall);
					else
						firstExprCallEx = GetSubCallExpr(firstPointer);

					if (secondExprCall)
						secondExprCallEx = m_MemManager->GetCallGraphEx()->GetSubProcExprCallEx(secondExprCall);
					else
						secondExprCallEx = GetSubCallExpr(secondPointer);

					SetAbstractMemoryCell samcFirst = firstExprCallEx->GetPointersTable()->GetSAMC(firstPointer);
					SetAbstractMemoryCell samcSecond = secondExprCallEx->GetPointersTable()->GetSAMC(secondPointer);

					return SAMCCompare().IsNamesAlias(samcFirst, samcSecond);
				}
				return NO_ALIAS;
			}
			return MAY_ALIAS;
		}

		bool AliasAnalyzer::IsParameters(void)
		{
			if (m_ProgramUnit)
			{
				int translationUnitCount = m_ProgramUnit->getUnitCount();
				for(int tu = 0; tu < translationUnitCount; ++tu)
				{
					for (Declarations::SubrIterator iter = m_ProgramUnit->getUnit(tu).getGlobals().getFirstSubr(); iter.isValid(); ++iter)
						if (iter->getName() == "main")
						{
							m_MainProc = &*iter;
							m_MainUnit = &m_ProgramUnit->getUnit(tu);
							return true;
						}
				}
			}
			m_MainProc = 0;
			m_MainUnit = 0;
			return false;
		}

		ExprCallEx* AliasAnalyzer::GetSubCallExpr(ReferenceExpression* pointer)
		{
			ExprCallEx *exprCallEx = 0;
			if (pointer)
			{
				RepriseBase* pParent = pointer->getParent();
				while (pParent && !(pParent->is_a<SubroutineDeclaration>()))
					pParent = pParent->getParent();
				if (pParent && pParent->is_a<SubroutineDeclaration>())
				{
					SubroutineDeclaration &subroutineDeclaration = pParent->cast_to<SubroutineDeclaration>();
					if (m_MainProc->hasImplementation())
					{
						SearchProcCall searchProcCall(subroutineDeclaration.getName());
						searchProcCall.visit(m_MainProc->getBodyBlock());
						if (searchProcCall.getSubCallExpr())
							exprCallEx = m_MemManager->GetCallGraphEx()->GetSubProcExprCallEx(searchProcCall.getSubCallExpr());
					}
				}
			}
			if (!exprCallEx)
				exprCallEx = m_CallGraphEx->GetExprCallExMain();
			return exprCallEx;
		}
	}
}
