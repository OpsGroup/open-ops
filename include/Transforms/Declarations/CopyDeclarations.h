#ifndef COPYDECLARATIONS_H
#define COPYDECLARATIONS_H

#include "Reprise/Reprise.h"

#include <string>
#include <vector>

namespace OPS
{
namespace Transforms
{
namespace Declarations
{

class CopyDeclarations
{
public:
    /// конструктор
    CopyDeclarations(const std::vector<std::string>& includePaths);

    /// формирует единицу трансляции содержащую объявления
    void initSourceTranslationUnit(std::string pathToFile);

    /// копирует все объявления в целевую (TargetTranslationUnit) единицу трансляции
    void copyAllDeclarations(Reprise::TranslationUnit* TargetTranslationUnit);

    /// копирует выбранное объявление функции в целевую (TargetTranslationUnit) единицу трансляции
    void copySubroutineDeclarationByName(Reprise::TranslationUnit* TargetTranslationUnit, std::string subroutineName);

protected:
    Reprise::ReprisePtr<Reprise::TranslationUnit> m_unitWithDeclarations;
    std::vector<std::string> m_includePaths;
};

}
}
}

#endif // COPYDECLARATIONS_H
