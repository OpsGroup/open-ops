#ifndef FRAMES_I_PROFILER_H_INCLUDED_
#define FRAMES_I_PROFILER_H_INCLUDED_

#include "Reprise/Reprise.h"
#include "OPS_Core/MemoryHelper.h"

namespace OPS 
{
	namespace Analysis 
	{
		namespace Frames 
		{
            using OPS::Reprise::ForStatement;
			using OPS::Reprise::RepriseBase;
            using OPS::Reprise::StatementBase;

			class IProfiler
			{
			public:
				virtual ~IProfiler() {}

                virtual long_long_t GetSizeCommand(StatementBase&, bool isOnlyForHeaders = false) = 0;
                virtual long_long_t GetSizeData(StatementBase&, bool isOnlyForHeaders = false) = 0;

                virtual long_long_t GetSizeData(ForStatement&, const std::map<const ForStatement*, int>&) = 0;
			};
		}
	}
}

#endif // FRAMES_I_PROFILER_H_INCLUDED_
