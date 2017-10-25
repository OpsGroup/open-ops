#ifndef FRAMES_PARTITIONER_H_INCLUDED
#define FRAMES_PARTITIONER_H_INCLUDED

#include "Analysis/Frames/IFramesPartitioner.h"
#include "Analysis/Frames/IProfiler.h"
#include "OPS_Core/MemoryHelper.h"

namespace OPS 
{
	namespace Analysis 
	{
		namespace Frames 
		{
            using OPS::Reprise::ProgramFragment;
			using std::tr1::shared_ptr;

			class FramesPartitioner: public IFramesPartitioner
			{
			public:
				FramesPartitioner(shared_ptr<IProfiler> spProfiler, long_long_t maxCommandSize, long_long_t maxDataSize);
				~FramesPartitioner();

                virtual FramesGraph MakePartition(ProgramFragment& fragment);

			private:
                bool checkConsistency(ProgramFragment& statement);

			private:
				shared_ptr<IProfiler> m_spProfiler;
				long_long_t m_maxCommandSize;
				long_long_t m_maxDataSize;
			};
		}
	}
}
#endif // FRAMES_PARTITIONER_H_INCLUDED
