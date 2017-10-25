#include "Transforms/TransformsHub.h"
#include "Reprise/Reprise.h"
#include "Reprise/ServiceFunctions.h"
#include "Shared/ExpressionHelpers.h"
#include <sstream>
#include <stdexcept>
#include "OPS_Core/Localization.h"

#include "FrontTransforms/ExpressionSimplifier.h"
#include "Transforms/Scalar/SubstitutionForward/SubstitutionForward.h"
#include "Transforms/Scalar/ConstantPropagation/ConstantPropagation.h"
//#include "Transforms/Scalar/SwitchToIf/SwitchToIf.h"
#include "Transforms/Scalar/ArithmeticOperatorExpansion.h"
#include "Transforms/Scalar/Silly/SillyTranslationUnit.h"
#include "Analysis/DeadDeclarations.h"

#include "Transforms/Loops/LoopDistribution/LoopDistribution.h"
#include "Transforms/Loops/LoopFusion/LoopFusion.h"
#include "Transforms/Loops/LoopNesting/LoopNesting.h"
#include "Transforms/Loops/StripMining/StripMining.h"
#include "Transforms/Loops/RecurrentLoops/RecurrentLoops.h"
#include "Transforms/Loops/LoopUnrolling/LoopUnrolling.h"
#include "Transforms/Loops/LoopFullUnrolling/LoopFullUnrolling.h"
//#include "Transforms/Loops/Var2Array/var2array.h"
#include "Transforms/Loops/IntToVector/IntToVector.h"
#include "Transforms/Loops/LoopHeaderRemoval/LoopHeaderRemoval.h"
#include "Transforms/Loops/LoopFragmentation/LoopFragmentation.h"
#include "Transforms/Loops/LoopCycleOffset/LoopCycleOffset.h"

#include "Transforms/If/IfDistribution/IfDistribution.h"
#include "Transforms/If/IfExtraction/IfExtraction.h"
#include "Transforms/If/IfSplitting/IfSplitting.h"

#include "Transforms/Statements/SwapStatements.h"

#include "Transforms/DataDistribution/BlockAffineDataDistribution.h"
#include "Transforms/DataDistribution/Shared/DataDistributionForSharedMemory.h"

#include "Transforms/Subroutines/Inlining.h"
#include "Transforms/Subroutines/InliningWithSubstitution.h"
#include "Transforms/Subroutines/FragmentToSubroutine.h"
#include "Transforms/Subroutines/SubroutineSplitting.h"

#include "Analysis/Montego/OccurrenceContainer.h"
#include "Analysis/Montego/AliasAnalysis/AliasInterface.h"
#include "Transforms/Declarations/RemoveDeadDeclarations.h"

using namespace OPS::Reprise;

#ifdef _DEBUG
#define TEST_TRANSFORMATIONS_ENABLED
#endif

// Начало чита
namespace OPS
{
namespace Montego
{
void apply4CanonicalTransforms(RepriseBase&);
}
}
// Конец чита

namespace OPS
{
namespace TransformationsHub
{

const ArgumentsInfo& TransformBase::getArgumentsInfo() const
{
	return m_argumentsInfo;
}

void TransformBase::makeTransform(ProgramUnit *program, const ArgumentValues &params)
{
    std::string message;
    if (!isApplicable(program, params, &message))
        throw OPS::RuntimeError(message);

    makeTransformImpl(program, params);
}

const std::string& InformerBase::getResult() const
{
	return m_result;
}

const std::string INVALID_ARGS_MESSAGE = _TL("Invalid arguments has been passed in transformation","Преобразованию были переданы неправильные аргументы");

class TransfExprSimplifier : public TransformBase
{
public:
	TransfExprSimplifier()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtExpr));
	}

    virtual void makeTransformImpl(OPS::Reprise::ProgramUnit* program, const ArgumentValues& params)
	{
		//OPSAssert(m_argumentsInfo.validate(params), INVALID_ARGS_MESSAGE);
		OPS_UNUSED(program);

		OPS::ExpressionSimplifier::Simplifier simplifier;
		simplifier.simplifyAndReplace(*params[0].getAsExpr());
	}
};

class TransfSubstForw : public TransformBase
{
public:
	TransfSubstForw()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtBlock, _TL("Target block","Целевой блок")));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::ExprAny, _TL("Destination expression","Выражение-назначение")));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::ExprAny, _TL("Source expression","Выражение-источник")));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, "Change Left Part Ass Stmt"));
	}

    virtual void makeTransformImpl(OPS::Reprise::ProgramUnit* program, const ArgumentValues& args)
	{
		OPS_UNUSED(program);

		OPS::Transforms::Scalar::makeSubstitutionForward(*args[0].getAsBlock(), *args[1].getAsExprNode(),
						ReprisePtr<ExpressionBase>(args[2].getAsExprNode()), args[3].getAsBool());
	}
};

class TransfLoopDistribution : public TransformBase
{
public:
	TransfLoopDistribution()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtFor));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, _TL("Use 'Var To Array'","Использовать 'Var To Array'")));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, _TL("Use 'Temp Arrays'","Использовать 'Temp Arrays'")));
	}

    virtual void makeTransformImpl(OPS::Reprise::ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		//OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		OPS::Reprise::ForStatement* targetLoop = args[0].getAsFor();
		const bool varToArray = args[1].getAsBool();
		const bool tempArrays = args[2].getAsBool();

		if (!OPS::Transforms::Loops::LoopDistribution(targetLoop, varToArray, tempArrays))
		{
			throw OPS::RuntimeError(_TL("Could not distribute loop.","Не удалось произвести разрезание цикла."));
		}
	}
};

class TransfIntToVector : public TransformBase
{
public:
	TransfIntToVector()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtFor));
	}

    virtual void makeTransformImpl(ProgramUnit *program, const ArgumentValues &params)
	{
		OPS::Reprise::ForStatement* targetLoop = params[0].getAsFor();

		if (!OPS::Transforms::Loops::IntToVector(targetLoop))
		{
			throw OPS::RuntimeError("IntToVector failed");
		}
	}
};

class TransfLoopNesting : public TransformBase
{
public:
	TransfLoopNesting()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtFor));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Int, "Nesting param (>=2)"));
		m_argumentsInfo.back().defaultValue.setInt(2);
        m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, "Outer loop"));
        m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, "Native counter"));
        m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, "Generate tail"));
        m_argumentsInfo.back().defaultValue.setBool(true);
	}

    bool isApplicable(ProgramUnit *, const ArgumentValues &args, string *message)
    {
        using namespace OPS::Reprise::Editing;
        using namespace OPS::Shared::ExpressionHelpers;

        OPS::Reprise::ForStatement* stmtFor = args[0].getAsFor();
        const int loop_nesting_param = args[1].getAsInt();

        if (loop_nesting_param < 2)
        {
            if (message) *message = "Loop Nesting parameter must be greater or equal 2.";
            return false;
        }
        if (!OPS::Transforms::Loops::canApplyLoopNestingTo(*stmtFor))
        {
            if (message) *message = "Loop Nesting can't transform loop.";
            return false;
        }
        return true;
    }

    virtual void makeTransformImpl(OPS::Reprise::ProgramUnit* program, const ArgumentValues& args)
	{
		using namespace OPS::Reprise::Editing;
		using namespace OPS::Shared::ExpressionHelpers;

		OPS::Reprise::ForStatement* stmtFor = args[0].getAsFor();
		const int loop_nesting_param = args[1].getAsInt();

        OPS::Transforms::Loops::makeLoopNesting(*stmtFor, loop_nesting_param, args[2].getAsBool(), args[3].getAsBool(), false, args[4].getAsBool());
	}
};

class TransfRecurrentLoops : public TransformBase
{
public:
	TransfRecurrentLoops()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtFor));
	}

    virtual void makeTransformImpl(ProgramUnit*, const ArgumentValues& args)
	{
		//OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		ForStatement* targetFor = args[0].getAsFor();

		OPS::Transforms::Loops::MakeRLT(targetFor);
	}
};

class TransfStripMining : public TransformBase
{
public:
	TransfStripMining()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtFor));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Int, "Mining param (>=2)"));
		m_argumentsInfo.back().defaultValue.setInt(2);
	}

    bool isApplicable(ProgramUnit*, const ArgumentValues &args, string *message)
    {
        using namespace OPS::Reprise::Editing;
        using namespace OPS::Shared::ExpressionHelpers;

        OPS::Reprise::ForStatement* stmtFor = args[0].getAsFor();
        const int loop_nesting_param = args[1].getAsInt();

        if (loop_nesting_param < 2)
        {
            if (message) *message = "Strip Mining parameter must be greater or equal 2.";
            return false;
        }

        if (!forIsBasic(*stmtFor))
        {
            if (message) *message = "Strip Mining can't get counter from loop.";
            return false;
        }

        ReferenceExpression& i = getBasicForCounter(*stmtFor);
        IntegerHelper c(i.getResultType()->cast_to<BasicType>());
        ReprisePtr<ExpressionBase> h(&c(loop_nesting_param));

        if (!OPS::Transforms::Loops::canApplyStripMiningTo(*stmtFor, h))
        {
            if (message) *message = "Strip Mining can't transform loop.";
        }
        return true;
    }

    virtual void makeTransformImpl(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		using namespace OPS::Reprise::Editing;
		using namespace OPS::Shared::ExpressionHelpers;

		OPS::Reprise::ForStatement* stmtFor = args[0].getAsFor();
		const int loop_nesting_param = args[1].getAsInt();

		ReferenceExpression& i = getBasicForCounter(*stmtFor);
        IntegerHelper c(i.getResultType()->cast_to<BasicType>());
		ReprisePtr<ExpressionBase> h(&c(loop_nesting_param));

		OPS::Transforms::Loops::makeStripMining(*stmtFor, h);
	}
};

