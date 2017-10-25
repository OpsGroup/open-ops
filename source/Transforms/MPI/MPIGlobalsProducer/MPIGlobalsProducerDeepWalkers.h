#ifndef MPI_GLOBALS_PRODUCER_C_DEEP_WALKERS_H
#define MPI_GLOBALS_PRODUCER_C_DEEP_WALKERS_H

#include "Reprise/Reprise.h"
#include "Reprise/Service/DeepWalker.h"

#include "Transforms/MPI/Utils/MPIHelper.h"

#include <list>

namespace OPS
{
    namespace Transforms
    {
        namespace MPIProducer
        {
            class ReturnFinderDeepWalker: public OPS::Reprise::Service::DeepWalker
            {
            public:
                typedef std::list<OPS::Reprise::ReturnStatement*> ReturnsList;

                ReturnFinderDeepWalker(): OPS::Reprise::Service::DeepWalker(), m_returnsList()
                {
                }

                ReturnsList getReturnsList()
                {
                    return m_returnsList;
                }

                void visit(OPS::Reprise::BlockStatement& blockStatement)
                {
                    OPS::Reprise::Service::DeepWalker::visit(blockStatement);
                }

                void visit(OPS::Reprise::ReturnStatement& returnStatement)
                {
                    m_returnsList.push_back(&returnStatement);
                }
            private:
                ReturnsList m_returnsList;
            };
        }
    }
}

#endif // MPI_GLOBALS_PRODUCER_C_DEEP_WALKERS_H
