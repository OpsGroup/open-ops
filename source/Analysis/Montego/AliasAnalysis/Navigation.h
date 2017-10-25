#pragma once
#include "Reprise/Reprise.h"
#include "Analysis/ControlFlowGraph.h"
#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"

namespace OPS
{
namespace Montego
{
class BasicOccurrence;
class OccurrenceContainer;
class ExpressionAnalyser;
class MemoryCellContainer;
class AliasAnalysisContext;

/// Класс определения порядка обхода процедур и выражений анализатором процедур
/// Один экземпляр класса нельзя использовать для одновременного обхода несколькими анализаторами процедур
class Navigation : public OPS::NonCopyableMix
{
public:

    //строит необходимые для анализатора процедур графы
    Navigation(Reprise::ProgramUnit &pu, AliasAnalysisOptions opt);

    ~Navigation();

    //Проводит анализ множеств достижимости в непосещенной части графа вызовов
    //и возвращает следующую процедуру, с которой нужно начать обход
    //если вернет 0, значит все обошли
    Reprise::SubroutineDeclaration* getNextSubroutine();

    // анализатор процедур сообщает навигатору, что он посетил данную процедуру
    void putNextSubroutine(Reprise::SubroutineDeclaration* sd);

private:

    AliasAnalysisOptions m_options;

    //какие функции вызывает данная (учитываются только функции с определением)
    std::map<Reprise::SubroutineDeclaration*, std::set<Reprise::SubroutineDeclaration*> > m_subroutineCalls;
    //какие функции вызывают данную
    std::map<Reprise::SubroutineDeclaration*, std::set<Reprise::SubroutineDeclaration*> > m_invertedSubroutineCalls;

    //какие функции потенциально могут быть вызваны по указателю (учитываются только функции с определением)
    std::set<Reprise::SubroutineDeclaration*> m_calledByPointerFuncs;
    //какие функции содержат вызовы по указателю
    std::set<Reprise::SubroutineDeclaration*> m_funcsWithPointerCall;

    //количество операторов внутри тела функции
    std::map<Reprise::SubroutineDeclaration*, int> m_subroutineSize;

    //непосещенные процедуры (учитываются только функции с определением)
    std::set<Reprise::SubroutineDeclaration*> m_notVisitedSubroutines;

    friend class AliasImplementation;
};

}
}
