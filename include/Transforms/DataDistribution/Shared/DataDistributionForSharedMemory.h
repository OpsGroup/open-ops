#ifndef DATA_DISTRIBUTION_FOR_SHARED_MEMORY_H
#define DATA_DISTRIBUTION_FOR_SHARED_MEMORY_H

#include <list>
#include <string>

#include "Reprise/Reprise.h"
#include "Transforms/ITransformation.h"
#include "Transforms/TransformArgs.h"
#include "Transforms/TransformsHub.h"

#include "Transforms/DataDistribution/Shared/BDParameters.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{

using std::string;
using OPS::Reprise::ProgramUnit;
using OPS::Reprise::BlockStatement;
using OPS::Reprise::VariableDeclaration;
using OPS::Reprise::StatementBase;
using OPS::Reprise::TypeBase;
using OPS::TransformationsHub::ArgumentValues;

class ReferenceTable;
struct ForStmtTree; 
class DataDistributionForSharedMemory : public OPS::TransformationsHub::TransformBase
{
public:		
    virtual void makeTransformImpl(OPS::Reprise::ProgramUnit* program, const OPS::TransformationsHub::ArgumentValues& params);
	void makeTransform(OPS::Reprise::ProgramUnit *program);
private:
    bool nest_loops(ReferenceTable& dim_table);

	void renameDistributionArraysInStatements(std::list<BDParameters> pragmas);
	void normalizeFreeCoefs(ForStmtTree& tree);
	void performBlockLoopNesting(ForStmtTree& tree);
	void changeIndexes(std::list<BDParameters> pragmas, OPS::Reprise::SubroutineDeclaration* func);
	void createRedistributionStatements(std::list<BDParameters> pragmas);

	OPS::Reprise::TranslationUnit* m_currentTranslationUnit;
};

}
}
}

#endif
