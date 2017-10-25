#ifndef I_FRAMES_PARTITIONER_H_INCLUDED
#define I_FRAMES_PARTITIONER_H_INCLUDED

#include "Analysis/Frames/FramesGraph.h"

namespace OPS 
{
	namespace Analysis 
	{
		namespace Frames 
		{
			using OPS::Reprise::StatementBase;
			
			class IFramesPartitioner
			{
			public:
				virtual ~IFramesPartitioner() {}
                virtual FramesGraph MakePartition(OPS::Reprise::ProgramFragment& fragment) = 0;

                inline FramesGraph MakePartition(Reprise::StatementBase& statement)
                {
                    Reprise::ProgramFragment fragment(statement);
                    return MakePartition(fragment);
                }
			};
		}
	}
}
#endif // I_FRAMES_PARTITIONER_H_INCLUDED
