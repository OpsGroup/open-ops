#include "Shared/ProgramTextBuilder.h"

using namespace OPS::Shared;
using namespace OPS::Reprise;

namespace
{
	inline std::string white(int n)
	{
		return std::string(2*n, ' ');
	}
}

ProgramTextBuilder::Options::Options()
	:hideTypeCasts(false)
	,hidePrototypes(false)
	,hideVariableDeclarations(false)
	,hideTypeDeclarations(false)
	,hideEmptyElses(true)
	,hideExternalDeclarations(true)
    ,emptyLineAfterSubroutine(true)
{
}

bool ProgramTextBuilder::Options::operator ==(const ProgramTextBuilder::Options& other) const
{
	return this->hideTypeCasts == other.hideTypeCasts &&
			this->hidePrototypes == other.hidePrototypes &&
			this->hideVariableDeclarations == other.hideVariableDeclarations &&
			this->hideTypeDeclarations == other.hideTypeDeclarations &&
			this->hideEmptyElses == other.hideEmptyElses &&
            this->hideExternalDeclarations == other.hideExternalDeclarations &&
            this->emptyLineAfterSubroutine == other.emptyLineAfterSubroutine;
}

ProgramTextBuilder::ProgramTextBuilder(IProgramOutputStream &stream, const Options &options)
	:m_options(options)
	,m_stream(&stream)
{
}

void ProgramTextBuilder::visit(ProgramUnit& program)
{
	baseClass::visit(program);
}

template<typename Decl>
	inline bool isExternalDeclaration(Decl& decl)
{
	if (decl.hasDefinition())
	{
		Decl* definition = &decl.getDefinition();
		if (definition != &decl &&
			definition->findTranslationUnit() != decl.findTranslationUnit())
			return true;
	}
	return false;
}

void ProgramTextBuilder::visit(VariableDeclaration& var)
{
	bool hide =(m_options.hideVariableDeclarations) ||
			   (m_options.hideExternalDeclarations && isExternalDeclaration(var));

	if (!hide)
	{
		newLine("", var);
		var.getType().accept(*this);
		addBack((" " + var.getName()).c_str(), var);
	}
}

void ProgramTextBuilder::visit(TypeDeclaration& type)
{
	if (m_options.hideTypeDeclarations == false)
	{
		newLine(type.getName().c_str(), type);
	}
}

void ProgramTextBuilder::visit(SubroutineDeclaration& sub)
{
	bool hide =(m_options.hidePrototypes && !sub.hasImplementation()) ||
			   (m_options.hideExternalDeclarations && isExternalDeclaration(sub));
	if (!hide)
	{
		newLine((sub.getName() + "()").c_str(), sub);
		baseClass::visit(sub);
        if (m_options.emptyLineAfterSubroutine && sub.hasImplementation())
            newLine("", sub);
	}
}

void ProgramTextBuilder::visit(Canto::HirFBasicType& type)
{
	addBack(Canto::HirFBasicType::basicKindToString(type.getKind(), false).c_str(), type);
}

void ProgramTextBuilder::visit(Canto::HirFArrayType& arrayType)
{
	arrayType.getBaseType().accept(*this);
	addBack("[", arrayType);
	arrayType.getShape().accept(*this);
	addBack("]", arrayType);
}

void ProgramTextBuilder::visit(BasicType& basicType)
{
	addBack(BasicType::basicTypeToString(basicType.getKind()).c_str(), basicType);
}

void ProgramTextBuilder::visit(PtrType& ptrType)
{
	ptrType.getPointedType().accept(*this);
	addBack("*", ptrType);
}

void ProgramTextBuilder::visit(ArrayType& arrType)
{
	if (!arrType.hasCountExpression())
	{
		arrType.getBaseType().accept(*this);
		if (arrType.isFullType())
			addBack(OPS::Strings::format("[%i]", arrType.getElementCount()).c_str(), arrType);
		else
			addBack("[]", arrType);
	}
}

void ProgramTextBuilder::visit(VectorType& vectType)
{
	vectType.getBaseType().accept(*this);
	if (vectType.isFullType())
		addBack(OPS::Strings::format("[%i]", vectType.getElementCount()).c_str(), vectType);
	
}

void ProgramTextBuilder::visit(StructType&) { /* ??? */ }
void ProgramTextBuilder::visit(SubroutineType&) { /* ??? */ }

void ProgramTextBuilder::visit(DeclaredType& declType)
{
	addBack(declType.getDeclaration().getName().c_str(), declType);
}

