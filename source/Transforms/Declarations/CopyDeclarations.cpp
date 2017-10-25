#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Transforms/Declarations/GenerateExtern.h"
#include "Transforms/Declarations/CopyDeclarations.h"

using namespace OPS;
using namespace OPS::Reprise;
using namespace std;

namespace OPS
{
	namespace Transforms
	{
		namespace Declarations
		{

			CopyDeclarations::CopyDeclarations(const vector<string>& includePaths)
			{
				m_includePaths.insert(m_includePaths.begin(), includePaths.begin(), includePaths.end());
			}

			void CopyDeclarations::initSourceTranslationUnit(std::string pathToFile)
			{
				if (m_unitWithDeclarations.get())
					delete m_unitWithDeclarations.release();

				Frontend::Frontend frontend;
				for(size_t i = 0; i < m_includePaths.size(); ++i)
					frontend.clangSettings().addIncludePath(m_includePaths[i]);
				frontend.addSourceBuffer("#include <" + pathToFile + ">");

				if (frontend.compile())
				{
					m_unitWithDeclarations = frontend.detachProgramUnit()->removeUnit(0);
				}
				else
				{
					throw OPS::RuntimeError(
						pathToFile + " could not be found!\nErrors:\n" +
						frontend.getResult(0).errorText());
				}
			}

			void CopyDeclarations::copyAllDeclarations(TranslationUnit* TargetTranslationUnit)
			{
				Reprise::Declarations::Iterator iter = m_unitWithDeclarations->getGlobals().getFirst();
				for(; iter.isValid(); ++iter)
				{
					if (iter->is_a<SubroutineDeclaration>())
					{
						SubroutineDeclaration* subrInSource = iter->cast_ptr<SubroutineDeclaration>();
						SubroutineDeclaration* subrInTarget = TargetTranslationUnit->getGlobals().findSubroutine(subrInSource->getName());
						if (!subrInTarget)
							TargetTranslationUnit->getGlobals().addFirst(subrInSource->clone());
					}
					if (iter->is_a<TypeDeclaration>())
					{
						TypeDeclaration* typeInSource = iter->cast_ptr<TypeDeclaration>();
						TypeDeclaration* typeInTarget = TargetTranslationUnit->getGlobals().findType(typeInSource->getName());
						if (!typeInTarget)
							TargetTranslationUnit->getGlobals().addFirst(typeInSource->clone());
					}
					if (iter->is_a<VariableDeclaration>())
					{
						VariableDeclaration* varInSource = iter->cast_ptr<VariableDeclaration>();
						VariableDeclaration* varInTarget = TargetTranslationUnit->getGlobals().findVariable(varInSource->getName());
						if (!varInTarget)
							TargetTranslationUnit->getGlobals().addFirst(varInSource->clone());
					}

				}

				Transforms::Declarations::generateExtern(*TargetTranslationUnit->findProgramUnit());
			}

			void CopyDeclarations::copySubroutineDeclarationByName(TranslationUnit* TargetTranslationUnit, string subroutineName)
			{
				SubroutineDeclaration* subrInSource = m_unitWithDeclarations->getGlobals().findSubroutine(subroutineName);
				if (!subrInSource)
					throw new OPS::ArgumentError("In function copySubroutineDeclarationByName: no subroutine named " + subroutineName 
					+ " to copy from " + m_unitWithDeclarations->getSourceFilename());
				SubroutineDeclaration* subrInTarget = TargetTranslationUnit->getGlobals().findSubroutine(subroutineName);
				if (!subrInTarget) 
					TargetTranslationUnit->getGlobals().addFirst(subrInSource->clone());
				generateExtern(*TargetTranslationUnit->findProgramUnit());
			}

		}
	}
}
