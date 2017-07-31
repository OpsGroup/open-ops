#pragma once

#include "Reprise/Units.h"
#include "Reprise/Statements.h"

namespace OPS
{
namespace Shared
{

	OPS::Reprise::ReprisePtr<OPS::Reprise::ProgramUnit> deepCloneProgramUnit(OPS::Reprise::ProgramUnit& unit);

	OPS::Reprise::ReprisePtr<OPS::Reprise::TranslationUnit> deepCloneTranslationUnit(OPS::Reprise::TranslationUnit& unit);

	void doPostCloneLinkFix(const OPS::Reprise::RepriseBase& original, OPS::Reprise::RepriseBase& clone);

	OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> cloneStatement(
				const OPS::Reprise::StatementBase& stmt,
				const OPS::Reprise::Declarations& sourceDeclarations,
				OPS::Reprise::Declarations& destDeclarations
				);
}
}