class TransfLoopHeaderRemoval : public TransformBase
{
	public:
	TransfLoopHeaderRemoval()
	{
        m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtFor, _TL("Loop","Цикл")));
	}

    bool isApplicable(ProgramUnit *program, const ArgumentValues &args, string *message)
    {
        OPS::Reprise::ForStatement* stmtFor = args[0].getAsFor();
        if (!OPS::Transforms::Loops::canApplyLoopHeaderRemovalTo(*stmtFor))
        {
            if (message) *message = "Cannot remove the header.";
            return false;
        }
        return true;
    }

    virtual void makeTransformImpl(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
        OPS::Reprise::ForStatement* stmtFor = args[0].getAsFor();
		OPS::Transforms::Loops::makeLoopHeaderRemoval(*stmtFor);
	}
};

class TransfLoopFragmentation : public TransformBase
{
public:
	TransfLoopFragmentation()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtFor));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::String, "Block sizes (,)"));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, "Nesting check"));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, "Distribution check"));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, "Interchange check"));
	}

	bool isApplicable(ProgramUnit *program, const ArgumentValues &args, string *message)
	{
		return true;
	}

protected:

	bool splitString(const std::string& str, std::vector<int>& result)
	{
		std::list<string> splits = OPS::Strings::split(str, ",");
		result.reserve(splits.size());
		for(std::list<string>::iterator it = splits.begin(); it != splits.end(); ++it)
		{
			result.push_back(0);
			if (!OPS::Strings::fetch(*it, result.back()))
				return false;
		}
		return true;
	}

	virtual void makeTransformImpl(ProgramUnit *program, const ArgumentValues &params)
	{
		ForStatement* loop = params[0].getAsFor();
		std::vector<int> fragmentationParams;
		if (!splitString(params[1].getAsString(), fragmentationParams))
		{
			throw OPS::RuntimeError("Block sizes must be comma separated list of integers");
		}

		bool	nestingCheck = params[2].getAsBool(),
				distributionCheck = params[3].getAsBool(),
				interchangeCheck = params[4].getAsBool();

		OPS::Transforms::Loops::loopFragmentation(*loop, fragmentationParams, nestingCheck, distributionCheck, interchangeCheck);
	}
};

/*
class TransfSwitchToIf : public TransformBase
{
public:
	TransfSwitchToIf()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtSwitch));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& params)
	{
		OPS_UNUSED(program);
		//OPSAssert(m_argumentsInfo.validate(params), INVALID_ARGS_MESSAGE);
		SwitchStatement* switchStmt = params[0].getAsSwitch();

		OPS::Transforms::Scalar::makeIfFromSwitch(switchStmt);
	}
};
*/

class TransfArithmeticOperatorExpansion : public TransformBase
{
public:
	TransfArithmeticOperatorExpansion()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtAny));
	}

    virtual void makeTransformImpl(OPS::Reprise::ProgramUnit *program, const ArgumentValues &params)
	{
		OPS_UNUSED(program);

		OPS::Transforms::Scalar::expandArithmeticOperators(*params[0].getAsStatement());
	}
};

class TransfConstantPropagation : public TransformBase
{
public:
	TransfConstantPropagation()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtBlock));
	}

    virtual void makeTransformImpl(ProgramUnit* /*program*/, const ArgumentValues &params)
	{
		BlockStatement* block = params[0].getAsBlock();
		OPS::Transforms::Scalar::makeConstantPropagation(*block);
	}
};

#ifdef TEST_TRANSFORMATIONS_ENABLED

class TransfExceptionGenerator : public TransformBase
{
public:
	TransfExceptionGenerator()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtAny));
	}

	static void makeStackOverflow()
	{
		char buffer[128];
		buffer[0] = 1;
		buffer[1] = buffer[0];
		makeStackOverflow();
	}

	static void makeAccessViolation()
	{
		int* ptr = 0;
		*ptr = 0;
	}

    virtual void makeTransformImpl(ProgramUnit* /*program*/, const ArgumentValues& /*args*/)
	{
		switch(rand() % 6)
		{
		case 0: return; // normal execution
		case 1: throw OPS::RuntimeError("Transformation failed");
		case 2: throw new OPS::Exception("by ptr");
		case 3: throw std::runtime_error("std exception");
		case 4: throw int(10);
		//case 5: makeStackOverflow(); break;
		default:
			makeAccessViolation(); break;
		}
	}
};

class TransfCorruptor : public TransformBase
{
public:
	TransfCorruptor()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtFor));
	}

    virtual void makeTransformImpl(ProgramUnit* , const ArgumentValues& args)
	{
		BlockStatement& forBody = args[0].getAsFor()->getBody();
		if (forBody.getFirst().isValid())
		{
			forBody.erase(forBody.getFirst());
		}
	}
};

class TransfNothing : public TransformBase
{
public:
	TransfNothing()
	{
	}

	virtual void makeTransformImpl(ProgramUnit*, const ArgumentValues&)
	{
		// does nothing
	}
};

class TransfSilly : public TransformBase
{
public:
	virtual void makeTransformImpl(ProgramUnit* pProgramUnit, const ArgumentValues&)
	{
		const int cnUnits = pProgramUnit->getUnitCount();
		for (int i = 0; i < cnUnits; ++ i)
		{
			TranslationUnit& rTranslationUnit = pProgramUnit->getUnit(i);
			Transforms::Scalar::applySillyToTranslationUnit(rTranslationUnit);
		}
	}
};

#endif

class TransfAliasAnalysis : public InformerBase
{
public:
	TransfAliasAnalysis()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::ExprVar, "First reference"));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::ExprVar, "Second reference"));
	}

    virtual void makeTransformImpl(OPS::Reprise::ProgramUnit *program, const ArgumentValues &params)
	{
		OPS::Montego::apply4CanonicalTransforms(*program);		
		OPS::Montego::OccurrenceContainer occurs(*program);
        unique_ptr<OPS::Montego::AliasInterface> ai(OPS::Montego::AliasInterface::create(*program, occurs));
        ai->runAliasAnalysis();
	
        bool isAlias = ai->isAlias(params[0].getAsData()->getReference(),params[1].getAsData()->getReference());
		if (isAlias==true)
			m_result = "May alias";
		else
			m_result = "No alias";
	}
};

class TransfSwapStatements : public TransformBase
{
public:
	TransfSwapStatements()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtAny, _TL("First statement","Первый оператор")));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtAny, _TL("Second statement","Второй оператор")));
	}

    virtual void makeTransformImpl(ProgramUnit* program, const ArgumentValues& args)
	{
		OPS_UNUSED(program);

		if (!OPS::Transforms::trySwapStmts(args[0].getAsStatement(), args[1].getAsStatement()))
		{
			throw OPS::RuntimeError("Failed to swap statements");
		}
	}
};

class TransfLoopUnrolling : public TransformBase
{
public:
	TransfLoopUnrolling()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtFor));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Int, _TL("Unrolling param","Параметр раскрутки")));
		m_argumentsInfo.back().defaultValue.setInt(2);
	}

    bool isApplicable(ProgramUnit *program, const ArgumentValues &args, string *message)
    {
        if (!OPS::Transforms::Loops::canApplyLoopUnrollingTo(*args[0].getAsFor(), args[1].getAsInt()))
        {
            if (message) *message = "Loop is not in canonised form.";
            return false;
        }
        return true;
    }

    virtual void makeTransformImpl(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		//OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);

		ForStatement* targetLoop = args[0].getAsFor();
		qword unrollParam = args[1].getAsInt();

		OPS::Transforms::Loops::makeLoopUnrolling(*targetLoop, unrollParam);
	}
};

class TransfLoopFullUnrolling : public TransformBase
{
public:
	TransfLoopFullUnrolling()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtFor));
	}

    bool isApplicable(ProgramUnit *program, const ArgumentValues &args, string *message)
    {
        if (!OPS::Transforms::Loops::canApplyLoopFullUnrollingTo(*args[0].getAsFor()))
        {
            if (message) *message = "Loop is not in canonised form\nor number of iterations is unknown";
            return false;
        }
        return true;
    }

    virtual void makeTransformImpl(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		//OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		ForStatement* targetLoop = args[0].getAsFor();
		OPS::Transforms::Loops::makeLoopFullUnrolling(*targetLoop);
	}
};

