#ifndef MPI_PRODUCER_FACTORY_H
#define MPI_PRODUCER_FACTORY_H

#include "Transforms/MPI/Utils/MPIHelper.h"

namespace OPS
{
	namespace Transforms
	{
		namespace MPIProducer
		{
			class IMPIGlobalsProducer;
			class IMPIRankProducer;
			class IMPISizeProducer;
			/**
				Abstract factory for creating MPIProducer objects
			*/
			class MPIProducerFactory
			{
			public:
				virtual MPIHelper* createMPIHelper(OPS::Reprise::TranslationUnit* pTranslationUnit) = 0;
				
				virtual MPIFunctionsHelper* createMPIFunctionHelper() = 0;
				virtual MPIBasicTypesHelper* createMPIBasicTypesHelper() = 0;
				virtual MPITypesHelper* createMPITypesHelper() = 0;
				virtual MPICommsHelper* createMPICommsHelper() = 0;
				virtual IMPIGlobalsProducer* createMPIGlobalsProducer(OPS::Reprise::SubroutineDeclaration* pEntryPoint) = 0;
				virtual IMPIRankProducer* createMPIRankProducer(OPS::Reprise::SubroutineDeclaration* pEntryPoint) = 0;
				virtual IMPISizeProducer* createMPISizeProducer(OPS::Reprise::SubroutineDeclaration* pEntryPoint) = 0;

				virtual ~MPIProducerFactory();
			};

			/**
				Abstract factory for creating MPIProducer objects for C output
			*/
			class MPIProducerCFactory: public MPIProducerFactory
			{
			public:
				virtual MPIHelper* createMPIHelper(OPS::Reprise::TranslationUnit* pTranslationUnit);
				
				virtual MPIFunctionsHelper* createMPIFunctionHelper();
				virtual MPIBasicTypesHelper* createMPIBasicTypesHelper();
				virtual MPITypesHelper* createMPITypesHelper();
				virtual MPICommsHelper* createMPICommsHelper();
				virtual IMPIGlobalsProducer* createMPIGlobalsProducer(OPS::Reprise::SubroutineDeclaration* pEntryPoint);
				virtual IMPIRankProducer* createMPIRankProducer(OPS::Reprise::SubroutineDeclaration* pEntryPoint);
				virtual IMPISizeProducer* createMPISizeProducer(OPS::Reprise::SubroutineDeclaration* pEntryPoint);
			};
		}
	}
}


#endif // MPI_PRODUCER_FACTORY_H
