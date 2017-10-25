#ifndef MPI_HELPER_DEEP_WALKERS_H
#define MPI_HELPER_DEEP_WALKERS_H

#include "Transforms/MPI/Utils/MPIHelper.h"
#include "Reprise/Reprise.h"
#include "Reprise/Service/DeepWalker.h"

namespace OPS
{
	namespace Transforms
	{
		namespace MPIProducer
		{
			using OPS::Reprise::Service::DeepWalker;
			using OPS::Reprise::SubroutineDeclaration;
			using OPS::Reprise::VariableDeclaration;
            using OPS::Reprise::BlockStatement;

			class RankDeclarationFinderDeepWalker: public DeepWalker
			{
			public:
				RankDeclarationFinderDeepWalker(): DeepWalker(), m_pRankDeclaration(NULL)
				{
				}

				void visit(VariableDeclaration& variableDeclaration)
				{
					if(variableDeclaration.hasNote(MPIHelper::RANK_NOTE_NAME) && variableDeclaration.getNote(MPIHelper::RANK_NOTE_NAME).getBool())
					{
						m_pRankDeclaration = &variableDeclaration;
					}
				}

				VariableDeclaration* getRankDeclaration()
				{
					return m_pRankDeclaration;
				}

			private:
				VariableDeclaration* m_pRankDeclaration;
			};

			class SizeDeclarationFinderDeepWalker: public DeepWalker
			{
			public:
				SizeDeclarationFinderDeepWalker(): DeepWalker(), m_pSizeDeclaration(NULL)
				{
				}

				void visit(VariableDeclaration& variableDeclaration)
				{
					if(variableDeclaration.hasNote(MPIHelper::SIZE_NOTE_NAME) && variableDeclaration.getNote(MPIHelper::SIZE_NOTE_NAME).getBool())
					{
						m_pSizeDeclaration = &variableDeclaration;
					}
				}

				VariableDeclaration* getSizeDeclaration()
				{
					return m_pSizeDeclaration;
				}

			private:
				VariableDeclaration* m_pSizeDeclaration;
			};

            class GlobalsInitBlockFinderDeepWalket: public DeepWalker
            {
            public:
                GlobalsInitBlockFinderDeepWalket(): DeepWalker(), m_pBlockStatement(NULL)
                {
                }

                void visit(BlockStatement& blockStatement)
                {
                    if(blockStatement.hasNote(MPIHelper::INIT_BLOCK_NOTE_NAME) && blockStatement.getNote(MPIHelper::INIT_BLOCK_NOTE_NAME).getBool())
                    {
                        m_pBlockStatement = &blockStatement;
                        return;
                    }
                    DeepWalker::visit(blockStatement);
                }

                BlockStatement* getGlobalsInitBlock()
                {
                    return m_pBlockStatement;
                }

            private:
                BlockStatement* m_pBlockStatement;
            };
		}
	}
}

#endif // MPI_HELPER_DEEP_WALKERS_H
