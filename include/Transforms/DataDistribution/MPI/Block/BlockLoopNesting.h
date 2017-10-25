#ifndef BLOCK_LOOP_NESTING_H
#define BLOCK_LOOP_NESTING_H

#include "Transforms/MPI/MPIGlobalsProducer/IMPIRankProducer.h"
#include "Transforms/MPI/MPIGlobalsProducer/IMPISizeProducer.h"
#include "Transforms/MPI/Utils/MPIProducerFactory.h"
#include "Transforms/DataDistribution/MPI/Block/BlockIndexesProducer.h"
#include "Transforms/Loops/LoopNestBlocking/LoopNestBlocking.h"
#include "Reprise/Reprise.h"
#include "Transforms/ITransformation.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{
    using OPS::Transforms::MPIProducer::MPIProducerFactory;
    using OPS::Transforms::MPIProducer::IMPIRankProducer;
    using OPS::Transforms::MPIProducer::IMPISizeProducer;
    using OPS::Reprise::ReprisePtr;
    using OPS::Reprise::ForStatement;
    using OPS::Reprise::SubroutineDeclaration;
    using OPS::Reprise::VariableDeclaration;
    using OPS::Transforms::Loops::LoopNestBlocking;

    class BlockLoopNesting: public ITransformation, public OPS::NonCopyableMix, public OPS::NonAssignableMix
    {
    public:
        // Constructor
        BlockLoopNesting(ForStatement& outerForStatement, const std::vector<int>& blockWidths, const std::vector<int>& iterationCounts);

        // Destructor
        virtual ~BlockLoopNesting();

        // ITransformation implementation
        virtual bool analyseApplicability();
        virtual std::string getErrorMessage();
        virtual void makeTransformation();

    private:
        MPIProducerFactory* m_pFactory;
        IMPIRankProducer* m_pRankProducer;
        IMPISizeProducer* m_pSizeProducer;
        LoopNestBlocking* m_pLoopNestBlocking;
        BlockIndexesProducer* m_pBlockIndexesProducer;

        ReprisePtr<ForStatement> m_rpOuterForStatement;
        std::vector<int> m_blockWidths;
        std::vector<int> m_iterationCounts;

        SubroutineDeclaration* m_pParentSubroutine;
        VariableDeclaration* m_pRankDeclaration;
        VariableDeclaration* m_pSizeDeclaration;

        std::list<std::string> m_errors;

        bool m_analysisRerformed;
    };
}
}
}

#endif // BLOCK_LOOP_NESTING_H
