#include "Analysis/Frames/FramesPartitioner.h"
#include "ProfilerFramesPartitioningWalker.h"
#include "Shared/Checks.h"
#include "OPS_Core/Localization.h"

namespace OPS 
{
	namespace Analysis 
	{
		namespace Frames 
		{
			using namespace OPS::Reprise;
			using namespace OPS::Shared;

			FramesPartitioner::FramesPartitioner(shared_ptr<IProfiler> spProfiler, long_long_t maxCommandSize, long_long_t maxDataSize)
				:m_spProfiler(spProfiler), m_maxCommandSize(maxCommandSize), m_maxDataSize(maxDataSize)
			{
			}

			FramesPartitioner::~FramesPartitioner()
			{
			}

            FramesGraph FramesPartitioner::MakePartition(ProgramFragment& fragment)
			{
                if (!checkConsistency(fragment))
				{
					throw OPS::Exception(_TL(
                        "Source fragment contains statements with unsupported types.",
                        "Исходный фрагмент содержит операторы с неподдерживаемыми типами"));
				}

				ProfilerFramesPartitioningWalker walker(m_spProfiler, m_maxCommandSize, m_maxDataSize);
                fragment.accept(walker);
				return walker.getResult();
			}

            bool FramesPartitioner::checkConsistency(ProgramFragment& fragment)
			{
				Checks::CompositionCheckObjects acceptableObjects;
				acceptableObjects << Checks::CompositionCheckObjects::CCOT_BlockStatement;
				acceptableObjects << Checks::CompositionCheckObjects::CCOT_ForStatement;
				acceptableObjects << Checks::CompositionCheckObjects::CCOT_WhileStatement;
				acceptableObjects << Checks::CompositionCheckObjects::CCOT_IfStatement;
				acceptableObjects << Checks::CompositionCheckObjects::CCOT_ExpressionStatement;

                return makeCompositionCheck(fragment, acceptableObjects);
			}
		}
	}
}
