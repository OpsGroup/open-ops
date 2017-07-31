
#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Shared/RepriseClone.h"
#include "Backends/RepriseXml/RepriseXml.h"
#include "Backends/OutToC/OutToC.h"

#include <string>

#include "GTestIncludeWrapper.h"
#include "../FrontendHelper.h"

using namespace std;
using namespace OPS;
using namespace OPS::Reprise;

//TEST(FactorialTest, HandlesZeroInput) 
//{
//  EXPECT_EQ(1, 1);
//}

TEST(Frontend, ParseNegativeFile)
{
    OPS::Frontend::Frontend frontend;
    const std::string input_filepath = IO::osPathToPosixPath(IO::combinePath(sourceRoot, L"tests/Reprise/UnitTests/negative_test.c"));
    const CompileResult& result = frontend.compileSingleFile(input_filepath);
    EXPECT_NE(0, result.errorCount());
}


TEST(Frontend, CheckGoto)
{
    COMPILE_FILE("tests/Reprise/UnitTests/goto_test.c");
	ASSERT_EQ(1, frontend.getProgramUnit().getUnitCount());
	
	TranslationUnit& unit = frontend.getProgramUnit().getUnit(0);
	EXPECT_EQ(0, unit.getGlobals().getVariableCount());
	EXPECT_EQ(1, unit.getGlobals().getSubroutineCount());

	Declarations::SubrIterator firstSubr = unit.getGlobals().getFirstSubr();
	if (firstSubr.isValid())
	{
		SubroutineDeclaration& main = *firstSubr;
		EXPECT_EQ(true, main.hasImplementation());

		BlockStatement& body = main.getBodyBlock();
        EXPECT_FALSE( body.isEmpty());
		BlockStatement::Iterator stmt = body.getFirst();
		int stmtCount = 0;
		while (stmt.isValid())
		{
			stmtCount += 1;
			stmt.goNext();
		}
		EXPECT_EQ(2, stmtCount);
	}
}

TEST(Reprise, DumpLabels) 
{
    COMPILE_FILE("tests/Reprise/UnitTests/dump_labels.c");
	EXPECT_GE(1, frontend.getProgramUnit().getUnitCount());
	//TranslationUnit& unit = frontend.getProgramUnit().getUnit(0);
//	std::cout << unit.dumpState();
}

TEST(Reprise, GetAssociatedVariable) 
{
    COMPILE_FILE("tests/Reprise/UnitTests/get_associated_variable.c");
	EXPECT_GE(1, frontend.getProgramUnit().getUnitCount());
	for (int unitIndex = 0; unitIndex < frontend.getProgramUnit().getUnitCount(); ++unitIndex)
	{
		TranslationUnit& unit = frontend.getProgramUnit().getUnit(unitIndex);
		for (Declarations::SubrIterator subIter = unit.getGlobals().getFirstSubr(); subIter.isValid(); ++subIter)
		{
			SubroutineType& subroutineF = subIter->getType();
			for (int paramIndex = 0; paramIndex < subroutineF.getParameterCount(); ++paramIndex)
			{
				EXPECT_TRUE(subroutineF.getParameter(paramIndex).hasAssociatedVariable());
				VariableDeclaration& varDecl = subroutineF.getParameter(paramIndex).getAssociatedVariable();
				EXPECT_TRUE(varDecl.hasParameterReference());
				EXPECT_EQ(&varDecl.getParameterReference(), &subroutineF.getParameter(paramIndex));
			}
		}
	}
}