class TransfLoopFusion : public TransformBase
{
public:
	TransfLoopFusion()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtFor, _TL("First loop","Первый цикл")));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtFor, _TL("Second loop","Второй цикл")));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, _TL("Use 'Var To Array'","Использовать 'Var To Array'")));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, _TL("Use 'Temp Arrays'","Использовать 'Temp Arrays'")));
	}

    virtual void makeTransformImpl(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		//OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		ForStatement* loop1 = args[0].getAsFor();
		ForStatement* loop2 = args[1].getAsFor();
		bool varToArray = args[2].getAsBool();
		bool tempArrays = args[3].getAsBool();

		if (!OPS::Transforms::Loops::LoopFusion(loop1, loop2, varToArray, tempArrays))
		{
			throw OPS::Exception("Loops fusion is not possible");
		}
	}
};


class TransfAliasCanonicalForm : public TransformBase
{
public:
	TransfAliasCanonicalForm()
	{
	}

    virtual void makeTransformImpl(ProgramUnit* program, const ArgumentValues& /*args*/)
	{
		OPS::Montego::apply4CanonicalTransforms(*program);
	}
};

class TransfRemoveUnusedDeclarations : public TransformBase
{
public:

    virtual void makeTransformImpl(ProgramUnit *program, const ArgumentValues &params)
    {
        OPS::Transformations::Declarations::removeDeadDeclarations(*program);
    }
};


#if 0
class TransfVar2Array : public TransformBase
{
public:
	TransfVar2Array()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtFor));
	}

	virtual void makeTransform(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		//OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor* stmtFor = args[0].getAsFor();

		try
		{
			MakeVar2ArrayTransform(stmtFor);
			return true;
		}
		catch (const OPS::Exception& except)
		{
			m_lastError = except.getMessage();
			return false;
		}
	}
};

class TransfCanonizationExpr : public TransformAdapter
{
public:
	TransfCanonizationExpr() : TransformAdapter("Expr Canonization") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_EXPR));
		m_argumentsInfo.push_back(ArgumentInfo(AT_BOOL, _TL("Simplify","Упростить")));
	}

	virtual voidl makeTransform(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);

		StmtExpr* stmtAssign = args[0].getAsExpr();
		bool bSimplify = args[1].getAsBool();

		try
		{
			if (bSimplify)
				makeExprCanonizationWSimplif(stmtAssign);
			else
				makeExprCanonization(stmtAssign);
			return true;
		}
		catch (const OPS::Exception& except)
		{
			m_lastError = except.getMessage();
			return false;
		}
	}
};

class TransfCanonizationAssign : public TransformAdapter
{
public:
	TransfCanonizationAssign() : TransformAdapter("Assign Canonization") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_ASSIGN));
	}

	virtual void makeTransform(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtExpr* stmtAssign = args[0].getAsAssign();

		try
		{
			if (isNeedAssignCanonization(stmtAssign))
			{
				makeAssignCanonization(stmtAssign);
				return true;
			}
			else
			{
				m_lastError = _TL("Assign statement already canonized.","Оператор присваивания уже канонизирован.");
				return false;
			}
		}
		catch (const OPS::Exception& except)
		{
			m_lastError = except.getMessage().c_str();
			return false;
		}
	}
};

class TransfCanonizationFor : public TransformAdapter
{
public:
	TransfCanonizationFor() : TransformAdapter("For Canonization") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
		m_argumentsInfo.addListArg(_TL("Initial value","Начальное значение"), _TL("From 0","С 0"), _TL("From 1","С 1"));
	}

	virtual void makeTransform(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor* stmtFor = args[0].getAsFor();

		/*
		if (stmtFor == 0)
		{
			m_lastError = "'for' statement not selected.";
			return false;
		}
		*/

		try
		{
			int canonize_from = args[1].getAsInt(); 
			if (isNeedForCanonization(stmtFor, canonize_from))
			{
				makeForCanonization(stmtFor, canonize_from);
				return true;
			}
			else
			{
				m_lastError = _TL("'For' statement already canonized.","Цикл for уже канонизирован.");
			}
			return false;
		}
		catch (const OPS::Exception& except)
		{
			m_lastError = except.getMessage().c_str();
			return false;
		}
	}
};

class TransfTempArrays : public TransformAdapter
{
public:
	TransfTempArrays() : TransformAdapter("Temp Arrays") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_BLOCK));
	}

	virtual void makeTransform(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		Block* stmtBlock = args[0].getAsBlock();

		try
		{
			MakeTempArraysTransform(*stmtBlock);
			return true;
		}
		catch (const OPS::Exception& except)
		{
			m_lastError = except.getMessage().c_str();
			return false;
		}
	}
};


class TransfSynchConstraints : public CodeGenAdapter
{
public:
	TransfSynchConstraints() : CodeGenAdapter("Synch Constraints") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
		m_argumentsInfo.push_back(ArgumentInfo(AT_INT, _TL("Parts count","Количество частей")));
		m_argumentsInfo.back().defaultValue.setInt(2);
		m_argumentsInfo.push_back(ArgumentInfo(AT_BOOL, _TL("Barrier after loop","Барьер после цикла")));
		m_argumentsInfo.back().defaultValue.setBool(true);
	}

	virtual void makeTransform(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);

		StmtFor* stmtFor = args[0].getAsFor();

		try
		{
			SynchConstraints synchCons;
			synchCons.generateSynchConstraints(stmtFor, args[1].getAsInt(), args[2].getAsBool());
			std::stringstream	os;
			synchCons.print(os);
			m_generatedCode = os.str();
			return true;
		}
		catch(const OPS::Exception& error)
		{
			m_lastError = error.getMessage().c_str();
			return false;
		}
	}
};

#if 0
class TransfMPIProducer : public IOpsTransform
{
public:
	TransfMPIProducer() : IOpsTransform("MPI Producer") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
	}

	DECLARE_TRANF_CATEGORY(TC_BACK_END);
	FACTORY_IMPL(TransfMPIProducer);

	virtual void makeTransform(const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor *f = args[0].getAsFor();
		
		/*
		if (f == 0)
		{
			m_lastError = "For loop doesn't selected.";
			return false;
		}
		*/

		NameSubProc &subProc = *m_program->getGlobals().getSubProc("main");
		NameVar &var = *subProc.getBody().getNamespace().getVar("sum");

		try
		{
			MPIProducer producer(&subProc, f, &var);
			string		mpiOutFilename = string(getenv("TEMP")) + "\\mpi_out.c";
			ofstream	mpiOut(mpiOutFilename.c_str());
			producer.ProduceMPI(mpiOut);
			mpiOut.close();
			WinExec(("notepad.exe " + mpiOutFilename).c_str(), SW_NORMAL);
		}
		catch(const Exception& error)
		{
			m_lastError = "MPI producer error: " + error.getMessage();
		}
		return false;
	}
	
};

class TransfMPIProducerEx : public IOpsTransform
{
public:
	TransfMPIProducerEx() : IOpsTransform("MPI Producer Ex") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
	}

	DECLARE_TRANF_CATEGORY(TC_BACK_END);
	FACTORY_IMPL(TransfMPIProducerEx);

	virtual std::string getTipsName() const { return "MPI ProducerEx"; }

	virtual void makeTransform(const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor *f = args[0].getAsFor();
		/*
		if (f == 0)
		{
			m_lastError = "For loop doesn't selected.";
			return false;
		}
		*/

		NameSubProc &subProc = *m_program->getGlobals().getSubProc("main");

		try
		{
			MPIProducerEx	producer (&subProc, f);
			string			mpiOutFilename = string(getenv("TEMP")) + "\\mpi_out.c";
			ofstream		mpiOut(mpiOutFilename.c_str());
			producer.ProduceMPI(mpiOut);
			mpiOut.close();
			WinExec(("notepad.exe " + mpiOutFilename).c_str(), SW_NORMAL);
		}
		catch(const Exception& error)
		{
			m_lastError = "MPI producer error: " + error.getMessage();
		}

		return false;
	}
	
};

//Михаил Гуревич
class TransfMPIProducerOutDep : public IOpsTransform
{
public:
	TransfMPIProducerOutDep() : IOpsTransform("MPI Producer Out Dep") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
	}

	DECLARE_TRANF_CATEGORY(TC_BACK_END);
	FACTORY_IMPL(TransfMPIProducerOutDep);

	virtual std::string getTipsName() const { return "MPI ProducerOutDep"; }

	virtual void makeTransform(const ArgumentValues& args)
	{	
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor *f = args[0].getAsFor();
		/*
		if (f == 0)
		{
			m_lastError = "For loop doesn't selected.";
			return false;
		}
		*/

		NameSubProc &subProc = *m_program->getGlobals().getSubProc("main");

		try
		{
			MPIProducerOutDep	producer(&subProc, f);
			string				mpiOutFilename = string(getenv("TEMP")) + "\\mpi_out.c";
			ofstream			mpiOut(mpiOutFilename.c_str());
			producer.ProduceMPI(mpiOut);
			mpiOut.close();
			WinExec(("notepad.exe " + mpiOutFilename).c_str(), SW_NORMAL);
		}
		catch(const Exception& error)
		{
			m_lastError = "MPI producer error: " + error.getMessage();
		}

		return false;
	}
	
};
class TransfMPIProducerOutDepEx : public IOpsTransform
{
public:
	TransfMPIProducerOutDepEx() : IOpsTransform("MPI Producer Out Dep Ex") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
	}

	DECLARE_TRANF_CATEGORY(TC_BACK_END);
	FACTORY_IMPL(TransfMPIProducerOutDepEx);

	virtual std::string getTipsName() const { return "MPI ProducerOutDepEx"; }

	virtual void makeTransform(const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor *f = args[0].getAsFor();
		/*
		if (f == 0)
		{
			m_lastError = "For loop doesn't selected.";
			return false;
		}
		*/

		NameSubProc &subProc = *m_program->getGlobals().getSubProc("main");

		try
		{
			MPIProducerOutDepEx producer(&subProc, f);
			string				mpiOutFilename = string(getenv("TEMP")) + "\\mpi_out.c";
			ofstream			mpiOut(mpiOutFilename.c_str());
			producer.ProduceMPI(mpiOut);
			mpiOut.close();
			WinExec(("notepad.exe " + mpiOutFilename).c_str(), SW_NORMAL);
		}
		catch(const Exception& error)
		{
			m_lastError = "MPI producer error: " + error.getMessage();
		}

		return false;
	}
};
//Михаил Гуревич
#endif

