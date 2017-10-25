#ifndef MPI_DATA_DISTRIBUTION_SCATTER_H
#define MPI_DATA_DISTRIBUTION_SCATTER_H

#include <list>

#include "Transforms/MPI/MPIGlobalsProducer/MPIRankProducer.h"
#include "Transforms/MPI/Utils/MPIProducerFactory.h"
#include "Reprise/Reprise.h"
#include "Transforms/DataDistribution/BlockAffineDataDistributionParameters.h"
#include "Transforms/DataDistribution/MPI/IMPIDataDistributionScatter.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{
    using OPS::Transforms::MPIProducer::MPIProducerFactory;
    using OPS::Transforms::MPIProducer::MPIHelper;
    using OPS::Transforms::MPIProducer::IMPIRankProducer;
    using OPS::Reprise::BasicType;
    using OPS::Reprise::ExpressionBase;
    using OPS::Reprise::ReprisePtr;
    using OPS::Reprise::StatementBase;
    using OPS::Reprise::SubroutineDeclaration;
    using OPS::Reprise::TypeBase;
    using OPS::Reprise::VariableDeclaration;
    
    class MPIDataDistributionScatter: public IMPIDataDistributionScatter, public OPS::NonCopyableMix, public OPS::NonAssignableMix
    {
    public:
        // Constructor
        MPIDataDistributionScatter(
            BADDParametersFamily distributionParameters, 
            VariableDeclaration& targetArrayDeclaration, 
            StatementBase& insertionPointStatement, 
            int leadingDimention,
            int sourceNodeNumber);

        // Destructor
        virtual ~MPIDataDistributionScatter();

        // ITransformation implementation
        virtual bool analyseApplicability();
        virtual std::string getErrorMessage();
        virtual void makeTransformation();

        // IMPIDataDistributionScatter implementation
        void setNewArrayDeclaration(VariableDeclaration& newArrayDeclaration) { m_pNewArrayDeclaration = &newArrayDeclaration; }

    private:
        // Typedefs
        typedef std::vector<int> MultyIndex;
        typedef std::vector<VariableDeclaration*> VariableContainer;

        // Fields
        MPIProducerFactory* m_pFactory;
        MPIHelper* m_pHelper;
        IMPIRankProducer* m_pRankProducer;

        BADDParametersFamily m_distributionParameters;
        VariableDeclaration& m_targetArrayDeclaration;
        VariableDeclaration* m_pNewArrayDeclaration;
        StatementBase& m_insertionPointStatement;
        int m_leadingDimention;

        VariableDeclaration* m_pRankDeclaration;
        BasicType* m_pArrayElementBasicType;

        std::list<std::string> m_errors;

        int m_sourceNodeNumber;

        SubroutineDeclaration* m_pMallocDeclaration;
        SubroutineDeclaration* m_pFreeDeclaration;

        bool m_analysisRerformed;

        // Methods
        virtual ReprisePtr<StatementBase> createMemoryAllocatingStatement(VariableDeclaration& variableDeclaration, int sizeInElements, BasicType& typeOfElements);
        virtual ReprisePtr<StatementBase> createMemoryDeallocatingStatement(VariableDeclaration& variableDeclaration);
    };
}
}
}

#endif
