#include "Transforms/MPI/Utils/MPIProducerFactory.h"

#include "Transforms/MPI/MPIGlobalsProducer/MPIRankProducer.h"
#include "Transforms/MPI/MPIGlobalsProducer/MPISizeProducer.h"
#include "Transforms/MPI/MPIGlobalsProducer/MPIGlobalsProducer.h"

namespace OPS
{
	namespace Transforms
	{
		namespace MPIProducer
		{
			/*
				Implementation of MPIProducerFactory
			*/
			MPIProducerFactory::~MPIProducerFactory()
			{
			}

			/*
				Implementation of MPIProducerCFactory
			*/
			MPIHelper* MPIProducerCFactory::createMPIHelper(OPS::Reprise::TranslationUnit* pTranslationUnit)
			{
				OPS_ASSERT(pTranslationUnit != NULL);

				return new MPICHelper(*pTranslationUnit);
			}

			MPIFunctionsHelper* MPIProducerCFactory::createMPIFunctionHelper()
			{
				return &(MPIFunctionsCHelper::getInstance());
			}

			MPIBasicTypesHelper* MPIProducerCFactory::createMPIBasicTypesHelper()
			{
				return &(MPIBasicTypesCHelper::getInstance());
			}

			MPITypesHelper* MPIProducerCFactory::createMPITypesHelper()
			{
				return &(MPITypesCHelper::getInstance());
			}

			MPICommsHelper* MPIProducerCFactory::createMPICommsHelper()
			{
				return &(MPICommsCHelper::getInstance());
			}

			IMPIGlobalsProducer* MPIProducerCFactory::createMPIGlobalsProducer(OPS::Reprise::SubroutineDeclaration* pEntryPoint)
			{
				return new MPIGlobalsProducer(pEntryPoint);
			}

			IMPIRankProducer* MPIProducerCFactory::createMPIRankProducer(OPS::Reprise::SubroutineDeclaration* pEntryPoint)
			{
				return new MPIRankProducer(pEntryPoint);
			}

			IMPISizeProducer* MPIProducerCFactory::createMPISizeProducer(OPS::Reprise::SubroutineDeclaration* pEntryPoint)
			{
				return new MPISizeProducer(pEntryPoint);
			}
		}
	}
}
