#ifndef OPS_TRANSFORMATIOINS_LOCALIZATION_SCALAR_H_
#define OPS_TRANSFORMATIOINS_LOCALIZATION_SCALAR_H_


#include "Reprise/Reprise.h"

#include "Analysis/ComplexOccurrenceAnalysis/GrouppedOccurrences.h"

namespace OPS
{
    namespace Transforms
    {
        namespace Scalar
        {
            class LocalizationScalar
            {
            public:
                LocalizationScalar(Reprise::ProgramFragment& program, Analysis::OccurrencesByDeclarations& in, Analysis::OccurrencesByDeclarations& out);

                void makeTransform();
                Reprise::ProgramFragment& getProgram();
                Analysis::OccurrencesByDeclarations& getIn();
                Analysis::OccurrencesByDeclarations& getOut();
            private:
                Analysis::OccurrencesByDeclarations& m_inOccurrencesByDeclarations;
                Analysis::OccurrencesByDeclarations& m_outOccurrencesByDeclarations;
                Reprise::ProgramFragment& m_fragment;
            };
        }
    }
}
#endif						// OPS_TRANSFORMATIOINS_LOCALIZATION_SCALAR_H_
