#include "Shared/SubroutinesShared.h"
#include "SubroutinesSharedDeepWalkers.h"

namespace OPS
{
	namespace Shared
	{
		using namespace OPS;
		using namespace OPS::Reprise;
		
		SubroutineDeclaration* findSubroutineWithImplementationByName(OPS::Reprise::ProgramUnit* pProgramUnit, std::string subroutineName)
		{
			OPS_ASSERT(pProgramUnit != NULL);

			FindSubroutinesByNameDeepWalker deepWalker(subroutineName, true);
			deepWalker.visit(*pProgramUnit);

			FindSubroutinesByNameDeepWalker::SubroutinesDeclarationsContainer dwResult = deepWalker.getSubroutinesDeclaration();
			if(dwResult.size() > 0)
			{
				return *(dwResult.begin());
			}

			return NULL;
		}

		SubroutineDeclaration* findSubroutineWithImplementationByName(OPS::Reprise::TranslationUnit* pTranslationUnit, std::string subroutineName)
		{
			OPS_ASSERT(pTranslationUnit != NULL);

			FindSubroutinesByNameDeepWalker deepWalker(subroutineName, true);
			deepWalker.visit(*pTranslationUnit);

			FindSubroutinesByNameDeepWalker::SubroutinesDeclarationsContainer dwResult = deepWalker.getSubroutinesDeclaration();
			if(dwResult.size() > 0)
			{
				return *(dwResult.begin());
			}

			return NULL;
		}

		SubroutineDeclaration* findSubroutineByName(OPS::Reprise::TranslationUnit* pTranslationUnit, std::string subroutineName)
		{
			OPS_ASSERT(pTranslationUnit != NULL);

			FindSubroutinesByNameDeepWalker deepWalker(subroutineName, false);
			deepWalker.visit(*pTranslationUnit);

			FindSubroutinesByNameDeepWalker::SubroutinesDeclarationsContainer dwResult = deepWalker.getSubroutinesDeclaration();
			if(dwResult.size() > 0)
			{
				return *(dwResult.begin());
			}

			return NULL;
		}

		TranslationUnit* getTranslationUnitBySubroutineDeclaration(SubroutineDeclaration* pSubroutineDeclaration)
		{
			OPS_ASSERT(pSubroutineDeclaration != NULL);

			TranslationUnit* pResult = NULL;
			
			if(pSubroutineDeclaration->getParent() != NULL && pSubroutineDeclaration->getParent()->getParent())
			{
				pResult = pSubroutineDeclaration->getParent()->getParent()->cast_ptr<TranslationUnit>();
			}

			return pResult;
		}

		TypeDeclaration* findTypeDeclarationByName(TranslationUnit* pTranslationUnit, std::string typeName)
		{
			OPS_ASSERT(pTranslationUnit != NULL);

			FindTypesDeclarationsByNameDeepWalker deepWalker(typeName);
			deepWalker.visit(*pTranslationUnit);

			FindTypesDeclarationsByNameDeepWalker::TypesDeclarationsContainer dwResult = deepWalker.getSubroutinesDeclaration();
			if(dwResult.size() > 0)
			{	
				return *(dwResult.begin());
			}

			return NULL;
		}

        OPS::Reprise::TypeBase* getArgumentPointedType(OPS::Reprise::SubroutineDeclaration* pSubroutineDeclaration, int argumentIndex)
		{
			OPS_ASSERT(pSubroutineDeclaration != NULL);
			OPS_ASSERT(argumentIndex >= 0);

			if(pSubroutineDeclaration->getType().getParameterCount() <= argumentIndex)
			{
				return NULL;
			}

            OPS::Reprise::TypeBase* pResult = &pSubroutineDeclaration->getType().getParameter(argumentIndex).getType();
			while(pResult->is_a<PtrType>())
			{
				pResult = &pResult->cast_ptr<PtrType>()->getPointedType();
			}

			return pResult;
		}

		SubroutineDeclaration* getSubroutineDeclarationByStatement(StatementBase* pStatement)
		{
			OPS_ASSERT(pStatement != NULL);

			BlockStatement& rootBlock = pStatement->getRootBlock();
			return rootBlock.getParent()->cast_ptr<SubroutineDeclaration>();
		}

		OPS::Reprise::TypeBase* findDeclaredTypeByName( OPS::Reprise::TranslationUnit* pTranslationUnit, std::string typeName )
		{
			OPS_ASSERT(pTranslationUnit != NULL);

			FindTypesDeclarationsByNameDeepWalker deepWalker(typeName);
			deepWalker.visit(*pTranslationUnit);

			FindTypesDeclarationsByNameDeepWalker::TypesContainer dwResult = deepWalker.getTypes();
			if(dwResult.size() > 0)
			{	
				return *(dwResult.begin());
			}

			return NULL;
		}
	}
}