void ProgramTextBuilder::visit(BlockStatement& block)
{
	outLabel(block);
	newLine("{", block);
	increaseIndent();
	baseClass::visit(block);
	decreaseIndent();
	newLine("}", block);
}

void ProgramTextBuilder::visit(ForStatement& forst)
{
	outLabel(forst);
	newLine("for ", forst);
	forst.getInitExpression().accept(*this);
	addBack("; ", forst);
	forst.getFinalExpression().accept(*this);
	addBack("; ", forst);
	forst.getStepExpression().accept(*this);
	forst.getBody().accept(*this);
}

void ProgramTextBuilder::visit(WhileStatement& whilest)
{
	outLabel(whilest);
	if (whilest.isPreCondition())
	{
		newLine("while ", whilest);
		baseClass::visit(whilest);
	}
	else
	{
		newLine("do", whilest);
		whilest.getBody().accept(*this);
		newLine("while ", whilest);
		whilest.getCondition().accept(*this);
	}
}

void ProgramTextBuilder::visit(PlainCaseLabel& label)
{
	m_stmtToCase[&label.getStatement()].push_back(&label);
}

void ProgramTextBuilder::visit(PlainSwitchStatement& switchStmt)
{
	outLabel(switchStmt);
	newLine("switch ", switchStmt);
	switchStmt.getCondition().accept(*this);

	for(int i = 0; i < switchStmt.getLabelCount(); ++i)
		switchStmt.getLabel(i).accept(*this);

	switchStmt.getBody().accept(*this);
}

void ProgramTextBuilder::visit(IfStatement& ifst)
{
	outLabel(ifst);
	newLine("if ", ifst);
	ifst.getCondition().accept(*this);
	ifst.getThenBody().accept(*this);
	if (!m_options.hideEmptyElses || !ifst.getElseBody().isEmpty())
	{
		newLine("else", ifst);
		ifst.getElseBody().accept(*this);
	}
}
void ProgramTextBuilder::visit(GotoStatement& go)
{
	outLabel(go);

	std::string label;
	if (go.getPointedStatement() != 0 && go.getPointedStatement()->hasLabel())
		label = go.getPointedStatement()->getLabel();

	newLine(("goto " + label).c_str(), go);
}
void ProgramTextBuilder::visit(ReturnStatement& ret)
{
	outLabel(ret);
	newLine("return ", ret);
	baseClass::visit(ret);
}
void ProgramTextBuilder::visit(ExpressionStatement& expr)
{
	outLabel(expr);
	newLine("", expr);
	baseClass::visit(expr);
}
void ProgramTextBuilder::visit(EmptyStatement& empty)
{
	outLabel(empty);
}

void ProgramTextBuilder::visit(BasicLiteralExpression& literalExpr)
{
	addBack(literalExpr.getLiteralValueAsString(false).c_str(), literalExpr);
}
void ProgramTextBuilder::visit(StrictLiteralExpression& literalExpr)
{
	std::string literalStr = literalExpr.getLiteralValueAsString(false);
	if (literalExpr.isString())
	{
		literalStr = "\"" + StrictLiteralExpression::stringToEscapedString(literalStr) + "\"";
	}
    else if (literalExpr.isCharacter())
    {
        literalStr = "\'" + StrictLiteralExpression::stringToEscapedString(literalStr) + "\'";
    }
	addBack(literalStr.c_str(), literalExpr);
}
void ProgramTextBuilder::visit(ReferenceExpression& referenceExpr)
{
	addBack(referenceExpr.getReference().getName().c_str(), referenceExpr);
}
void ProgramTextBuilder::visit(SubroutineReferenceExpression& referenceExpr)
{
	addBack(referenceExpr.getReference().getName().c_str(), referenceExpr);
}
void ProgramTextBuilder::visit(StructAccessExpression& structAccess)
{
	addBack("(", structAccess);
	baseClass::visit(structAccess);
	addBack((")." + structAccess.getMember().getName()).c_str(), structAccess);
}
void ProgramTextBuilder::visit(EnumAccessExpression& enumAccess)
{
	addBack(enumAccess.getMember().getName().c_str(), enumAccess);
}