class TransfOMPProducer : public CodeGenAdapter
{
public:
	TransfOMPProducer() : CodeGenAdapter("OpenMP Producer") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_INT, "Max parallel nesting"));
		m_argumentsInfo.back().defaultValue.setInt(1);	
		m_argumentsInfo.push_back(ArgumentInfo(AT_INT, _TL("Threads number","Количество нитей")));
		m_argumentsInfo.back().defaultValue.setInt(5);
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		try
		{
			OMPProducer producer (program, args[0].getAsInt(), args[1].getAsInt());
			std::stringstream ompOut;
			producer.ProduceOMP(ompOut);
			m_generatedCode = ompOut.str();
			return true;
		}
		catch(const OPS::Exception& error)
		{
			m_lastError = error.getMessage();
		}
		return false;
	}

};

class TransfOMPProducerDep : public CodeGenAdapter
{
public:
	TransfOMPProducerDep() : CodeGenAdapter("OpenMP Dep Producer") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
		m_argumentsInfo.push_back(ArgumentInfo(AT_INT, _TL("Threads number","Количество нитей")));
		m_argumentsInfo.back().defaultValue.setInt(2);
		m_argumentsInfo.push_back(ArgumentInfo(AT_BOOL, _TL("Use notify areas","Использовать области уведомлений")));
		m_argumentsInfo.back().defaultValue.setBool(false);
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		
		StmtFor* stmtFor = args[0].getAsFor();

		try
		{
			OMPProducerDep producer (stmtFor, args[1].getAsInt(), true , "sync_", args[2].getAsBool());
			std::stringstream ompOut;
			producer.ProduceOMP(ompOut);
			m_generatedCode = ompOut.str();
			return true;
		}
		catch(const OPS::Exception& error)
		{
			m_lastError = "OpenMP producer error: " + error.getMessage();
		}
		return false;
	}

};

// Геннадий Хачкинаев

class TransfHDL : public CodeGenAdapter
{
public:
	TransfHDL() : CodeGenAdapter("HDL Producer") { 
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		HDL::ProgramConstPtr ptr(program);
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		try
		{
			HDL::Generator generator(ptr);
			HDL::BackEndVHDL backEndVHDL;
			std::stringstream ompOut;
			backEndVHDL.convert(generator, ompOut);
			m_generatedCode = ompOut.str();
		}
		catch(const OPS::Exception& error)
		{
			m_lastError = "HDL producer error: " + error.getMessage();
			ptr.release();
			return false;
		}
		ptr.release();
		return true;
	}

};

class TransfForGe : public CodeGenAdapter
{
public:
	TransfForGe() : CodeGenAdapter("Fortran Producer") { 
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		HDL::ProgramConstPtr ptr(program);
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		try
		{
			ForGe::Generator(ForGe::Generator::F77).generate(program,"fortran\\");
			LPITEMIDLIST items;
			SFGAOF flags;
			wchar_t buff[4096];
			::GetCurrentDirectoryW(4096, buff);
			std::wstring currentFolder(buff); 
			SHParseDisplayName((currentFolder + L"\\fortran\\").c_str(), 0, &items, SFGAO_FILESYSTEM, &flags);
			SHOpenFolderAndSelectItems(items, 0, 0, 0);
			m_generatedCode = "";
		}
		catch(const std::exception& error)
		{
			m_lastError = std::string("Fortran producer error: ") + error.what();
			ptr.release();
			return false;
		}
		ptr.release();
		return true;
	}

};

class TransfPartitioningPipelining : public CodeGenAdapter
{
public:
	TransfPartitioningPipelining() : CodeGenAdapter("Partitioning Pipelining") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_BLOCK));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		HDL::ProgramConstPtr ptr(program);
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		try
		{
			std::stringstream ompOut;
			Frames::ResourceTable::init();
			//Frames::ResourceTable::Cout(std::cout);

			Frames::TFrames frames = Frames::partitioningBlock(*args[0].getAsBlock());

			int i = 1;
			for(Frames::TFrames::iterator it1 = frames.begin(); it1 != frames.end(); ++it1)
			{
				for (Frames::Frame::TNodes::iterator it2 = (*it1).m_Nodes.begin(); it2 != (*it1).m_Nodes.end(); ++it2)
				{
					ompOut<<i<<"frame)";
					(*it2)->C_Out(ompOut);
					ompOut<<endl;
				}
				i++;
				ompOut<<endl;
			}

			m_generatedCode = ompOut.str();
		}
		catch(const OPS::Exception& error)
		{
			m_lastError = "PartitioningProgForPipelining error: " + error.getMessage();
			ptr.release();
			return false;
		}
		ptr.release();
		return true;
	}

};


#if 0
class TransfRuntimePrecheck : public IOpsTransform
{
public:
	TransfRuntimePrecheck() : IOpsTransform("Runtime Precheck") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
	}

	DECLARE_TRANF_CATEGORY(TC_ONE_DIM_LOOPS);
	FACTORY_IMPL(TransfRuntimePrecheck);

	virtual void makeTransform(const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor* stmtFor = args[0].getAsFor();

		/*
		if (stmtFor == 0) {
			m_lastError = "'for' statement not selected.";
			return false;
		}
		*/

		try
		{
			MakeRuntimePrecheckTransform(stmtFor, stmtFor);
			return true;
		}
		catch (const OPS::Exception& except)
		{
			m_lastError = "Could not make 'Runtime Check' transformation.";
			return false;
		}
	}
};

// FIXME: Dead code eliminator doesn't function properly
class TransfDeadCodeEliminator : public IOpsTransform
{
public:
	TransfDeadCodeEliminator() : IOpsTransform("Dead Code Eliminator") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_BLOCK));
	}
	
	DECLARE_TRANF_CATEGORY(TC_GENERAL);
	FACTORY_IMPL(TransfDeadCodeEliminator);

	virtual void makeTransform(const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		Block* targetBlock = args[0].getAsBlock();

		/*
		if (targetBlock == 0)
		{
			m_lastError = "Statement block not selected.";
			return false;
		}
		*/

		try
		{
			//DeadCodeEliminator dce(*targetBlock);
//			dce.generateDeadCodeList();
		}
		catch (const OPS::Exception& except)
		{
		}

		return true;
	}
};
#endif
class TransfInterchangingLoops : public TransformAdapter
{
public:
	TransfInterchangingLoops() : TransformAdapter("Interchanging Loops") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
	}

	virtual void makeTransform(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor* stmtFor = args[0].getAsFor();

		try
		{
			interchangingLoops (stmtFor);
			return true;
		}
		catch (OPS::Exception& e)
		{
			m_lastError = e.getMessage();
			return false;
		}
	}
};


class TransfInvariantExportation : public TransformAdapter
{
public:
	TransfInvariantExportation() : TransformAdapter("Invariant Exportation") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
	}

	virtual void makeTransform(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor* stmtFor = args[0].getAsFor();

		try
		{
			StmtExpr* expr = invariantExportation(stmtFor);
			if (expr != 0)
			{
				return true;
			}
			else
			{
				m_lastError = "Invariant Exportation failed.";
			}
			return false;
		}
		catch (const OPS::Exception& except)
		{
			m_lastError = except.getMessage();
			return false;
		}
	}
};

class TransfIterSpaceSplitting : public TransformAdapter
{
public:
	TransfIterSpaceSplitting() : TransformAdapter("Iteration Space Splitting") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_VAR, _TL("Source occurence","Вхождение источник")));
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_VAR, _TL("Destination occurence","Вхождение назначения")));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor* stmt = args[0].getAsFor();
		ExprData* srcDep = args[1].getAsData();
		ExprData* dstDep = args[2].getAsData();

		try
		{
			if (splitPerfectLoopIterSpaceUsingDep(stmt->getThisInBlock(), srcDep, dstDep) == 0)
			{
				return true;
			}
			return false;
		}
		catch (OPS::Exception& e)
		{
			m_lastError = e.getMessage();
			return false;
		}
	}
};