TEST(Reprise, GotoClone)
{
	using namespace OPS::Shared;
    COMPILE_FILE("tests/Reprise/UnitTests/goto_test.c");
	ASSERT_GE(1, frontend.getProgramUnit().getUnitCount());
	TranslationUnit& unit = frontend.getProgramUnit().getUnit(0);
	ReprisePtr<TranslationUnit> newUnit(deepCloneTranslationUnit(unit));
	StatementBase& oldGoto = *unit.getGlobals().getFirstSubr()->getBodyBlock().getLast();
	StatementBase* oldPointed = oldGoto.cast_to<GotoStatement>().getPointedStatement();
	StatementBase& newGoto = *newUnit->getGlobals().getFirstSubr()->getBodyBlock().getLast();
	StatementBase* newPointed = newGoto.cast_to<GotoStatement>().getPointedStatement();
	EXPECT_NE(oldPointed, newPointed);

	/*
	OPS::XmlBuilder xml;
	OPS::Backends::RepriseXml::Options options;
	options.writeNCIDofParent = true;
	OPS::Backends::RepriseXml repXml(xml, options);
	repXml.visit(*newUnit);
	cout << xml.dump();
	*/
}

TEST(Reprise, StructAccess)
{
	using namespace OPS::Shared;
    COMPILE_FILE("tests/Reprise/UnitTests/struct_access.c");
	ASSERT_GE(1, frontend.getProgramUnit().getUnitCount());
	//TranslationUnit& unit = frontend.getProgramUnit().getUnit(0);


	/*
	OPS::XmlBuilder xml;
	OPS::Backends::RepriseXml::Options options;
	options.writeNCIDofParent = true;
	OPS::Backends::RepriseXml repXml(xml, options);
	repXml.visit(*newUnit);
	cout << xml.dump();
	*/
}

TEST(Reprise, GetResultTypeForArrays)
{
    COMPILE_FILE("tests/Reprise/UnitTests/get_result_type.c");
	ASSERT_GE(1, frontend.getProgramUnit().getUnitCount());
	TranslationUnit& unit = frontend.getProgramUnit().getUnit(0);

	BlockStatement& body = unit.getGlobals().getFirstSubr()->getBodyBlock();
	for (BlockStatement::Iterator stIter = body.getFirst(); stIter.isValid(); ++stIter)
	{
		ExpressionStatement& expr = stIter->cast_to<ExpressionStatement>();
		BasicCallExpression& arrayAccess = expr.get().cast_to<BasicCallExpression>().getArgument(0).cast_to<BasicCallExpression>();
		ReprisePtr<TypeBase> resultType;
        ASSERT_NO_THROW(resultType = arrayAccess.getResultType());
		ASSERT_TRUE(0 != resultType.get());
		EXPECT_TRUE(resultType->is_a<BasicType>());
	}
}

TEST(Reprise, GetResultTypeForVectors)
{
	COMPILE_FILE("tests/Reprise/UnitTests/vec_get_result_type.c");
	BlockStatement& body = frontend.getProgramUnit().getUnit(0).getGlobals().findSubroutine("main")->getBodyBlock();

	EXPECT_TRUE(body.getChild(0).cast_to<ExpressionStatement>().get().getResultType()->is_a<VectorType>());
	EXPECT_TRUE(body.getChild(1).cast_to<ExpressionStatement>().get().getResultType()->is_a<BasicType>());
	EXPECT_TRUE(body.getChild(2).cast_to<ExpressionStatement>().get().getResultType()->is_a<VectorType>());
	EXPECT_TRUE(body.getChild(3).cast_to<ExpressionStatement>().get().getResultType()->is_a<VectorType>());
}

TEST(Reprise, GetResultTypeForFuncPtr)
{
	COMPILE_FILE("tests/Reprise/UnitTests/func_ptr_result_type.c");

	BlockStatement& body = frontend.getProgramUnit().getUnit(0).getGlobals().findSubroutine("main")->getBodyBlock();

	ASSERT_EQ(4, body.getChildCount());

	// Explicit function call
	ASSERT_TRUE(body.getChild(0).is_a<ExpressionStatement>());
	ReprisePtr<TypeBase> type0 = body.getChild(0).cast_to<ExpressionStatement>().get().getResultType();
	EXPECT_TRUE(type0->is_a<BasicType>());

	// Function call by pointer
	ASSERT_TRUE(body.getChild(1).is_a<ExpressionStatement>());
	ReprisePtr<TypeBase> type1 = body.getChild(1).cast_to<ExpressionStatement>().get().getResultType();
	EXPECT_TRUE(type1->is_a<BasicType>());

	// Explicit function reference
	ASSERT_TRUE(body.getChild(2).is_a<ExpressionStatement>());
	ReprisePtr<TypeBase> type2 = body.getChild(2).cast_to<ExpressionStatement>().get().getResultType();
	EXPECT_TRUE(type2->is_a<SubroutineType>());

	// Dereference pointer to function
	ASSERT_TRUE(body.getChild(3).is_a<ExpressionStatement>());
	ReprisePtr<TypeBase> type3 = body.getChild(3).cast_to<ExpressionStatement>().get().getResultType();
	EXPECT_TRUE(type3->is_a<SubroutineType>());
}