void ProgramTextBuilder::visit(TypeCastExpression& typeCast)
{
	if (m_options.hideTypeCasts)
	{
		typeCast.getCastArgument().accept(*this);
	}
	else
	{
		addBack("((", typeCast);
		typeCast.getCastType().accept(*this);
		addBack(")", typeCast);

		const bool varCast = typeCast.getCastArgument().is_a<ReferenceExpression>();
		if (!varCast)
			addBack("(", typeCast);

		typeCast.getCastArgument().accept(*this);

		if (!varCast)
			addBack("))", typeCast);
	}
}
void ProgramTextBuilder::visit(BasicCallExpression& basic)
{
	if (basic.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
	{
		const bool simpleAccess = basic.getArgument(0).is_a<ReferenceExpression>();
		if (!simpleAccess)
			addBack("(", basic);
		basic.getArgument(0).accept(*this);
		if (!simpleAccess)
			addBack(")", basic);

		addBack("[", basic);
		for(int i = 1; i < basic.getArgumentCount(); ++i)
		{
			if (i > 1)
				addBack(", ", basic);
			basic.getArgument(i).accept(*this);
		}
		addBack("]", basic);
	}
	else if (basic.getKind() == BasicCallExpression::BCK_CONDITIONAL)
	{
		basic.getArgument(0).accept(*this);
		addBack(" ? ", basic);
		basic.getArgument(1).accept(*this);
		addBack(" : ", basic);
		basic.getArgument(2).accept(*this);
	}
	else
	{
		const int argCount = basic.getArgumentCount();
		const bool topLevel = basic.getParent()->is_a<StatementBase>();

		if (!topLevel)
			addBack("(", basic);

		switch (argCount)
		{
		case 1:
			addBack((" " + BasicCallExpression::builtinCallKindToString(basic.getKind()) + " ").c_str(), basic);
			basic.getArgument(0).accept(*this);
			break;
		case 2:
			basic.getArgument(0).accept(*this);
			addBack((" " + BasicCallExpression::builtinCallKindToString(basic.getKind()) + " ").c_str(), basic);
			basic.getArgument(1).accept(*this);
			break;
		OPS_DEFAULT_CASE_LABEL
		}

		if (!topLevel)
			addBack(")", basic);
	}
}
void ProgramTextBuilder::visit(SubroutineCallExpression& subroutine)
{
	if (subroutine.hasExplicitSubroutineDeclaration())
	{
		addBack((subroutine.getExplicitSubroutineDeclaration().getName() + "(").c_str(), subroutine);
	}
	else
	{
		subroutine.getCallExpression().accept(*this);
		addBack("(", subroutine);
	}

	for (int index = 0; index < subroutine.getArgumentCount(); ++index)
	{
		if (index > 0)
			addBack(", ", subroutine);
		subroutine.getArgument(index).accept(*this);
	}
	addBack(")", subroutine);
}

void ProgramTextBuilder::visit(Canto::HirBreakStatement& brk)
{
    outLabel(brk);
    newLine("break", brk);
}

void ProgramTextBuilder::visit(Canto::HirContinueStatement& cont)
{
    outLabel(cont);
    newLine("continue", cont);
}

void ProgramTextBuilder::visit(Canto::HirCCallExpression& basic)
{
	using namespace OPS::Reprise::Canto;

	switch (basic.getKind())
	{
		//	Unary
	case HirCCallExpression::HIRC_UNARY_PLUS:
		basic.getArgument(0).accept(*this);
		break;
	case HirCCallExpression::HIRC_UNARY_MINUS:
		addBack("-(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")", basic);
		break;

	case HirCCallExpression::HIRC_SIZE_OF:			// sizeof() operator
		addBack("sizeof(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")", basic);
		break;

	case HirCCallExpression::HIRC_TAKE_ADDRESS:		// &
		addBack("&(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")", basic);
		break;

	case HirCCallExpression::HIRC_DE_REFERENCE:		// *
		addBack("*(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")", basic);
		break;

	case HirCCallExpression::HIRC_PREFIX_PLUS_PLUS:
		addBack("++", basic);
		basic.getArgument(0).accept(*this);
		break;
	case HirCCallExpression::HIRC_POSTFIX_PLUS_PLUS:
		basic.getArgument(0).accept(*this);
		addBack("++", basic);
		break;
	case HirCCallExpression::HIRC_PREFIX_MINUS_MINUS:
		addBack("--", basic);
		basic.getArgument(0).accept(*this);
		break;
	case HirCCallExpression::HIRC_POSTFIX_MINUS_MINUS:
		basic.getArgument(0).accept(*this);
		addBack("--", basic);
		break;

		//	Binary
	case HirCCallExpression::HIRC_BINARY_PLUS:		// +
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")+(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;
	case HirCCallExpression::HIRC_BINARY_MINUS:		// -
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")-(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;
	case HirCCallExpression::HIRC_MULTIPLY:			// *
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")*(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;
	case HirCCallExpression::HIRC_DIVISION:			// /
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")/(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;
		//	case BCK_INTEGER_DIVISION:	// div
		//		return "div", basic);
	case HirCCallExpression::HIRC_INTEGER_MOD:		// mod (%)
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")%(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;

		//	Assign
	case HirCCallExpression::HIRC_ASSIGN:				// =
		basic.getArgument(0).accept(*this);
		addBack(" = ", basic);
		basic.getArgument(1).accept(*this);
		break;

	case HirCCallExpression::HIRC_PLUS_ASSIGN:				// +=
		basic.getArgument(0).accept(*this);
		addBack("+=", basic);
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_MINUS_ASSIGN:				// -=
		basic.getArgument(0).accept(*this);
		addBack("-=", basic);
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_MULTIPLY_ASSIGN:			// *=
		basic.getArgument(0).accept(*this);
		addBack("*=", basic);
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_DIVISION_ASSIGN:			// /=
		basic.getArgument(0).accept(*this);
		addBack("/=", basic);
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_MOD_ASSIGN:				// %=
		basic.getArgument(0).accept(*this);
		addBack("%=", basic);
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_LSHIFT_ASSIGN:				// <<=
		basic.getArgument(0).accept(*this);
		addBack("<<=", basic);
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_RSHIFT_ASSIGN:				// >>=
		basic.getArgument(0).accept(*this);
		addBack(">>=", basic);
		basic.getArgument(1).accept(*this);
		break;

	case HirCCallExpression::HIRC_BAND_ASSIGN:				// &=
		basic.getArgument(0).accept(*this);
		addBack("&=", basic);
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_BOR_ASSIGN:				// |=
		basic.getArgument(0).accept(*this);
		addBack("|=", basic);
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_BXOR_ASSIGN:				// ^=
		basic.getArgument(0).accept(*this);
		addBack("^=", basic);
		basic.getArgument(1).accept(*this);
		break;

		//	Equality
	case HirCCallExpression::HIRC_LESS:				// <
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")<(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;
	case HirCCallExpression::HIRC_GREATER:			// >
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")>(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;
	case HirCCallExpression::HIRC_LESS_EQUAL:		// <=
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")<=(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;
	case HirCCallExpression::HIRC_GREATER_EQUAL:		// >=
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")>=(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;
	case HirCCallExpression::HIRC_EQUAL:				// ==
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")==(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;
	case HirCCallExpression::HIRC_NOT_EQUAL:			// !=
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")!=(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;

		//	Shifts
	case HirCCallExpression::HIRC_LEFT_SHIFT:			// <<
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")<<(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;
	case HirCCallExpression::HIRC_RIGHT_SHIFT:		// >>
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")>>(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;

		//	Logical
	case HirCCallExpression::HIRC_LOGICAL_NOT:		// !
		addBack("!(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")", basic);
		break;
	case HirCCallExpression::HIRC_LOGICAL_AND:		// &&
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")&&(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;
	case HirCCallExpression::HIRC_LOGICAL_OR:			// ||
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")||(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;

		//	Bitwise
	case HirCCallExpression::HIRC_BITWISE_NOT:		// ~
		addBack("~(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")", basic);
		break;
	case HirCCallExpression::HIRC_BITWISE_AND:		// &
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")&(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;
	case HirCCallExpression::HIRC_BITWISE_OR:			// |
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")|(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;
	case HirCCallExpression::HIRC_BITWISE_XOR:		// ^
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")^(", basic);
		basic.getArgument(1).accept(*this);
		addBack(")", basic);
		break;
		//	Special
	case HirCCallExpression::HIRC_ARRAY_ACCESS:		// []
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(")[", basic);
		for(int i = 1; i < basic.getArgumentCount(); ++i)
		{
			if (i > 1)
				addBack(", ", basic);
			basic.getArgument(i).accept(*this);
		}
		addBack("]", basic);
		break;
	case HirCCallExpression::HIRC_COMMA:		// ,
		basic.getArgument(0).accept(*this);
		addBack(", ", basic);
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_CONDITIONAL:		// ? :
		addBack("(", basic);
		basic.getArgument(0).accept(*this);
		addBack(") ? (", basic);
		basic.getArgument(1).accept(*this);
		addBack(") : (", basic);
		basic.getArgument(2).accept(*this);
		addBack(")", basic);
		break;
	OPS_DEFAULT_CASE_LABEL
	}
}

