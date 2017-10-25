#ifndef LOOP_SHARED_DEEP_WALKERS_H
#define LOOP_SHARED_DEEP_WALKERS_H

#include "Reprise/Service/DeepWalker.h"
#include "Reprise/ServiceFunctions.h"

namespace OPS
{
	namespace Shared
	{
		class AllLoopsInNestAreCanonisedDeepWalker: public OPS::Reprise::Service::DeepWalker
		{
		public:
			AllLoopsInNestAreCanonisedDeepWalker(): OPS::Reprise::Service::DeepWalker(),
				m_allLoopsInNestAreCanonised(true)
			{
			}

		public:
			bool isAllLoopsInNestAreCanonised()
			{
				return m_allLoopsInNestAreCanonised;
			}

			void visit(OPS::Reprise::ForStatement& forStatement)
			{
				if(OPS::Reprise::Editing::forHeaderIsCanonized(forStatement))	
				{
					OPS::Reprise::Service::DeepWalker::visit(forStatement);
				}
				else
				{
					m_allLoopsInNestAreCanonised = false;
				}
			}
		private:
			bool m_allLoopsInNestAreCanonised;
		};
	}
}

#endif		//	LOOP_SHARED_DEEP_WALKERS_H