class TransfLoopInversion : public TransformAdapter
{
public:
	TransfLoopInversion() : TransformAdapter("Loop Inversion") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
		m_argumentsInfo.addListArg(_TL("Check type","Тип проверки"), _TL("Auto","Автоматическая"), _TL("Yes","Да"), _TL("No","Нет"));
	}

	virtual void makeTransform(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor* stmtFor = args[0].getAsFor();

		try {

			CheckType	chType;
			if (args[1].getAsInt() == 0)
				chType = Auto;
			if (args[1].getAsInt() == 1)
				chType = Yes;
			if (args[1].getAsInt() == 2)
				chType = No;

			makeCanonizedForInversion(stmtFor, chType);

			return true;
		}
		catch(const OPS::Exception& except)
		{
			m_lastError = except.getMessage();
			return false;
		}
	}
};

#if 0
class TranfFindVectorizedLoops : public IOpsTransform
{
public:
	TranfFindVectorizedLoops() : IOpsTransform("Find Vectorized Loops") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
	}

	DECLARE_TRANF_CATEGORY(TC_ONE_DIM_LOOPS);
	FACTORY_IMPL(TranfFindVectorizedLoops);

	virtual void makeTransform(const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor* stmtFor = args[0].getAsFor();

		/*
		if (stmtFor == 0) {
			m_lastError = "'for' statement not selected.";
			return false;
		}
		*/

		try {

			if (TestLoopOnVectorizing(stmtFor)) {
				m_lastError = "Loop is vectorizable.";
			}
			else {
				m_lastError = "Loop is not vectorizable.";
			}

			return false;
		}
		catch(const OPS::Exception& except)
		{
			m_lastError = m_strName + " failed. " + except.getMessage().c_str();
			return false;
		}
	}
};

class TransfMisCalculation : public IOpsTransform
{
public:
	TransfMisCalculation() : IOpsTransform("Mis Calculation") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_ANY));
	}

	DECLARE_TRANF_CATEGORY(TC_CALCULATION_PRECISION);
	FACTORY_IMPL(TransfMisCalculation);

	virtual void makeTransform(const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		Statement* stmt = args[0].getAsStatement();

		/*
		if (stmt == 0)
		{
			m_lastError = "Operator is not selected.";
			return false;
		}
		*/

		try
		{
			createErrStmt(stmt);

			return true;
		}
		catch (OPS::Exception& e)
		{
			m_lastError = "Не удалось произвести построение оценки погрешности. Причина: " + e.getMessage();
			return false;
		}
	}
};
#endif
class TransfMultidimVarSubst : public TransformAdapter
{
public:
	TransfMultidimVarSubst() : TransformAdapter("Multidim Var Substitution") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_VAR, _TL("Source occurence","Вхождение источник")));
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_VAR, _TL("Destination occurence","Вхождение назначение")));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		ExprData* srcData = args[0].getAsData();
		ExprData* dstData = args[1].getAsData();

		try
		{
			if (substituteMultidimVar(srcData, dstData, program->getGlobals().getSubProc("main")->getBody()) == 0)
			{
				return true;
			}
		}
		catch (OPS::Exception& e)
		{
			m_lastError = _TL("Multidimesion variable substitutuon failed. Reason: ","Не удалось произвести подстановку в многомерных циклах. Причина: ") + e.getMessage();
		}
		return false;
	}
};

class TransfLoopCanonization : public TransformAdapter
{
public:
	TransfLoopCanonization() : TransformAdapter("Loop Canonization") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_WHILE));
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_ANY));
	}

	virtual void makeTransform(ProgramUnit*, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtWhile* pLoopStatement = args[0].getAsWhile();
		StmtExpr* pInitExpr = args[1].getAsExpr();

		try
		{
			return whileToCanonizedFor(pLoopStatement, pInitExpr);
		}
		catch (OPS::Exception& e)
		{
			m_lastError = e.getMessage();
			return false;
		}		
	}
};


#if 0
class TransfParallelRecurrentLoops : public IOpsTransform
{
public:
	TransfParallelRecurrentLoops() : IOpsTransform("Mapping of Recurrent Loops") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
	}

	DECLARE_TRANF_CATEGORY(TC_RECURRENT_LOOPS);
	FACTORY_IMPL(TransfParallelRecurrentLoops);

	std::string getTipsName() const { return "Paralellizing Recurrent Loops"; }

	virtual void makeTransform(const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor* targetFor = args[0].getAsFor();

		/*
		if (targetFor == 0)
		{
			m_lastError = "Не выделен цикл FOR.";
			return false;
		}
		*/

		try
		{
			RecurrentLoop RL;
            if (RL.Correct(targetFor) && RL.Build() && RL.Transform() ) 
                RL.Parallel(20);
            if (RL.m_state == RL.IS_PARALLEL) 
            {
                targetFor->getParentBlock()->insertAfter(targetFor->getThisInBlock(), RL.m_output);
                targetFor->getParentBlock()->erase(targetFor->getThisInBlock());
				return true;
			}
			else {
				m_lastError = "Ошибка выполнения преобразования.";
			}
		}
		catch (OPS::Exception& e)
		{
			m_lastError = "Не удалось произвести построение оценки погрешности. Причина: ";
		}
		return false;
	}
};
#endif

class TransfRenaming : public TransformAdapter
{
public:
	TransfRenaming() : TransformAdapter("Renaming") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_VAR));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		ExprData* targetOccurData = args[0].getAsData();

		try
		{
			if (renameInPerfectFunc(targetOccurData, program->getGlobals().getSubProc("main")->getBody()) == 0)
			{
				return true;
			}
		}
		catch (OPS::Exception& e)
		{
			m_lastError = _TL("Renaming failed. Reason: ","Не удалось произвести переименование. Причина: ") + e.getMessage();
		}
		return false;
	}
};

class TransfRenamingScalars : public TransformAdapter
{
public:
	TransfRenamingScalars() : TransformAdapter("Renaming Scalars") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_ANY, "First operator"));
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_ANY, "Last operator"));
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_VAR));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		Statement* stmtFirst = args[0].getAsStatement();
		Statement* stmtLast = args[1].getAsStatement();
		ExprData* entry = args[2].getAsData();

		try
		{
			renamingInSegment(stmtFirst, stmtLast, entry);
			return true;
		}
		catch (OPS::Exception& e)
		{
			m_lastError = _TL("Renaming failed.","Не удалось произвести переименование. Причина: ") + e.getMessage();
		}
		return false;
	}
};

#if 0

class TransfBta : public IOpsTransform
{
public:
	TransfBta() : IOpsTransform("Partial Evaluation") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_CALL));
	}

	DECLARE_TRANF_CATEGORY(TC_CALCULATION_PRECISION);
	FACTORY_IMPL(TransfBta);

	virtual std::string getTipsName() const { return "Binding Time Analysis"; }

	virtual void makeTransform(const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		Statement* stmt = 0;

		ExprCall* exprCall = args[0].getAsCall();
		if (exprCall->hasParentStatement())
			stmt = &exprCall->getParentStatement();

		/*
		if (stmt == 0)
		{
			m_lastError = "Statement with function call not selected.";
			return false;
		}
		*/

		try
		{
			createNewPartialFunction(stmt);
			return true;
		}
		catch (const OPS::Exception& except)
		{
			m_lastError = ("BTA failed. " + except.getMessage()).c_str();
			return false;
		}
	}
};
#endif

class TransfPartByIf : public TransformAdapter
{
public:
	TransfPartByIf() : TransformAdapter("Partitioning by If") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
		//m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_IF));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor* stmtFor = args[0].getAsFor();
		//StmtIf* stmtIf = args[1].getAsIf();

		try
		{
			Statement* stmtIfNew = partForByIf(stmtFor);
			return true;
		}
		catch (const OPS::Exception& except)
		{
			m_lastError = except.getMessage();
			return false;
		}
	}
};

class TransfPartIfByCondition : public TransformAdapter
{
public:
	TransfPartIfByCondition() : TransformAdapter("Partition If By Condition") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_IF));
	}

	virtual void makeTransform(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtIf* stmtIf = args[0].getAsIf();

		try
		{
			partitionIfByCondition(stmtIf);
			return true;
		}
		catch(const OPS::Exception& except)
		{
			m_lastError = except.getMessage();
			return false;
		}
	}
};

class TransfPartIfByStatement : public TransformAdapter
{
public:
	TransfPartIfByStatement() : TransformAdapter("Partition If By Statement") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_IF));
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_ANY));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtIf* stmtIf = args[0].getAsIf();
		Statement* stmtAny = args[1].getAsStatement();

		try
		{
			partitionIfByStatement(stmtIf, stmtAny);
			return true;
		}
		catch(const OPS::Exception& except)
		{
			m_lastError = except.getMessage();
			return false;
		}
	}
};

