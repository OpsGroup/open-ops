#include "ConsistCheckWalker.h"

#include <Reprise/Reprise.h>

#include <OPS_Core/Helpers.h>

using namespace OPS::Reprise;

namespace OPS
{

namespace Analysis
{

ConsistCheckWalker::ConsistCheckWalker(const ConditionList& conditions)
	: m_result(true)
	, m_conditions(conditions)
	, m_isFirstVisit(true)
{
}

bool ConsistCheckWalker::getResult() const
{
	return m_result;
}

void ConsistCheckWalker::validate(RepriseBase& repriseObject)
{
	{
		const bool skeepValidation = (repriseObject.is_a<StatementBase>() ||
			repriseObject.is_a<VariableDeclaration>()) && m_isFirstVisit;

		m_isFirstVisit = false;

		if (skeepValidation)
		{
			return;
		}
	}

	typedef ConditionList::const_iterator ConstIterator;

	for (ConstIterator it = m_conditions.begin(); it != m_conditions.end(); ++it)
	{
		OPS_ASSERT(*it != NULL);

		if ((**it).isAllowed(repriseObject))
		{
			return;
		}
	}

	fail();
}

void ConsistCheckWalker::fail()
{
	m_result = false;
}

void ConsistCheckWalker::visit(ProgramUnit& )
{
	OPS_ASSERT(!"Unexpected behaviour");

	fail();
}

void ConsistCheckWalker::visit(TranslationUnit& )
{
	OPS_ASSERT(!"Unexpected behaviour");

	fail();
}

void ConsistCheckWalker::visit(ProgramFragment& )
{
	OPS_ASSERT(!"Unexpected behaviour");

	fail();
}

void ConsistCheckWalker::visit(Declarations& )
{
	OPS_ASSERT(!"Unexpected behaviour");

	fail();
}

void ConsistCheckWalker::visit(VariableDeclaration& variableDeclaration)
{
	validate(variableDeclaration);

	DeepWalker::visit(variableDeclaration);
}

void ConsistCheckWalker::visit(TypeDeclaration& )
{
	OPS_ASSERT(!"Unexpected behaviour");

	 fail();
}

void ConsistCheckWalker::visit(SubroutineDeclaration& )
{
	OPS_ASSERT(!"Unexpected behaviour");

	fail();
}

void ConsistCheckWalker::visit(BlockStatement& blockStatement)
{
	validate(blockStatement);

	Declarations& declarations = blockStatement.getDeclarations();

	typedef Declarations::Iterator Iterator;

	for(Iterator it = declarations.getFirst(); it.isValid(); ++it)
	{
		if (it->is_a<VariableDeclaration>())
		{
			VariableDeclaration& varDecl = it->cast_to<VariableDeclaration>();

			if (varDecl.hasDefinedBlock() &&
				&varDecl.getDefinedBlock() == &blockStatement)
			{
				varDecl.accept(*this);
			}
		}
	}

	DeepWalker::visit(blockStatement);
}

void ConsistCheckWalker::visit(ForStatement& forStatement)
{
	validate(forStatement);

	DeepWalker::visit(forStatement);
}

void ConsistCheckWalker::visit(WhileStatement& whileStatement)
{
	validate(whileStatement);

	DeepWalker::visit(whileStatement);
}

void ConsistCheckWalker::visit(IfStatement& ifStatement)
{
	validate(ifStatement);

	DeepWalker::visit(ifStatement);
}

void ConsistCheckWalker::visit(PlainCaseLabel& plainCaseLabel)
{
	validate(plainCaseLabel);

	DeepWalker::visit(plainCaseLabel);
}

void ConsistCheckWalker::visit(PlainSwitchStatement& plainSwitchStatement)
{
	validate(plainSwitchStatement);

	DeepWalker::visit(plainSwitchStatement);
}

void ConsistCheckWalker::visit(GotoStatement& gotoStatement)
{
	validate(gotoStatement);

	DeepWalker::visit(gotoStatement);
}

void ConsistCheckWalker::visit(ReturnStatement& returnStatement)
{
	validate(returnStatement);

	DeepWalker::visit(returnStatement);
}

void ConsistCheckWalker::visit(Canto::HirBreakStatement& breakStatement)
{
	validate(breakStatement);

	DeepWalker::visit(breakStatement);
}

void ConsistCheckWalker::visit(Canto::HirContinueStatement& continueStatement)
{
	validate(continueStatement);

	DeepWalker::visit(continueStatement);
}

void ConsistCheckWalker::visit(ExpressionStatement& expressionStatement)
{
	validate(expressionStatement);

	DeepWalker::visit(expressionStatement);
}

void ConsistCheckWalker::visit(EmptyStatement& emptyStatement)
{
	validate(emptyStatement);

	DeepWalker::visit(emptyStatement);
}

void ConsistCheckWalker::visit(BasicType& basicType)
{
	validate(basicType);

	DeepWalker::visit(basicType);
}

void ConsistCheckWalker::visit(PtrType& ptrType)
{
	validate(ptrType);

	DeepWalker::visit(ptrType);
}

void ConsistCheckWalker::visit(TypedefType& typedefType)
{
	validate(typedefType);

	DeepWalker::visit(typedefType);
}

void ConsistCheckWalker::visit(ArrayType& arrayType)
{
	validate(arrayType);

	DeepWalker::visit(arrayType);
}

void ConsistCheckWalker::visit(StructMemberDescriptor& structMemberDescriptor)
{
	validate(structMemberDescriptor);

	DeepWalker::visit(structMemberDescriptor);
}

void ConsistCheckWalker::visit(StructType& structType)
{
	validate(structType);

	DeepWalker::visit(structType);
}

void ConsistCheckWalker::visit(EnumMemberDescriptor& enumMemberDescriptor)
{
	validate(enumMemberDescriptor);

	DeepWalker::visit(enumMemberDescriptor);
}

void ConsistCheckWalker::visit(EnumType& enumType)
{
	validate(enumType);

	DeepWalker::visit(enumType);
}

void ConsistCheckWalker::visit(ParameterDescriptor& parameterDescriptor)
{
	validate(parameterDescriptor);

	DeepWalker::visit(parameterDescriptor);
}

void ConsistCheckWalker::visit(SubroutineType& subroutineType)
{
	validate(subroutineType);

	DeepWalker::visit(subroutineType);
}

void ConsistCheckWalker::visit(DeclaredType& declaredType)
{
	validate(declaredType);

	DeepWalker::visit(declaredType);
}

void ConsistCheckWalker::visit(Reprise::VectorType& vectorType)
{
	validate(vectorType);

	DeepWalker::visit(vectorType);
}

void ConsistCheckWalker::visit(Canto::HirCBasicType& )
{
	fail();
}

void ConsistCheckWalker::visit(Canto::HirFBasicType& )
{
	fail();
}

void ConsistCheckWalker::visit(Canto::HirFArrayType& )
{
	fail();
}

void ConsistCheckWalker::visit(BasicLiteralExpression& basicLiteralExpression)
{
	validate(basicLiteralExpression);

	DeepWalker::visit(basicLiteralExpression);
}

void ConsistCheckWalker::visit(StrictLiteralExpression& strictLiteralExpression)
{
	validate(strictLiteralExpression);

	DeepWalker::visit(strictLiteralExpression);
}

void ConsistCheckWalker::visit(CompoundLiteralExpression& compoundLiteralExpression)
{
	validate(compoundLiteralExpression);

	DeepWalker::visit(compoundLiteralExpression);
}

void ConsistCheckWalker::visit(ReferenceExpression& referenceExpression)
{
	validate(referenceExpression);

	DeepWalker::visit(referenceExpression);
}

void ConsistCheckWalker::visit(SubroutineReferenceExpression& subroutineReferenceExpression)
{
	validate(subroutineReferenceExpression);

	DeepWalker::visit(subroutineReferenceExpression);
}

void ConsistCheckWalker::visit(StructAccessExpression& structAccessExpression)
{
	validate(structAccessExpression);

	DeepWalker::visit(structAccessExpression);
}

void ConsistCheckWalker::visit(EnumAccessExpression& enumAccessExpression)
{
	validate(enumAccessExpression);

	DeepWalker::visit(enumAccessExpression);
}

void ConsistCheckWalker::visit(TypeCastExpression& typeCastExpression)
{
	validate(typeCastExpression);

	DeepWalker::visit(typeCastExpression);
}

void ConsistCheckWalker::visit(BasicCallExpression& basicCallExpression)
{
	validate(basicCallExpression);

	DeepWalker::visit(basicCallExpression);
}

void ConsistCheckWalker::visit(SubroutineCallExpression& subroutineCallExpression)
{
	validate(subroutineCallExpression);

	DeepWalker::visit(subroutineCallExpression);
}

void ConsistCheckWalker::visit(EmptyExpression& emptyExpression)
{
	validate(emptyExpression);

	DeepWalker::visit(emptyExpression);
}

void ConsistCheckWalker::visit(Canto::HirCCallExpression& )
{
	fail();
}

void ConsistCheckWalker::visit(Canto::HirFAltResultExpression& )
{
	fail();
}

void ConsistCheckWalker::visit(Canto::HirFAsteriskExpression& )
{
	fail();
}

void ConsistCheckWalker::visit(Canto::HirFDimensionExpression& )
{
	fail();
}

void ConsistCheckWalker::visit(Canto::HirFArrayShapeExpression& )
{
	fail();
}

void ConsistCheckWalker::visit(Canto::HirFArrayIndexRangeExpression& )
{
	fail();
}

void ConsistCheckWalker::visit(Canto::HirFImpliedDoExpression& )
{
	fail();
}

void ConsistCheckWalker::visit(Canto::HirFArgumentPairExpression& )
{
	fail();
}

void ConsistCheckWalker::visit(Canto::HirFIntrinsicCallExpression& )
{
	fail();
}

void ConsistCheckWalker::visit(Canto::HirFIntrinsicReferenceExpression& )
{
	fail();
}

}

}
