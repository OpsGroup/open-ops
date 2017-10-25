#ifndef MPI_SINGLE_ACCESS_AREA_PRODUCER_H
#define MPI_SINGLE_ACCESS_AREA_PRODUCER_H

#include "Transforms/MPI/MPIGlobalsProducer/MPIRankProducer.h"
#include "Transforms/MPI/MPIGlobalsProducer/MPISizeProducer.h"
#include "Transforms/MPI/Utils/MPIProducerFactory.h"
#include "Reprise/Reprise.h"
#include "Transforms/DataDistribution/MPI/IMPISingleAccessAreaProducer.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{
    using OPS::Transforms::MPIProducer::MPIProducerFactory;
    using OPS::Transforms::MPIProducer::MPIHelper;
    using OPS::Transforms::MPIProducer::IMPIRankProducer;
    using OPS::Transforms::MPIProducer::IMPISizeProducer;
    using OPS::Reprise::ExpressionBase;
    using OPS::Reprise::ReprisePtr;
    using OPS::Reprise::StatementBase;
    using OPS::Reprise::SubroutineDeclaration;
    using OPS::Reprise::VariableDeclaration;

    class MPISingleAccessAreaProducer: public IMPISingleAccessAreaProducer, public OPS::NonCopyableMix, public OPS::NonAssignableMix
    {
    public:
        // Constructor
        MPISingleAccessAreaProducer(StatementBase& initialisationSatatement, ReprisePtr<ExpressionBase> rpShiftExpression);

        // Destructor
        virtual ~MPISingleAccessAreaProducer();

        // ITransaformation implementation
        virtual bool analyseApplicability();
        virtual std::string getErrorMessage();
        virtual void makeTransformation();

        // IMPISingleAccessAreaProducer implementation
        virtual void setDeclarationsToBcast(VariableDeclarationContainer declarations);

    private:
        MPIProducerFactory* m_pFactory;
        MPIHelper* m_pHelper;
        IMPIRankProducer* m_pRankProducer;
        IMPISizeProducer* m_pSizeProducer;
        
        ReprisePtr<StatementBase> m_rpInitialisationSatatement;
        ReprisePtr<ExpressionBase> m_rpShiftExpression;

        SubroutineDeclaration* m_pParentSubroutine;
        VariableDeclaration* m_pRankDeclaration;
        VariableDeclaration* m_pSizeDeclaration;

        VariableDeclarationContainer m_declarationsToBcast;

        std::list<std::string> m_errors;

        bool m_analysisRerformed;
    };
}
}
}

#endif // MPI_SINGLE_ACCESS_AREA_PRODUCER_H