class TransfMisRoundOfNumber : public TransformAdapter
{
public:
	TransfMisRoundOfNumber() : TransformAdapter("Misround of Number")
	{
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_BLOCK));
		m_argumentsInfo.push_back(ArgumentInfo(AT_BOOL, _TL("Take initial data mistake","Учитывать начальную погрешность")));
		m_argumentsInfo.addListArg(_TL("Numbers","Числа"), _TL("Decimal numbers","Десятичные числа"), _TL("Binary numbers","Бинарные числа"));
		m_argumentsInfo.push_back(ArgumentInfo(AT_INT, "Fraction capacity"));
		m_argumentsInfo.back().defaultValue.setInt(4);
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		Block* block = args[0].getAsBlock();

		try
		{
			int fraction_capacity_param = args[3].getAsInt();
			if (fraction_capacity_param < 2)
			{
				m_lastError = "Fraction capacity parameter must be greater or equal 2.";
				return false;
			}

			addErrorStatementsToBlock(*block, args[1].getAsBool(), 
				args[2].getAsInt() == 0 ? TN_DECIMAL : TN_BINARY,
				fraction_capacity_param);

			return true;
		}
		catch (const OPS::Exception& except)
		{
			m_lastError = except.getMessage();
			return false;
		}
	}
};

#if 0
class TransfSpeculativeParExec : public IOpsTransform
{
public:
	TransfSpeculativeParExec() : IOpsTransform("Speculative Par Exec") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
	}

	DECLARE_TRANF_CATEGORY(TC_ONE_DIM_LOOPS);
	FACTORY_IMPL(TransfSpeculativeParExec);

	virtual void makeTransform(const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor* stmtFor = args[0].getAsFor();

		/*
		if (stmtFor == 0)
		{
			m_lastError = "For loop not selected.";
			return false;
		}
		*/

		try
		{
			MakeSpeculativeParExecTransform(stmtFor);
			return true;
		}
		catch (const OPS::Exception& except)
		{
			m_lastError = ("Speculative Par Exec failed. " + except.getMessage()).c_str();
			return false;
		}
	}
};

#endif
class TransfNonUnimodular : public TransformAdapter
{
private:
	void init(IMatrix& m, const std::string& filePath)
	{
		std::ifstream inT(filePath.c_str());
		for (int i = 0; i < m.numberOfRows(); i++)
			for (int j = 0; j < m.numberOfCols(); j++)
				inT >> m[i][j];
	}

public:
	TransfNonUnimodular() : TransformAdapter("Non Unimodular") { 
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_FOR));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtFor* stmtFor = args[0].getAsFor();

		try
		{
			std::string matrixFileName = program->getSourcePath().c_str();
			if (matrixFileName.rfind('.') == string::npos)
			{
				m_lastError = "could not calculate matrix file name";
				return false;
			}
			matrixFileName = matrixFileName.substr(0, matrixFileName.rfind('.')) + ".txt";
			nonUnimodularTransform(stmtFor, matrixFileName);
			return true;
		}
		catch (const OPS::Exception& except)
		{
			m_lastError = except.getMessage();
			return false;
		}
	}
};
#if 0
class TransfRecursionElimination : public IOpsTransform
{
public:
	TransfRecursionElimination() : IOpsTransform("Recursion Elimination") { }

	DECLARE_TRANF_CATEGORY(TC_GENERAL);
	FACTORY_IMPL(TransfRecursionElimination);

	virtual void makeTransform(const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
        try
		{ 
			RecursionEliminator	eliminator(m_program);
			eliminator.EliminateAllRecursion();

			return true;
		}
		catch (OPS::Exception& e)
		{
			m_lastError = "Recursion Elimination failed: " + e.getMessage();
			return false;
		}
	}
};
#endif
class TransfMatrixTransposing : public TransformAdapter
{
public:
	TransfMatrixTransposing() : TransformAdapter("Matrix Transposing") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_ARRAY, _TL("Array occurence","Вхождение массива")));
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_ANY, _TL("First index","Первый индекс")));
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_ANY, _TL("Second index","Второй индекс")));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);

		ExprArray* targetOccurData = args[0].getAsArray();
		ExprNode* srcDep = args[1].getAsExprNode();
		ExprNode* dstDep = args[2].getAsExprNode();

		try
		{
			if (MakeMatrixTransposing(targetOccurData, srcDep, dstDep, program->getGlobals().getSubProc("main")->getBody()) == 0)
			{
				return true;
			}
		}
		catch (OPS::Exception& e)
		{
			m_lastError = _TL("Matrix transposing failed.","Не удалось произвести транспонирование. Причина: ") + e.getMessage();
		}
		return false;
	}
};

class TransfLinearClass : public InformerAdapter
{
public:
	TransfLinearClass() : InformerAdapter("Linear Class") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_ANY, _TL("First statement","Первый оператор")));
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_ANY, _TL("Last statement","Последний оператор")));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);

		Statement* first = args[0].getAsStatement();
		Statement* last  = args[1].getAsStatement();

		try 
		{
			if (isInLinearClass(first)) {
				m_result = "Фрагмент принадлежит линейному классу программ.";
			}
			else {
				m_result = "Фрагмент не принадлежит линейному классу программ.";
			}
			return true;
		}
		catch(OPS::Exception& e)
		{
			m_lastError = e.getMessage();
		}
		return false;
	}
};

class TransfRegularClass : public InformerAdapter
{
public:
	TransfRegularClass() : InformerAdapter("Regular Class") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_ANY, _TL("First statement","Первый оператор")));
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_ANY, _TL("Last statement","Последний оператор")));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);

		Statement* first = args[0].getAsStatement();
		Statement* last  = args[1].getAsStatement();

		try 
		{
			if (isInRegularClass(first)) {
				m_result = "Фрагмент принадлежит регулярному классу программ.";
			}
			else {
				m_result = "Фрагмент не принадлежит регулярному классу программ.";
			}
			return true;
		}
		catch(OPS::Exception& e)
		{
			m_lastError = e.getMessage();
		}
		return false;
	}
};

class TransfFragmentToProgram : public CodeGenAdapter
{
public:
	TransfFragmentToProgram() : CodeGenAdapter("Fragment to program") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_ANY));
		m_argumentsInfo.push_back(ArgumentInfo(AT_BOOL, _TL("Insert timer","Вставлять таймер")));
		m_argumentsInfo.push_back(ArgumentInfo(AT_BOOL, _TL("Initialize variables by 0","Инициализировать переменные 0")));
		m_argumentsInfo.back().defaultValue.setBool(true);
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		Statement* fragment = args[0].getAsStatement();
		bool insertTimer = args[1].getAsBool();
		bool initVarNull = args[2].getAsBool();

		try {
			ProgramUnit* newProgram = fragmentToProgram(fragment, program, insertTimer, initVarNull);

			if (newProgram) {
				setGeneratedCode(*newProgram);
				return true;
			}
		}
		catch(const OPS::Exception& error)
		{
			m_lastError = error.getMessage();
		}
		return false;
	}
};

class TransfFragmentToProgramWithErrorCalc : public CodeGenAdapter
{
public:
	TransfFragmentToProgramWithErrorCalc() : CodeGenAdapter("Fragment to program with error calculations") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_ANY));
		m_argumentsInfo.push_back(ArgumentInfo(AT_BOOL, _TL("Take initial data mistake","Учитывать начальную погрешность")));
		m_argumentsInfo.addListArg(_TL("Numbers","Числа"), _TL("Decimal numbers","Десятичные числа"), _TL("Binary numbers","Бинарные числа"));
		m_argumentsInfo.push_back(ArgumentInfo(AT_INT, "Fraction capacity"));
		m_argumentsInfo.back().defaultValue.setInt(4);
		m_argumentsInfo.push_back(ArgumentInfo(AT_DOUBLE, "Begin data default error"));

		//m_argumentsInfo.push_back(ArgumentInfo(AT_BOOL, "Insert timer"));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		Statement* fragment = args[0].getAsStatement();
		bool isBegErr = args[1].getAsBool();
		TypeNumbers  typeNumbers = args[2].getAsInt() == 0 ? TN_DECIMAL : TN_BINARY;
		int  mantissas = args[3].getAsInt();
		double  defaultBegErr = args[4].getAsDouble();

		try {
			ProgramUnit* newProgram = fragmentToProgram(fragment, program);
			if (newProgram) {
				addErrorStatementsToBlock(newProgram, isBegErr, typeNumbers, mantissas, defaultBegErr);
				setGeneratedCode(*newProgram);
				return true;
			}
			else {
				throw OPS::Exception(_TL("Could not generate program from fragment","Не удалось сгенерировать программу по фрагменту"));
			}
		}
		catch(const OPS::Exception& error)
		{
			m_lastError = error.getMessage();
		}
		return false;
	}
};

class TransfPipIntegration : public CodeGenAdapter
{
public:
	TransfPipIntegration():CodeGenAdapter(_TL("Lattice Graph", "Решетчатый граф")) {
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_ANY, _TL("Generator Expression", "Генератор")));
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_ANY, _TL("Use Expression","Использование")));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		ExprNode* generator = args[0].getAsExprNode();
		ExprNode* use = args[1].getAsExprNode();

		try 
		{
			Block* Stmt = buildTrueDependenceLatticeGraph(generator, use);
			if (Stmt) 
			{
				std::stringstream os;
				Backends::OutToC	printer(os);
				printer.visit(*Stmt);
				m_generatedCode = os.str();
				return true;
			}
			else
				throw OPS::Exception(_TL("Error in buildTrueDependenceLatticeGraph","Ошибка в buildTrueDependenceLatticeGraph"));
		}
		catch(const OPS::Exception& error)
		{
			m_lastError = error.getMessage();
		}
		catch(...)
		{
			m_lastError = "Unknown error. Please send your program example to OPS Team.";
		}
		return false;
	}
};

