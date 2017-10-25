#include "Analysis/Frames/BlockNestAnalizer.h"

#include "Analysis/LoopsInterchange/LoopsInterchangeAnalysis.h"
#include "OPS_Core/Localization.h"
#include "Shared/Checks.h"
#include "Shared/LoopShared.h"
#include "Shared/DataShared.h"
#include "Shared/ExpressionHelpers.h"
#include "Transforms/Loops/LoopInterchange/LoopInterchange.h"
#include "Transforms/Loops/StripMining/StripMining.h"

namespace OPS
{
namespace Analysis
{
namespace Frames
{

using namespace OPS::Reprise;
using namespace OPS::Shared;
using namespace OPS::Shared::ExpressionHelpers;

BlockNestAnalizer::BlockNestAnalizer(ForStatement& outerLoop, int loopNestDepth)
    : m_outerLoop(outerLoop), m_loopNestDepth(loopNestDepth)
{

}

bool BlockNestAnalizer::analyse()
{
    m_errors.clear();

    validateAndLookupLoopNestInfo();
    checkConsistency();

    if(m_errors.size() != 0)
    {
        checkEquivalency();
    }

    return m_errors.size() == 0;
}

void BlockNestAnalizer::validateAndLookupLoopNestInfo()
{
    if(m_loopNestDepth <= 0)
    {
        m_errors.push_back(_TL("Invalid loopNestDepth", "Некорректное значение loopNestDepth"));
        return;
    }

    m_nestLoops.resize(m_loopNestDepth);

    ForStatement* pCurrentLoop = &m_outerLoop;
    for(int loopIndex = 0; loopIndex < m_loopNestDepth; ++loopIndex)
    {
        if(pCurrentLoop == NULL)
        {
            m_errors.push_back(_TL("Unable to find dense loop nest of required size", "Не удалось найти тесное гнездо циклов указанного размера"));
            return;
        }
        m_nestLoops[loopIndex] = pCurrentLoop;

        for(Declarations::VarIterator iter = m_nestLoops[loopIndex]->getBody().getDeclarations().getFirstVar(); iter.isValid(); ++iter)
        {
            if(iter->hasDefinedBlock() && &iter->getDefinedBlock() == &m_nestLoops[loopIndex]->getBody())
            {
                m_errors.push_back(_TL("Loop nest must contain no declarations", "Гнездо циклов не должно содержать объявлений"));
                break;
            }
        }

        if(!Editing::forHeaderIsCanonized(*pCurrentLoop))
        {
            m_errors.push_back(_TL("All loops in target nest must be canonized", "Все циклы в целевом гнезде должны быть канонизированы"));
            return;
        }

        if(pCurrentLoop->getBody().getChildCount() != 1)
        {
            pCurrentLoop = NULL;
        }
        else
        {
            pCurrentLoop = pCurrentLoop->getBody().getChild(0).cast_ptr<ForStatement>();
        }
    }
}

void BlockNestAnalizer::checkConsistency()
{
    Checks::CompositionCheckObjects acceptableObjects;
    acceptableObjects << Checks::CompositionCheckObjects::CCOT_BlockStatement;
    acceptableObjects << Checks::CompositionCheckObjects::CCOT_ForStatement;
    acceptableObjects << Checks::CompositionCheckObjects::CCOT_WhileStatement;
    acceptableObjects << Checks::CompositionCheckObjects::CCOT_IfStatement;
    acceptableObjects << Checks::CompositionCheckObjects::CCOT_ExpressionStatement;

    if(!makeCompositionCheck(m_outerLoop, acceptableObjects))
    {
        m_errors.push_back(_TL("Unsupported statements", "Обнаружены неподдерживаемые операторы"));
    }
}

void BlockNestAnalizer::checkEquivalency()
{
    for(int loopIndex = 0; loopIndex < m_loopNestDepth - 1; ++loopIndex)
    {
        OPS_ASSERT(m_nestLoops[loopIndex] != NULL);
        if(!OPS::Analysis::LoopsInterchange::isInterchangable(*m_nestLoops[loopIndex]))
        {
            m_errors.push_back(_TL("Loop interchanging is not equivalent", "Перестановка циклов не эквивалентна"));
            return;
        }
    }
}

std::string BlockNestAnalizer::getErrorMessage()
{
    std::string error = "";

    for(std::list<std::string>::iterator it = m_errors.begin(); it != m_errors.end(); ++it)
    {
        error = error + "\n" + *it;
    }

    return error;
}

}
}
}
