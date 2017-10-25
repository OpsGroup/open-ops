#ifndef BLOCK_NEST_ANALIZER_H_INCLUDED
#define BLOCK_NEST_ANALIZER_H_INCLUDED

#include "Reprise/Reprise.h"
#include "Transforms/ITransformation.h"

namespace OPS
{
namespace Analysis
{
namespace Frames
{
    using OPS::Reprise::BlockStatement;
    using OPS::Reprise::ExpressionBase;
    using OPS::Reprise::ForStatement;
    using OPS::Reprise::VariableDeclaration;
    using OPS::Reprise::ReprisePtr;

    class BlockNestAnalizer: public OPS::NonCopyableMix, public OPS::NonAssignableMix
    {
    public:
        BlockNestAnalizer(ForStatement& outerLoop, int loopNestDepth);

        bool analyse();
        virtual std::string getErrorMessage();

    private:
        void validateAndLookupLoopNestInfo();
        void checkConsistency();
        void checkEquivalency();

    private:
        ForStatement& m_outerLoop;
        int m_loopNestDepth;

        std::vector<ForStatement*> m_nestLoops;

        std::list<std::string> m_errors;
    };
}
}
}

#endif // BLOCK_NEST_ANALIZER_H_INCLUDED