class TransfRenamingThroughSSA : public TransformAdapter
{
public:
	TransfRenamingThroughSSA() : TransformAdapter("Renaming Through SSA") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_BLOCK));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		Block* stmtBlock = args[0].getAsBlock();

		try
		{
			 renamingInBlock(stmtBlock);
			 return true;
		}
		catch(const OPS::Exception& error)
		{
			m_lastError = error.getMessage();
		}
		return false;
	}
};

class TransfSSAForm : public TransformAdapter
{
public:
	TransfSSAForm() : TransformAdapter("SSA Form") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_BLOCK));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		Block* stmtBlock = args[0].getAsBlock();

		try
		{
			 SSAForm ssa(stmtBlock);
			 return true;
		}
		catch(const OPS::Exception& error)
		{
			m_lastError = error.getMessage();
		}
		return false;
	}
};

class TransfLinearization : public TransformAdapter
{
public:
	TransfLinearization() : TransformAdapter("Linearization")
	{
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_EXPR));
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_ANY));
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_ANY));
		m_argumentsInfo.back().mandatory = false;
		m_argumentsInfo.push_back(ArgumentInfo(AT_EXPR_ANY));
		m_argumentsInfo.back().mandatory = false;
	}

	void makeTransform(ProgramUnit* /*program*/, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);
		StmtExpr* stmtExpr = args[0].getAsExpr();

		std::vector<ExprNode*>	vExprs;

		for(int i = 1; i < 4; ++i)
		{
			if (args[i].getAsExprNode())
				vExprs.push_back(args[i].getAsExprNode());
		}

		try
		{
			if (makeLinearize(stmtExpr, vExprs))
				return true;

			m_lastError = "Выражение нелинейно относительно подвыражений";
			return false;
		}
		catch(OPS::Exception& error)
		{
			m_lastError = error.getMessage();
			return false;
		}
	}

};

NameSubProcIter findFunctionIter(ProgramUnit& prg, Block& body)
{
	NameSubProcIter it = prg.getGlobals().getSubProcsFirst();

	for(; it.isValid(); ++it) {
		if (&it->getBody() == &body)
			break;
	}
	return it;
}

NameSubProc* findFunction(ProgramUnit& prg, Block& body)
{
	NameSubProcIter it = findFunctionIter(prg, body);
	if (it.isValid())
		return &(*it);
	else
		return 0;
}

class TransfDivisionFunction : public TransformAdapter
{
public:
	TransfDivisionFunction() : TransformAdapter("Function Division") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_BLOCK, _TL("Function","Функция")));
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_ANY));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);

		NameSubProc* subProc = findFunction(*program, *args[0].getAsBlock());
		Statement* stmt = args[1].getAsStatement();

		Block::Iterator itStmt = stmt->getThisInBlock();

		try {
			fragmentation(itStmt, subProc, &program->getGlobals());
			return true;
		}
		catch(const OPS::Exception& error)
		{
			m_lastError = error.getMessage();
		}
		return false;
	}
};

class TransfExplicitTypeConversion : public TransformAdapter
{
public:

	TransfExplicitTypeConversion() : TransformAdapter("Explicit Type Conversion")
	{
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_BLOCK));
	}

	virtual void makeTransform(ProgramUnit* /*program*/, const ArgumentValues& params)
	{
		OPSAssert(m_argumentsInfo.validate(params), INVALID_ARGS_MESSAGE);
		Block* block = params[0].getAsBlock();

		try
		{
			MakeExplicitTypeConversion(block);
			return true;
		}
		catch (OPS::Exception& e)
		{
			m_lastError = e.getMessage();
			return false;
		}
	}
};

class TransfWhileToFor : public TransformAdapter
{
public:
	TransfWhileToFor() : TransformAdapter("While To For")
	{
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_WHILE));
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_EXPR, _TL("Initialization statement","Выражение инициализации")));
	}

	virtual void makeTransform(ProgramUnit* program, const ArgumentValues& params)
	{
		OPSAssert(m_argumentsInfo.validate(params), INVALID_ARGS_MESSAGE);
		StmtWhile* whileStmt = params[0].getAsWhile();
		StmtExpr* initStmt = params[1].getAsExpr();

		if (whileToCanonizedFor(whileStmt, initStmt))
			return true;
		else
		{
			m_lastError = _TL("conversion impossible","преобразование невозможно");
			return false;
		}
	}
};

class TransfSSADeadCodeElimination : public TransformAdapter
{
public:
	TransfSSADeadCodeElimination() : TransformAdapter("SSA Dead Code Elimination")
	{
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_BLOCK));
	}

	virtual void makeTransform(ProgramUnit* /*program*/, const ArgumentValues& params)
	{
		OPSAssert(m_argumentsInfo.validate(params), INVALID_ARGS_MESSAGE);
		Block* block = params[0].getAsBlock();

		try
		{
			SSADeadCodeElimination(block);
			return true;
		}
		catch(const OPS::Exception& err)
		{
			m_lastError = err.getMessage();
			return false;
		}
	}
};

class TransfInlining : public TransformAdapter
{
public:
	TransfInlining():TransformAdapter("Inlining") {
		m_argumentsInfo.push_back(ArgumentInfo(AT_STMT_BLOCK, _TL("Function","Функция")));
	}

	void makeTransform(ProgramUnit* program, const ArgumentValues& args)
	{
		OPSAssert(m_argumentsInfo.validate(args), INVALID_ARGS_MESSAGE);

		NameSubProcIter subProc = findFunctionIter(*program, *args[0].getAsBlock());

		try 
		{
			inlining(program, subProc);
			return true;
		}
		catch(const OPS::Exception& error)
		{
			m_lastError = error.getMessage();
		}
		return false;
	}
};


FACTORY_IMPL(CanonizationAssign)
FACTORY_IMPL(CanonizationFor)
FACTORY_IMPL(CanonizationExpr)
FACTORY_IMPL(SubstForw)
FACTORY_IMPL(TempArrays)
FACTORY_IMPL(Var2Array)
FACTORY_IMPL(MultidimVarSubst)
FACTORY_IMPL(Renaming)
FACTORY_IMPL_GEN(SynchConstraints)
FACTORY_IMPL_GEN(OMPProducer)
FACTORY_IMPL_GEN(OMPProducerDep)
FACTORY_IMPL_GEN(HDL)
FACTORY_IMPL_GEN(ForGe)
FACTORY_IMPL_GEN(PartitioningPipelining)
FACTORY_IMPL(ExceptionGenerator)
FACTORY_IMPL(LoopDistribution)
FACTORY_IMPL(LoopNesting)
FACTORY_IMPL(StripMining)
FACTORY_IMPL(LoopUnrolling)
FACTORY_IMPL(LoopFullUnrolling)
FACTORY_IMPL(DivisionFunction)
FACTORY_IMPL(SSAForm)
FACTORY_IMPL(RenamingThroughSSA)
FACTORY_IMPL(FragmentToProgram)
FACTORY_IMPL_INFO(LinearClass)
FACTORY_IMPL_INFO(RegularClass)
FACTORY_IMPL(IterSpaceSplitting)
FACTORY_IMPL(MatrixTransposing)
FACTORY_IMPL(LoopsMerging)
FACTORY_IMPL(NonUnimodular)
FACTORY_IMPL(RenamingScalars)
FACTORY_IMPL(InterchangingLoops)
FACTORY_IMPL(InvariantExportation)
FACTORY_IMPL(LoopInversion)
FACTORY_IMPL(ExplicitTypeConversion)
FACTORY_IMPL(SwapFragments)
FACTORY_IMPL(FragmentToProgramWithErrorCalc)
FACTORY_IMPL(PartByIf)
FACTORY_IMPL(SwitchToIf)
FACTORY_IMPL(WhileToFor)
FACTORY_IMPL(MisRoundOfNumber)
FACTORY_IMPL(Linearization)
FACTORY_IMPL(RecurrentLoops)
FACTORY_IMPL(LoopCanonization)
FACTORY_IMPL(SSADeadCodeElimination)
FACTORY_IMPL(PartIfByCondition)
FACTORY_IMPL(PartIfByStatement)
FACTORY_IMPL(Corruptor)
FACTORY_IMPL(Nothing)
FACTORY_IMPL(Silly)
FACTORY_IMPL_GEN(PipIntegration)
FACTORY_IMPL(Inlining)
#endif

typedef OPS::Transforms::IfDistribution TransfIfDistribution;
typedef OPS::Transforms::IfExtraction TransfIfExtraction;
typedef OPS::Transforms::IfSplitting TransfIfSplitting;
typedef OPS::Transforms::DataDistribution::BADD TransfBADD;
typedef OPS::Transforms::DataDistribution::DataDistributionForSharedMemory TransfDataDistributionForSharedMemory;
typedef OPS::Transforms::Subroutines::Inlining TransfInlining;
typedef OPS::Transforms::Subroutines::FullInlining TransfFullInlining;
typedef OPS::Transforms::Subroutines::InliningWithSubtitution TransfInliningWithSubstitution;
typedef OPS::Transforms::Subroutines::FragmentToSubroutine TransfFragmentToSubroutine;
typedef OPS::Transforms::Subroutines::SubroutineSplitting TransfSubroutineSplitting;
typedef OPS::Transforms::LoopCycleOffset TransfLoopCycleOffset;

