#ifndef LOOP_NEST_BLOCKING_H_INCLUDED
#define LOOP_NEST_BLOCKING_H_INCLUDED

#include "Reprise/Reprise.h"
#include "Transforms/ITransformation.h"

namespace OPS
{
namespace Transforms
{
namespace Loops
{
    using OPS::Reprise::BlockStatement;
    using OPS::Reprise::ExpressionBase;
    using OPS::Reprise::ForStatement;
    using OPS::Reprise::VariableDeclaration;
    using OPS::Reprise::ReprisePtr;

    class LoopNestBlocking: public ITransformation, public OPS::NonCopyableMix, public OPS::NonAssignableMix
    {
    public:
        LoopNestBlocking(ForStatement& outerLoop, int loopNestDepth, const std::vector<int>& blockWidths, bool skipTails);

        // ITransformation implementation
        virtual bool analyseApplicability();
        virtual std::string getErrorMessage();
        virtual void makeTransformation();

        std::vector<ForStatement*> getResultNestLoops();
        std::vector<VariableDeclaration*> getCounters();

        std::vector<VariableDeclaration*> getOldCounters();

    private:
        void validateBlockWidths();
        void lookupLoopNestInfo();

        void addTails(const std::vector<ReprisePtr<ExpressionBase> >& finalExpressions, ReprisePtr<BlockStatement> rpBody, ReprisePtr<BlockStatement> rpResultContainer);

    private:
        ForStatement& m_outerLoop;
        int m_loopNestDepth;
        std::vector<int> m_blockWidths;
        bool m_skipTails;

        std::vector<ForStatement*> m_nestLoops;
        std::vector<VariableDeclaration*> m_nestCounters;

        std::vector<ForStatement*> m_resultNestLoops;
        std::vector<VariableDeclaration*> m_resultCounters;

        std::list<std::string> m_errors;
        bool m_analysisRerformed;
    };
}
}
}

#endif // LOOP_NEST_BLOCKING_H_INCLUDED
