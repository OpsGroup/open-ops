#ifndef SUBROUTINES_SHARED_H
#define SUBROUTINES_SHARED_H

#include "Reprise/Declarations.h"


namespace OPS
{
	namespace Shared
	{
		OPS::Reprise::SubroutineDeclaration* findSubroutineWithImplementationByName(OPS::Reprise::ProgramUnit* pProgramUnit, std::string subroutineName);

		OPS::Reprise::SubroutineDeclaration* findSubroutineWithImplementationByName(OPS::Reprise::TranslationUnit* pTranslationUnit, std::string subroutineName);
		
		OPS::Reprise::SubroutineDeclaration* findSubroutineByName(OPS::Reprise::TranslationUnit* pTranslationUnit, std::string subroutineName);

		OPS::Reprise::TranslationUnit* getTranslationUnitBySubroutineDeclaration(OPS::Reprise::SubroutineDeclaration* pSubroutineDeclaration);

		OPS::Reprise::SubroutineDeclaration* getSubroutineDeclarationByStatement(OPS::Reprise::StatementBase* pStatement);

		OPS::Reprise::TypeDeclaration* findTypeDeclarationByName(OPS::Reprise::TranslationUnit* ptranslationUnit, std::string typeName);

		OPS::Reprise::TypeBase* findDeclaredTypeByName(OPS::Reprise::TranslationUnit* ptranslationUnit, std::string typeName);

        OPS::Reprise::TypeBase* getArgumentPointedType(OPS::Reprise::SubroutineDeclaration* pSubroutineDeclaration, int argumentIndex);
	}
}

#endif