#define FACTORY_IMPL(transformation)\
	TransformBase* create##transformation()	{ return new Transf##transformation; }

#define FACTORY_IMPL_GEN(generator)\
	CodeGeneratorBase* create##generator()	{ return new Transf##generator; }

#define FACTORY_IMPL_INFO(informer)\
	InformerBase* create##informer()		{ return new Transf##informer; }

FACTORY_IMPL(ExprSimplifier);
FACTORY_IMPL(SubstForw);
FACTORY_IMPL(ConstantPropagation);
//FACTORY_IMPL(SwitchToIf);
FACTORY_IMPL(ArithmeticOperatorExpansion);
FACTORY_IMPL(LoopDistribution);
FACTORY_IMPL(IntToVector);
FACTORY_IMPL(LoopNesting);
FACTORY_IMPL(StripMining);
//FACTORY_IMPL(Var2Array);
FACTORY_IMPL(RecurrentLoops);
FACTORY_IMPL(IfDistribution);
FACTORY_IMPL(IfExtraction);
FACTORY_IMPL(IfSplitting);
FACTORY_IMPL(Inlining);
FACTORY_IMPL(FullInlining);
FACTORY_IMPL(InliningWithSubstitution);
FACTORY_IMPL(FragmentToSubroutine);
FACTORY_IMPL(SubroutineSplitting);
FACTORY_IMPL(AliasAnalysis);

FACTORY_IMPL(SwapStatements);
FACTORY_IMPL(LoopUnrolling);
FACTORY_IMPL(LoopFullUnrolling);
FACTORY_IMPL(LoopFusion);
FACTORY_IMPL(AliasCanonicalForm);
FACTORY_IMPL(BADD);
FACTORY_IMPL(DataDistributionForSharedMemory);
FACTORY_IMPL(LoopHeaderRemoval);
FACTORY_IMPL(LoopFragmentation);
FACTORY_IMPL(RemoveUnusedDeclarations);
FACTORY_IMPL(LoopCycleOffset);


#ifdef TEST_TRANSFORMATIONS_ENABLED
FACTORY_IMPL(ExceptionGenerator);
FACTORY_IMPL(Corruptor);
FACTORY_IMPL(Nothing);
FACTORY_IMPL(Silly);
#endif

TransformFactory& TransformFactory::instance()
{
	static bool g_initialized = false;
	static TransformFactory g_factory;

	if (!g_initialized)
	{
		g_factory.registerTransform("Expression Simplifier", TransformDescription("General", createExprSimplifier));
		g_factory.registerTransform("Substitution Forward", TransformDescription("General", createSubstForw));
		g_factory.registerTransform("Constant Propagation", TransformDescription("General", createConstantPropagation, "Transforms/Scalar/ConstantPropagation"));
        //g_factory.registerTransform("Switch To If", TransformDescription("General", createSwitchToIf));
		g_factory.registerTransform("Arithmetic Operator Expansion", TransformDescription("General", createArithmeticOperatorExpansion));
		g_factory.registerTransform("Swap Statements", TransformDescription("General", createSwapStatements, "Transforms/Statements/SwapStatements"));
        g_factory.registerTransform("Remove Unused Declarations", TransformDescription("General", createRemoveUnusedDeclarations));

		g_factory.registerTransform("Loop Distribution", TransformDescription("Loops", createLoopDistribution, "Transforms/Loops/LoopDistribution"));
		g_factory.registerTransform("Loop Nesting", TransformDescription("Loops", createLoopNesting, "Transforms/Loops/LoopNesting"));
		g_factory.registerTransform("Strip Mining", TransformDescription("Loops", createStripMining));
		g_factory.registerTransform("Loop Header Removal", TransformDescription("Loops", createLoopHeaderRemoval, "Transforms/Loops/HeaderRemoval"));
		g_factory.registerTransform("Loop Fragmentation", TransformDescription("Loops", createLoopFragmentation));
		//g_factory.registerTransform("Var To Array", TransformDescription("Loops", createVar2Array));
		g_factory.registerTransform("Recurrency Elimination", TransformDescription("Loops", createRecurrentLoops, "Transforms/Loops/Recurrency Elimination"));
		g_factory.registerTransform("Loop Unrolling", TransformDescription("Loops", createLoopUnrolling, "Transforms/Loops/LoopUnrolling"));
		g_factory.registerTransform("Loop Full Unrolling", TransformDescription("Loops", createLoopFullUnrolling, "Transforms/Loops/LoopUnrolling"));
		g_factory.registerTransform("Loop Fusion", TransformDescription("Loops", createLoopFusion, "Transforms/Loops/LoopFusion"));
		//g_factory.registerTransform("Int To Vector", TransformDescription("Loops", createIntToVector, "Transforms/Vectorization"));
		g_factory.registerTransform("Loop Cycle Offset", TransformDescription("Loops", createLoopCycleOffset, "Transforms/Loops/LoopCycleOffset"));

		g_factory.registerTransform("If Distribution", TransformDescription("If", createIfDistribution, "Transforms/If/IfDistribution"));
		g_factory.registerTransform("If Extraction", TransformDescription("If", createIfExtraction, "Transforms/If/IfExtraction"));
		g_factory.registerTransform("If Splitting", TransformDescription("If", createIfSplitting, "Transforms/If/IfSplitting"));

		g_factory.registerTransform("Inline substitution", TransformDescription("Subroutines", createInlining));
		g_factory.registerTransform("Full inline substitution", TransformDescription("Subroutines", createFullInlining));
		g_factory.registerTransform("Inlining with substitution", TransformDescription("Subroutines", createInliningWithSubstitution));
		g_factory.registerTransform("Fragment to subroutine", TransformDescription("Subroutines", createFragmentToSubroutine));
		g_factory.registerTransform("Subroutine splitting", TransformDescription("Subroutines", createSubroutineSplitting));

		g_factory.registerTransform("Alias Analysis", TransformDescription("Information", createAliasAnalysis));

#ifdef TEST_TRANSFORMATIONS_ENABLED
		g_factory.registerTransform("Exception Generator", TransformDescription("Test", createExceptionGenerator));
		g_factory.registerTransform("Corruptor", TransformDescription("Test", createCorruptor));
		g_factory.registerTransform("Nothing", TransformDescription("Test", createNothing));
		g_factory.registerTransform("Silly", TransformDescription("Test", createSilly));
#endif

		g_factory.registerTransform("Alias Canonical Form", TransformDescription("Canonization", createAliasCanonicalForm));

		g_factory.registerTransform("Block Affine Data Distribution", TransformDescription("Data Distribution", createBADD, "Transforms/DataDistribution"));
		g_factory.registerTransform("Data Distribution For Shared Memory", TransformDescription("Data Distribution", createDataDistributionForSharedMemory, "Transforms/DataDistribution"));

		g_initialized = true;
	}

	return g_factory;
}

void TransformFactory::registerTransform(const std::string &name, const TransformDescription &desc)
{
	m_transforms[name] = desc;
}

std::unique_ptr<TransformBase> TransformFactory::create(const std::string& name) const
{
	TransformRegistry::const_iterator it = m_transforms.find(name);

	if (it != m_transforms.end())
	{
		return std::unique_ptr<TransformBase>((*it->second.factory)());
	}
	return std::unique_ptr<TransformBase>(nullptr);
}

std::string TransformFactory::getCategory(const std::string& name) const
{
	TransformRegistry::const_iterator it = m_transforms.find(name);

	if (it != m_transforms.end())
	{
		return it->second.category;
	}
	else
		throw OPS::ArgumentError("Transformation '" + name + "' was not found");
}

std::string TransformFactory::getSamplesSuffix(const std::string& name) const
{
	TransformRegistry::const_iterator it = m_transforms.find(name);

	if (it != m_transforms.end())
	{
		return it->second.samplesSuffix;
	}
	else
		throw OPS::ArgumentError("Transformation '" + name + "' was not found");
}


std::vector<std::string> TransformFactory::getNames() const
{
	std::vector<std::string> names;
	names.reserve(m_transforms.size());

	TransformRegistry::const_iterator it = m_transforms.begin();
	for(; it != m_transforms.end(); ++it)
		names.push_back(it->first);

	return names;
}

std::list<std::string> TransformFactory::getCategories() const
{
	std::set<std::string> categories;

	TransformRegistry::const_iterator it = m_transforms.begin();
	for(; it != m_transforms.end(); ++it)
		categories.insert(it->second.category);

	return std::list<std::string>(categories.begin(), categories.end());
}

TransformFactory::TransformDescription::TransformDescription(const std::string& _cat, TransformFactoryFunc _func, const std::string& samplesSuffix_)
	:category(_cat)
	,samplesSuffix(samplesSuffix_)
	,factory(_func)
{
}

}
}