TEST(Reprise, GetResultTypeForConditional)
{
    COMPILE_FILE("tests/Reprise/UnitTests/conditional_result_type.c");

    BlockStatement& main = frontend.getProgramUnit().getUnit(0).getGlobals().findSubroutine("main")->getBodyBlock();
    ASSERT_EQ(9, main.getChildCount());

    for(int i = 0; i < 9; ++i)
    {
        ASSERT_TRUE(main.getChild(i).is_a<ExpressionStatement>());
        ReprisePtr<TypeBase> resType = main.getChild(i).cast_to<ExpressionStatement>().get().getResultType();
        ASSERT_TRUE(resType->is_a<BasicType>());
        EXPECT_EQ(BasicType::BT_INT32, resType->cast_to<BasicType>().getKind());
    }
}

TEST(Reprise, GetResultTypeForArithmetic)
{
    COMPILE_FILE("tests/Reprise/UnitTests/arithmetic_result_type.c");

    BlockStatement& main = frontend.getProgramUnit().getUnit(0).getGlobals().findSubroutine("main")->getBodyBlock();
    ASSERT_EQ(16, main.getChildCount());

    for(int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(BasicType::BT_INT32, main.getChild(i).cast_to<ExpressionStatement>().get().getResultType()->cast_to<BasicType>().getKind());
    }

    EXPECT_EQ(BasicType::BT_FLOAT32, main.getChild(10).cast_to<ExpressionStatement>().get().getResultType()->cast_to<BasicType>().getKind());
    EXPECT_EQ(BasicType::BT_FLOAT64, main.getChild(11).cast_to<ExpressionStatement>().get().getResultType()->cast_to<BasicType>().getKind());
    EXPECT_TRUE(main.getChild(12).cast_to<ExpressionStatement>().get().getResultType()->is_a<PtrType>());
    EXPECT_TRUE(main.getChild(13).cast_to<ExpressionStatement>().get().getResultType()->is_a<PtrType>());
    EXPECT_EQ(BasicType::BT_UINT32, main.getChild(14).cast_to<ExpressionStatement>().get().getResultType()->cast_to<BasicType>().getKind());
    EXPECT_EQ(BasicType::BT_INT32, main.getChild(15).cast_to<ExpressionStatement>().get().getResultType()->cast_to<BasicType>().getKind());
}

TEST(Reprise, RecursiveStructs)
{
    COMPILE_FILE("tests/Reprise/UnitTests/recursive_structs.c");
	ASSERT_GE(1, frontend.getProgramUnit().getUnitCount());
	TranslationUnit& unit = frontend.getProgramUnit().getUnit(0);

    std::stringstream ss;
    OPS::Backends::OutToC outc(ss);
	unit.accept(outc);
}

TEST(Reprise, ArrayEqual)
{
    COMPILE_FILE("tests/Reprise/UnitTests/array_equal.c");
    TypeBase& Atype = frontend.getProgramUnit().getUnit(0).getGlobals().findVariable("A")->getType();
    TypeBase& Btype = frontend.getProgramUnit().getUnit(0).getGlobals().findVariable("B")->getType();

    EXPECT_TRUE(Atype.isEqual(Btype));
}
