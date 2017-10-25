#include "Transforms/Loops/LoopNestBlocking/LoopNestBlocking.h"

#include "Analysis/Frames/BlockNestAnalizer.h"
#include "OPS_Core/MemoryHelper.h"
#include "OPS_Core/Localization.h"
#include "Shared/Checks.h"
#include "Shared/LoopShared.h"
#include "Shared/DataShared.h"
#include "Shared/ExpressionHelpers.h"
#include "Transforms/Loops/LoopInterchange/LoopInterchange.h"
#include "Transforms/Loops/StripMining/StripMining.h"

namespace OPS
{
namespace Transforms
{
namespace Loops
{

using namespace OPS::Reprise;
using namespace OPS::Shared;
using namespace OPS::Shared::ExpressionHelpers;
using namespace OPS::Analysis::Frames;

LoopNestBlocking::LoopNestBlocking(ForStatement& outerLoop, int loopNestDepth, const std::vector<int>& blockWidths, bool skipTails)
    : m_outerLoop(outerLoop), m_loopNestDepth(loopNestDepth), m_blockWidths(blockWidths), m_skipTails(skipTails)
{

}

bool LoopNestBlocking::analyseApplicability()
{
    m_errors.clear();

    validateBlockWidths();

    std::tr1::shared_ptr<BlockNestAnalizer> spBna(new BlockNestAnalizer(m_outerLoop, m_loopNestDepth));
    if(!spBna->analyse())
    {
        m_errors.push_back(spBna->getErrorMessage());
    }

    m_analysisRerformed = true;

    return m_errors.size() == 0;
}

void LoopNestBlocking::lookupLoopNestInfo()
{
    m_nestLoops.resize(m_loopNestDepth);
    m_nestCounters.resize(m_loopNestDepth);

    ForStatement* pCurrentLoop = &m_outerLoop;
    for(int loopIndex = 0; loopIndex < m_loopNestDepth; ++loopIndex)
    {
        m_nestLoops[loopIndex] = pCurrentLoop;
        m_nestCounters[loopIndex] = &Editing::getBasicForCounter(*pCurrentLoop).getReference();

        pCurrentLoop = pCurrentLoop->getBody().getChild(0).cast_ptr<ForStatement>();
    }
}

void LoopNestBlocking::validateBlockWidths()
{
    OPS_ASSERT(m_loopNestDepth > 0);

	if(int(m_blockWidths.size()) != m_loopNestDepth)
    {
        m_errors.push_back(_TL("Invalid blockWidths size", "Некорректный размер коллекции blockWidths"));
    }
	for(size_t blockWidthIndex = 0; blockWidthIndex < m_blockWidths.size(); ++blockWidthIndex)
    {
        if(m_blockWidths[blockWidthIndex] <= 0)
        {
            m_errors.push_back(_TL("Invalid blockWidth", "Некорректное значение ширины блока"));
        }
    }
}

std::string LoopNestBlocking::getErrorMessage()
{
    std::string error = "";

    for(std::list<std::string>::iterator it = m_errors.begin(); it != m_errors.end(); ++it)
    {
        error = error + "\n" + *it;
    }

    return error;
}

void LoopNestBlocking::makeTransformation()
{
    if(!m_analysisRerformed)
    {
        analyseApplicability();
    }
    OPS_ASSERT(m_errors.size() == 0);

    lookupLoopNestInfo();

    IntegerHelper ih(BasicType::BT_INT32);
    ReprisePtr<BlockStatement> rpCounterPostAssignBlock(new BlockStatement());
    ReprisePtr<BlockStatement> rpTailsBlock(new BlockStatement());
    std::vector<ReprisePtr<ExpressionBase> > finalExpressions(m_loopNestDepth);
    ReprisePtr<BlockStatement> rpNestBody(&m_nestLoops[m_loopNestDepth - 1]->getBody());
    m_resultNestLoops.resize(m_loopNestDepth * 2);
    m_resultCounters.resize(m_loopNestDepth * 2);

    for(int loopIndex = m_loopNestDepth - 1; loopIndex >= 0; --loopIndex)
    {
        finalExpressions[loopIndex].reset(Editing::getBasicForFinalExpression(*m_nestLoops[loopIndex]).clone());

        ReprisePtr<ExpressionBase> rpStripMiningParameter(&ih(m_blockWidths[loopIndex]));
        BlockStatement& stripMiningResult = makeStripMining(*m_nestLoops[loopIndex], rpStripMiningParameter);

        OPS_ASSERT(stripMiningResult.getChild(0).is_a<ForStatement>());
        m_resultNestLoops[loopIndex * 2] = stripMiningResult.getChild(0).cast_ptr<ForStatement>();
        m_resultCounters[loopIndex * 2] = &Editing::getBasicForCounter(*m_resultNestLoops[loopIndex * 2]).getReference();
        OPS_ASSERT(m_resultNestLoops[loopIndex * 2]->getBody().getChildCount() == 1 && m_resultNestLoops[loopIndex * 2]->getBody().getChild(0).is_a<ForStatement>());
        m_resultNestLoops[loopIndex * 2 + 1] = m_resultNestLoops[loopIndex * 2]->getBody().getChild(0).cast_ptr<ForStatement>();
        m_resultCounters[loopIndex * 2 + 1] = &Editing::getBasicForCounter(*m_resultNestLoops[loopIndex * 2 + 1]).getReference();

        // Remove tail
        stripMiningResult.erase(stripMiningResult.getChild(1).cast_ptr<StatementBase>());

        // Move post loop counter assign to specific block
        rpCounterPostAssignBlock->addLast(stripMiningResult.getChild(1).cast_to<StatementBase>().clone());
        stripMiningResult.erase(stripMiningResult.getChild(1).cast_ptr<StatementBase>());

        if(loopIndex > 0)
        {
            m_nestLoops[loopIndex - 1]->setBody(stripMiningResult.clone());
        }
        else
        {
            if(!m_skipTails)
            {
                stripMiningResult.addLast(rpTailsBlock.get());
            }
            stripMiningResult.addLast(rpCounterPostAssignBlock.get());
        }
    }

    // Interchange loops
    for(int loopIndex = 1; loopIndex < m_loopNestDepth; ++loopIndex)
    {
        for(int interchangingLoopIndex = loopIndex; interchangingLoopIndex < 2 * loopIndex; ++interchangingLoopIndex)
        {
            makeLoopInterchange(*m_resultNestLoops[interchangingLoopIndex]);
        }
    }

    // Add tails to the end
    if(!m_skipTails)
    {
        addTails(finalExpressions, rpNestBody, rpTailsBlock);
    }
}

void LoopNestBlocking::addTails(const std::vector<ReprisePtr<ExpressionBase> >& finalExpressions, ReprisePtr<BlockStatement> rpBody, ReprisePtr<BlockStatement> rpResultContainer)
{
    for(int tailDimensionNumber = m_loopNestDepth - 1; tailDimensionNumber >= 0; --tailDimensionNumber)
    {
        BlockStatement* pCurrentBlock = rpResultContainer.get();
        for(int loopIndex = 0; loopIndex < m_loopNestDepth; ++loopIndex)
        {
            ReprisePtr<ExpressionBase> rpCounter(new ReferenceExpression(*m_nestCounters[loopIndex]));
            IntegerHelper c(rpCounter->getResultType()->cast_to<BasicType>());
            ForStatement* pCurrentForStatement = NULL;
            if(loopIndex < tailDimensionNumber)
            {
                pCurrentForStatement = new ForStatement(
                        &(op(rpCounter) R_AS c(0)),
                        &(op(rpCounter) < (op(finalExpressions[loopIndex]) / c(m_blockWidths[loopIndex])) * c(m_blockWidths[loopIndex])),
                        &(op(rpCounter) R_AS op(rpCounter) + c(1))
                    );
            }
            else if(loopIndex == tailDimensionNumber)
            {
                pCurrentForStatement = new ForStatement(
                        &(op(rpCounter) R_AS (op(finalExpressions[loopIndex]) / c(m_blockWidths[loopIndex])) * c(m_blockWidths[loopIndex])),
                        &(op(rpCounter) < op(finalExpressions[loopIndex])),
                        &(op(rpCounter) R_AS op(rpCounter) + c(1))
                    );
            }
            else
            {
                pCurrentForStatement = new ForStatement(
                        &(op(rpCounter) R_AS c(0)),
                        &(op(rpCounter) < op(finalExpressions[loopIndex])),
                        &(op(rpCounter) R_AS op(rpCounter) + c(1))
                    );
            }

            pCurrentBlock->addLast(pCurrentForStatement);

            if(loopIndex == m_loopNestDepth - 1)
            {
                pCurrentForStatement->setBody(rpBody->clone());
            }

            pCurrentBlock = &pCurrentForStatement->getBody();
        }
    }
}

std::vector<ForStatement*> LoopNestBlocking::getResultNestLoops()
{
    return m_resultNestLoops;
}

std::vector<VariableDeclaration*> LoopNestBlocking::getCounters()
{
    return m_resultCounters;
}

std::vector<VariableDeclaration*> LoopNestBlocking::getOldCounters()
{
    return m_nestCounters;
}

}
}
}
