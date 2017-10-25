#ifndef DATA_DISTRIBUTION_ANALIZER_H
#define DATA_DISTRIBUTION_ANALIZER_H

#include <list>

#include "Analysis/ControlFlowGraph.h"
#include "Analysis/Montego/OccurrenceContainer.h"
#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"
#include "Reprise/Reprise.h"
#include "Transforms/DataDistribution/BlockAffineDataDistributionParameters.h"

namespace OPS
{
	namespace Transforms
	{
		namespace DataDistribution
		{
			using OPS::Reprise::BlockStatement;
			using OPS::Reprise::ForStatement;
			using OPS::Reprise::StatementBase;
			using OPS::Reprise::VariableDeclaration;

			class SingleAccessAreaInfo
			{
			public:
				typedef std::list<VariableDeclaration*> VariableDeclarationContainer;

			public:
				explicit SingleAccessAreaInfo(StatementBase& statement, int shift);

			public:
				StatementBase& getStatement();
                int getShift();
				VariableDeclarationContainer getGenerators();

			private:
				StatementBase* m_pStatement;
                int m_shift;

				VariableDeclarationContainer m_generators;
			};

			class DistributionAreaInfo
			{
			public:
				typedef std::vector<BADDParameters> BADDParametersContainer;

			public:
                DistributionAreaInfo(StatementBase& statement, VariableDeclaration& declaration, VariableDeclaration* pPartDeclaration, BADDParameters distributionParameters, BADDParameters generatorParameters, bool scatterRequired, bool gatherRequired, bool isBlock);

			public:
				StatementBase&       getStatement();
				VariableDeclaration& getDeclaration();
				VariableDeclaration* getPartDeclaration();

				BADDParameters       getDistributionParameters();
				BADDParametersFamily getDistributionParametersFamily();
				BADDParameters       getGeneratorBADDParameters();

				bool isIterationalProcess();
				
				BADDParameters::ParametersContainer::size_type getLeadingDimention();

				bool hasRefineErrors();

				bool isScatterRequired();
				bool isGatherRequired();

                int getSourceNodeNumber();
                void setSourceNodeNumber(int sourceNodeNumber);

                bool isBlock();

			private:
				BADDParameters getDistrParametersForArrayReference(OPS::Reprise::BasicCallExpression* arrayAccessExpr);

			private:
				StatementBase* m_pStatement;
				VariableDeclaration* m_pDeclaration;
				VariableDeclaration* m_pPartDelaration;
				BADDParameters m_distributionParameters;
				BADDParametersFamily m_distributionParametersFamily;
				BADDParameters       m_generatorBADDParameters;
	
				bool m_refineErrorsDetected;

                bool m_isBlock;

				bool m_scatterRequired;
				bool m_gatherRequired;

                int m_sourceNodeNumber;
			};

			class DistributedDataInfo
			{
			public:
				DistributedDataInfo(VariableDeclaration& declaration, int sourceNodeNumber);

			public:
				VariableDeclaration& getDeclaration();
                int getSourceNodeNumber();

			private:
				VariableDeclaration* m_pDeclaration;
                int m_sourceNodeNumber;
			};

			class IgnoredDataInfo
			{
			public:
				explicit IgnoredDataInfo(VariableDeclaration& declaration);

			public:
				VariableDeclaration& getDeclaration();

			private:
				VariableDeclaration* m_pDeclaration;
			};

			class ParallelLoopNestingInfo
			{
			public:
                ParallelLoopNestingInfo(ForStatement& forStatement, bool isBlock, std::vector<int> blockWidths, std::vector<int> iterationsCount);

			public:
				ForStatement& getForStatement();
                bool isBlock();
                std::vector<int> getBlockWidths();
                std::vector<int> getIterationsCount();

			private:
				ForStatement* m_pForStatement;
                bool m_isBlock;
                std::vector<int> m_blockWidths;
                std::vector<int> m_iterationsCount;
			};

			class DataDistributionAnalizer: public OPS::NonCopyableMix, public OPS::NonAssignableMix
			{
			public:
				typedef std::list<SingleAccessAreaInfo>    SingleAccessAreaInfoContainer;
				typedef std::list<DistributionAreaInfo>    DistributionAreaInfoContainer;
				typedef std::list<DistributedDataInfo>     DistributedDataInfoContainer;
				typedef std::list<IgnoredDataInfo>         IgnoredDataInfoContainer;
				typedef std::list<ParallelLoopNestingInfo> ParallelLoopNestingInfoContainer;

				typedef std::list<std::string> StringContainer;

			public:
				explicit DataDistributionAnalizer(BlockStatement& blockStatement);

			public:
				bool analize();

				DistributedDataInfoContainer getDistributedDataInfoContainer();
				IgnoredDataInfoContainer getIgnoredDataInfoContainer();

				SingleAccessAreaInfoContainer getSingleAccessAreaInfoContainer();
				DistributionAreaInfoContainer getDistributionAreaInfoContainer();
				ParallelLoopNestingInfoContainer getParallelLoopNestingInfoContainer();

				StringContainer getErrors();

			private:
				BlockStatement& m_blockStatement;
                std::tr1::shared_ptr<OPS::Montego::OccurrenceContainer> m_spOccurenceContainer;
                std::tr1::shared_ptr<OPS::Montego::AliasInterface> m_spAliasInterface;

				//DependenceGraph  m_dependenceGraph;
				ControlFlowGraph m_controlFlowGraph;

				SingleAccessAreaInfoContainer    m_singleAccessAreaInfoContainer;
				DistributionAreaInfoContainer    m_distributionAreaInfoContainer;
				DistributedDataInfoContainer     m_distributedDataInfoContainer;
				IgnoredDataInfoContainer         m_ignoredDataInfoContainer;
				ParallelLoopNestingInfoContainer m_parallelLoopNestingInfoContainer;

				StringContainer m_errors;
			};
		}
	}
}

#endif // DATA_DISTRIBUTION_ANALIZER_H
