#ifndef FRAMES_I_PROFILER_BRIDGE_H_INCLUDED_
#define FRAMES_I_PROFILER_BRIDGE_H_INCLUDED_

#include "Analysis/Frames/IProfiler.h"
#include "Analysis/Profiler/Profiler.h"

namespace OPS 
{
	namespace Analysis 
	{
		namespace Frames 
		{
			using OPS::Profiling::Profiler;
			using OPS::Reprise::RepriseBase;
			using std::tr1::shared_ptr;
            using OPS::Reprise::ForStatement;

            class ProfilerBridge : public IProfiler
			{
			public:
                ProfilerBridge();
                ProfilerBridge(shared_ptr<Profiler> spProfiler);

                virtual long_long_t GetSizeCommand(StatementBase&, bool isOnlyForHeaders = false);
                virtual long_long_t GetSizeData(StatementBase&, bool isOnlyForHeaders = false);

                virtual long_long_t GetSizeData(ForStatement&, const std::map<const ForStatement*, int>&);

			private:
                shared_ptr<Profiler> m_spProfiler;
                bool m_analizeOnlyDataSize;
			};
		}
	}
}

#endif // FRAMES_I_PROFILER_BRIDGE_H_INCLUDED_
