#ifndef MPI_LOOP_NESTING_H
#define MPI_LOOP_NESTING_H

#include "Transforms/MPI/MPIGlobalsProducer/IMPIRankProducer.h"
#include "Transforms/MPI/MPIGlobalsProducer/IMPISizeProducer.h"
#include "Transforms/MPI/Utils/MPIProducerFactory.h"
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

    class MPILoopNesting: public ITransformation, public OPS::NonCopyableMix, public OPS::NonAssignableMix
    {
    public:
        // Constructor
        MPILoopNesting(ForStatement& forStatement);

        // Destructor
        virtual ~MPILoopNesting();

        // ITransformation implementation
        virtual bool analyseApplicability();
        virtual std::string getErrorMessage();
        virtual void makeTransformation();

    private:
        MPIProducerFactory* m_pFactory;
        IMPIRankProducer* m_pRankProducer;
        IMPISizeProducer* m_pSizeProducer;
        
        ReprisePtr<ForStatement> m_rpForStatement;

        SubroutineDeclaration* m_pParentSubroutine;
        VariableDeclaration* m_pRankDeclaration;
        VariableDeclaration* m_pSizeDeclaration;

        std::list<std::string> m_errors;

        bool m_analysisRerformed;
    };
}
}
}

#endif