void ProgramTextBuilder::visit(Canto::HirFAltResultExpression& resultExpr)
{
	addBack(resultExpr.dumpState().c_str(), resultExpr);	
}

void ProgramTextBuilder::visit(Canto::HirFAsteriskExpression& asteriskExpr)
{
	addBack("*", asteriskExpr);
}

void ProgramTextBuilder::visit(Canto::HirFDimensionExpression& dimExpr)
{
	addBack("(", dimExpr);
	dimExpr.getLowerBound().accept(*this);
	addBack(":", dimExpr);
	dimExpr.getUpperBound().accept(*this);
	addBack(")", dimExpr);
}

void ProgramTextBuilder::visit(Canto::HirFArrayShapeExpression& shapeExpr)
{
	addBack("[", shapeExpr);
	int rank = shapeExpr.getRank();
	for(int i = 0; i < rank; ++i)
	{
		shapeExpr.getDimension(i).accept(*this);
		if (i != rank - 1)
			addBack(", ", shapeExpr);
	}
	addBack("]", shapeExpr);
}

void ProgramTextBuilder::visit(Canto::HirFArrayIndexRangeExpression& indexRangeExpr)
{
	addBack("(", indexRangeExpr);
	const int argCount = indexRangeExpr.getArgumentCount();
	for(int index = 0; index < argCount; ++index)
	{
		indexRangeExpr.getArgument(index).accept(*this);
		if (index != argCount - 1)
			addBack(", ", indexRangeExpr);
	}
	addBack(")", indexRangeExpr);
}
void ProgramTextBuilder::visit(Canto::HirFImpliedDoExpression& doExpr)
{
	addBack("(", doExpr);
	doExpr.getBodyExpression().accept(*this);
	addBack(", ", doExpr);
	doExpr.getInitExpression().accept(*this);
	addBack(", ", doExpr);
	doExpr.getFinalExpression().accept(*this);
	addBack(", ", doExpr);
	doExpr.getStepExpression().accept(*this);	
	addBack(")", doExpr);
}

void ProgramTextBuilder::visit(Canto::HirFArgumentPairExpression& argPairExpr)
{
	addBack((argPairExpr.getName() + " = ").c_str(), argPairExpr);
	argPairExpr.getValue().accept(*this);
}

void ProgramTextBuilder::visit(Canto::HirFIntrinsicCallExpression& call)
{
	addBack((Canto::HirFIntrinsicCallExpression::intrinsicKindToString(call.getKind()) + " (").c_str(), call);
	for (int index = 0; index < call.getArgumentCount(); ++index)
	{
		if (index > 0)
			addBack(", ", call);
		call.getArgument(index).accept(*this);
	}
	addBack(")", call);
}
/*
void ProgramTextBuilder::visit(Canto::HirFIntrinsicReferenceExpression&)
{
}
*/

void ProgramTextBuilder::outLabel(OPS::Reprise::StatementBase &stmt)
{
	if (stmt.hasLabel())
	{
		newLine((stmt.getLabel() + ":").c_str(), stmt);
	}

	StmtToCaseMap::const_iterator labeled = m_stmtToCase.find(&stmt);
	if (labeled != m_stmtToCase.end())
	{
		decreaseIndent();
		for (CasesList::const_iterator caseIt = labeled->second.begin(); caseIt != labeled->second.end(); ++caseIt)
		{
			if (!(*caseIt)->isDefault())
			{
				newLine("case ", stmt);
				addBack(OPS::Strings::format("%d:", (*caseIt)->getValue()).c_str(), stmt);
			}
			else
			{
				newLine("default:", stmt);
			}
		}
		increaseIndent();
	}
}

void ProgramTextBuilder::addBack(const char* text, RepriseBase& node)
{
	m_stream->write(text, node);
}

void ProgramTextBuilder::newLine(const char* text, RepriseBase& node)
{
	m_stream->writeNewLine((m_indention + text).c_str(), node);
}

void ProgramTextBuilder::increaseIndent()
{
	m_indention.append(2, ' ');
}

void ProgramTextBuilder::decreaseIndent()
{
	m_indention.resize(m_indention.size() - 2);
}
