#include "Backends/RepriseXml/RepriseXml.h"
#include "Reprise/Layouts.h"

using namespace OPS;
using namespace OPS::Reprise;
using namespace OPS::Reprise::Canto;

namespace OPS
{
namespace Backends
{


RepriseXml::RepriseXml(XmlBuilder& builder, const RepriseXml::Options& options) : m_builder(&builder), m_options(options)
{
}

void RepriseXml::visit(ProgramUnit& programUnit)
{
	m_builder->writeStartElement("ProgramUnit");
	writeHelper(programUnit);
	for (int unit = 0; unit < programUnit.getUnitCount(); ++unit)
	{
		programUnit.getUnit(unit).accept(*this);
	}
	m_builder->writeEndElement();
}

void RepriseXml::visit(TranslationUnit& translationUnit)
{
	m_builder->writeStartElement("TranslationUnit");
	writeHelper(translationUnit);
	m_builder->writeAttribute("source_filename", translationUnit.getSourceFilename());
	m_builder->writeStartElement("Globals");
	translationUnit.getGlobals().accept(*this);
	m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(Declarations& declarations)
{
	m_builder->writeStartElement("Declarations");
	writeHelper(declarations);
	for (Declarations::Iterator it = declarations.getFirst(); it.isValid(); ++it)
	{
		it->accept(*this);
	}
	m_builder->writeEndElement();
}

void RepriseXml::visit(VariableDeclaration& variable)
{
	m_builder->writeStartElement("VariableDeclaration");
		writeHelper(variable);
		m_builder->writeAttribute("name", variable.getName());
		m_builder->writeAttribute("declarators", variable.getDeclarators().dump());
		m_builder->writeAttribute("has_parameter_reference", variable.hasParameterReference());
		if (variable.hasParameterReference())
		{
			m_builder->writeAttribute("parameter_reference_name", variable.getParameterReference().getName());
			if (m_options.writeNCID)
			{
				m_builder->writeAttribute("parameter_reference_ncid", variable.getParameterReference().getNCID());
			}
		}
		m_builder->writeAttribute("has_defined_block", variable.hasDefinedBlock());
		if (variable.hasDefinedBlock() && m_options.writeNCID)
		{
			m_builder->writeAttribute("defined_block_ncid", variable.getDefinedBlock().getNCID());
		}
		m_builder->writeStartElement("Type");
			variable.getType().accept(*this);
		m_builder->writeEndElement();
		m_builder->writeStartElement("InitExpression");
			variable.getInitExpression().accept(*this);
		m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(TypeDeclaration& typeDeclaration)
{
	m_builder->writeStartElement("TypeDeclaration");
	writeHelper(typeDeclaration);
	m_builder->writeAttribute("name", typeDeclaration.getName());
	m_builder->writeAttribute("extern", typeDeclaration.isExtern());
	m_builder->writeAttribute("static", typeDeclaration.isStatic());
	m_builder->writeStartElement("Type");
	typeDeclaration.getType().accept(*this);
	m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(SubroutineDeclaration& subroutine)
{
	m_builder->writeStartElement("SubroutineDeclaration");
		writeHelper(subroutine);
		m_builder->writeAttribute("name", subroutine.getName());
		m_builder->writeAttribute("declarators", subroutine.getDeclarators().dump());
		m_builder->writeAttribute("has_implementation", subroutine.hasImplementation());
        m_builder->writeAttribute("has_definition", subroutine.hasDefinition());
        if (subroutine.hasDefinition())
        {
            m_builder->writeAttribute("definition_ncid", subroutine.getDefinition().getNCID());
        }
		m_builder->writeStartElement("Type");
			subroutine.getType().accept(*this);
		m_builder->writeEndElement();
		if (subroutine.hasImplementation())
		{
			subroutine.getDeclarations().accept(*this);
			subroutine.getBodyBlock().accept(*this);
		}
	m_builder->writeEndElement();
}

void RepriseXml::visit(BasicType& basicType)
{
	m_builder->writeStartElement("BasicType");
	writeHelper(basicType);
	m_builder->writeAttribute("kind", BasicType::basicTypeToString(basicType.getKind(), true));
	m_builder->writeEndElement();
}

void RepriseXml::visit(PtrType& ptr)
{
	m_builder->writeStartElement("PtrType");
	writeHelper(ptr);
	m_builder->writeStartElement("PointedType");
	ptr.getPointedType().accept(*this);
	m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(TypedefType& typeDef)
{
	m_builder->writeStartElement("TypedefType");
	writeHelper(typeDef);
	m_builder->writeStartElement("BaseType");
	typeDef.getBaseType().accept(*this);
	m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(ArrayType& arrayType)
{
	m_builder->writeStartElement("ArrayType");
		writeHelper(arrayType);
		m_builder->writeAttribute("element_count", arrayType.getElementCount());
		m_builder->writeAttribute("full_type", arrayType.isFullType());
		m_builder->writeStartElement("CountExpression");
		if (arrayType.hasCountExpression())
		{
			arrayType.getCountExpression().accept(*this);
		}
		m_builder->writeEndElement();
		m_builder->writeStartElement("BaseType");
			arrayType.getBaseType().accept(*this);
		m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(StructMemberDescriptor& member)
{
	m_builder->writeStartElement("StructMemberDescriptor");
		writeHelper(member);
		m_builder->writeAttribute("name", member.getName());
		m_builder->writeAttribute("bits_limit", member.getBitsLimit());
		m_builder->writeStartElement("LimitExpression");
			if (member.hasLimitExpression())
			{
				member.getLimitExpression().accept(*this);
			}
		m_builder->writeEndElement();
		m_builder->writeStartElement("Type");
			member.getType().accept(*this);
		m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(StructType& structType)
{
	m_builder->writeStartElement("StructType");
	writeHelper(structType);
	m_builder->writeAttribute("union", structType.isUnion());
	m_builder->writeAttribute("full_type", structType.isFullType());
	for (int index = 0; index < structType.getMemberCount(); ++index)
	{
		structType.getMember(index).accept(*this);
	}
	m_builder->writeEndElement();
}

void RepriseXml::visit(EnumMemberDescriptor& member)
{
	m_builder->writeStartElement("EnumMemberDescriptor");
	writeHelper(member);
	m_builder->writeAttribute("name", member.getName());
	m_builder->writeAttribute("value", member.getValue());
	m_builder->writeEndElement();
}

void RepriseXml::visit(EnumType& enumType)
{
	m_builder->writeStartElement("EnumType");
	writeHelper(enumType);
	m_builder->writeAttribute("full_type", enumType.isFullType());
	for (int index = 0; index < enumType.getMemberCount(); ++index)
	{
		enumType.getMember(index).accept(*this); 
	}
	m_builder->writeEndElement();
}

void RepriseXml::visit(ParameterDescriptor& parameter)
{
	m_builder->writeStartElement("ParameterDescriptor");
	writeHelper(parameter);
	m_builder->writeAttribute("name", parameter.getName());
	m_builder->writeAttribute("transit_kind", ParameterDescriptor::transitKindToString(parameter.getTransitKind()));
	m_builder->writeAttribute("optional", parameter.isOptional());
	m_builder->writeStartElement("Type");
		parameter.getType().accept(*this);
	m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(SubroutineType& subroutineType)
{
	m_builder->writeStartElement("SubroutineType");
		writeHelper(subroutineType);
		m_builder->writeAttribute("calling_kind", SubroutineType::getCallingText(subroutineType.getCallingKind()));
		m_builder->writeAttribute("vararg", subroutineType.isVarArg());
		m_builder->writeAttribute("args_known", subroutineType.isArgsKnown());
		for (int index = 0; index < subroutineType.getParameterCount(); ++index)
		{
			subroutineType.getParameter(index).accept(*this); 
		}
		m_builder->writeStartElement("ReturnType");
			subroutineType.getReturnType().accept(*this);
		m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(DeclaredType& declared)
{
	m_builder->writeStartElement("DeclaredType");
		writeHelper(declared);
		m_builder->writeAttribute("declared_name", declared.getDeclaration().getName());
		if (m_options.writeNCID)
		{
			m_builder->writeAttribute("declared_ncid", declared.getDeclaration().getNCID());
		}
	m_builder->writeEndElement();
}

void RepriseXml::visit(VectorType& vector)
{
    m_builder->writeStartElement("VectorType");
        writeHelper(vector);
        m_builder->writeAttribute("element_count", vector.getElementCount());
        m_builder->writeStartElement("BaseType");
            vector.getBaseType().accept(*this);
        m_builder->writeEndElement();
    m_builder->writeEndDocument();
}

void RepriseXml::visit(Canto::HirCBasicType& basicType)
{
	m_builder->writeStartElement("Canto_HirCBasicType");
	writeHelper(basicType);
	m_builder->writeAttribute("kind", HirCBasicType::basicKindToString(basicType.getKind(), true));
	m_builder->writeEndElement();
}

void RepriseXml::visit(Canto::HirFBasicType& basicType)
{
	m_builder->writeStartElement("Canto_HirBasicType");
		writeHelper(basicType);
		m_builder->writeAttribute("kind", HirFBasicType::basicKindToString(basicType.getKind(), true));
	m_builder->writeEndElement();
}

void RepriseXml::visit(Canto::HirFArrayType& arrayType)
{
	m_builder->writeStartElement("Canto_HirFArrayType");
		writeHelper(arrayType);
		m_builder->writeStartElement("Shape");
			arrayType.getShape().accept(*this);
		m_builder->writeEndElement();
		m_builder->writeStartElement("BaseType");
			arrayType.getBaseType().accept(*this);
		m_builder->writeEndElement();
	m_builder->writeEndElement();
}

//	RepriseXml - Expressions tree
void RepriseXml::visit(BasicLiteralExpression& literalExpr)
{
	m_builder->writeStartElement("BasicLiteralExpression");
		writeHelper(literalExpr);
		m_builder->writeAttribute("literal_type", BasicLiteralExpression::literalTypeToString(literalExpr.getLiteralType()));
		m_builder->writeAttribute("value", literalExpr.getLiteralValueAsString());
	m_builder->writeEndElement();
}

void RepriseXml::visit(StrictLiteralExpression& literalExpr)
{
	m_builder->writeStartElement("StrictLiteralExpression");
		writeHelper(literalExpr);
		m_builder->writeAttribute("literal_type", BasicType::basicTypeToString(literalExpr.getLiteralType()));
		m_builder->writeAttribute("value", literalExpr.getLiteralValueAsString());
	m_builder->writeEndElement();
}

void RepriseXml::visit(CompoundLiteralExpression& literalExpr)
{
	m_builder->writeStartElement("CompoundLiteralExpression");
		writeHelper(literalExpr);
		for (int i = 0; i < literalExpr.getValueCount(); ++i)
		{
			m_builder->writeStartElement("Expression");
			literalExpr.getValue(i).accept(*this);
			m_builder->writeEndElement();
		}
	m_builder->writeEndElement();
}

void RepriseXml::visit(ReferenceExpression& referenceExpr)
{
	m_builder->writeStartElement("ReferenceExpression");
		writeHelper(referenceExpr);
		m_builder->writeAttribute("referenced_name", referenceExpr.getReference().getName());
		if (m_options.writeNCID)
		{
			m_builder->writeAttribute("referenced_ncid", referenceExpr.getReference().getNCID());
		}
	m_builder->writeEndElement();
}

void RepriseXml::visit(SubroutineReferenceExpression& referenceExpr)
{
	m_builder->writeStartElement("SubroutineReferenceExpression");
		writeHelper(referenceExpr);
		m_builder->writeAttribute("referenced_name", referenceExpr.getReference().getName());
		if (m_options.writeNCID)
		{
			m_builder->writeAttribute("referenced_ncid", referenceExpr.getReference().getNCID());
		}
	m_builder->writeEndElement();
}

void RepriseXml::visit(StructAccessExpression& structAccess)
{
	m_builder->writeStartElement("StructAccessExpression");
		writeHelper(structAccess);
		m_builder->writeAttribute("member_name", structAccess.getMember().getName());
		if (m_options.writeNCID)
		{
			m_builder->writeAttribute("member_ncid", structAccess.getMember().getNCID());
		}
		m_builder->writeStartElement("StructPointer");
			structAccess.getStructPointerExpression().accept(*this);
		m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(EnumAccessExpression& enumAccess)
{
	m_builder->writeStartElement("EnumAccessExpression");
		writeHelper(enumAccess);
		m_builder->writeAttribute("member_name", enumAccess.getMember().getName());
		if (m_options.writeNCID)
		{
			m_builder->writeAttribute("member_ncid", enumAccess.getMember().getNCID());
		}
	m_builder->writeEndElement();
}

void RepriseXml::visit(TypeCastExpression& typeCast)
{
	m_builder->writeStartElement("TypeCastExpression");
		writeHelper(typeCast);
		m_builder->writeAttribute("implicit", typeCast.isImplicit());
		m_builder->writeStartElement("CastTo");
			typeCast.getCastType().accept(*this);
		m_builder->writeEndElement();
		m_builder->writeStartElement("Argument");
			typeCast.getCastArgument().accept(*this);
		m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(BasicCallExpression& basic)
{
	m_builder->writeStartElement("BasicCallExpression");
		writeHelper(basic);
		m_builder->writeAttribute("kind", BasicCallExpression::builtinCallKindToString(basic.getKind()));
		writeCallArgumentsHelper(basic);
	m_builder->writeEndElement();
}

void RepriseXml::visit(SubroutineCallExpression& subroutineCallExpression)
{
	m_builder->writeStartElement("SubroutineCallExpression");
		writeHelper(subroutineCallExpression);
		m_builder->writeAttribute("explicit_subroutine_declaration", subroutineCallExpression.hasExplicitSubroutineDeclaration());
		if (subroutineCallExpression.hasExplicitSubroutineDeclaration())
		{
			m_builder->writeAttribute("explicit_subroutine_name", subroutineCallExpression.getExplicitSubroutineDeclaration().getName());
			if (m_options.writeNCID)
			{
				m_builder->writeAttribute("explicit_subroutine_ncid", subroutineCallExpression.getExplicitSubroutineDeclaration().getNCID());
			}
		}
		m_builder->writeStartElement("CallExpression");
			subroutineCallExpression.getCallExpression().accept(*this);
		m_builder->writeEndElement();
		writeCallArgumentsHelper(subroutineCallExpression);
	m_builder->writeEndElement();
}

void RepriseXml::visit(EmptyExpression& empty)
{
	m_builder->writeStartElement("EmptyExpression");
		writeHelper(empty);
	m_builder->writeEndElement();
}

void RepriseXml::visit(Canto::HirCCallExpression& basic)
{
	m_builder->writeStartElement("Canto_HirCCallExpression");
		writeHelper(basic);
		m_builder->writeAttribute("kind", HirCCallExpression::callKindToString(basic.getKind()));
		writeCallArgumentsHelper(basic);
	m_builder->writeEndElement();
}

void RepriseXml::visit(Canto::HirFAsteriskExpression& expr)
{
	m_builder->writeStartElement("Canto_HirFAsteriskExpression");
		writeHelper(expr);
	m_builder->writeEndElement();
}

void RepriseXml::visit(Canto::HirFDimensionExpression& expr)
{
	m_builder->writeStartElement("Canto_HirFDimensionExpression");
		writeHelper(expr);
		m_builder->writeStartElement("LowerBound");
			expr.getLowerBound().accept(*this);
		m_builder->writeEndElement();
		m_builder->writeStartElement("UpperBound");
			expr.getUpperBound().accept(*this);
		m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(OPS::Reprise::Canto::HirFArrayShapeExpression& expr)
{
	m_builder->writeStartElement("Canto_HirFArrayShapeExpression");
		for(int i = 0; i < expr.getRank(); ++i)
		{
			expr.getDimension(i).accept(*this);
		}
	m_builder->writeEndElement();
}

void RepriseXml::visit(Canto::HirFImpliedDoExpression& expr)
{
	m_builder->writeStartElement("Canto_HirFImpliedDoExpression");
		writeHelper(expr);	
		m_builder->writeStartElement("Init");
			expr.getInitExpression().accept(*this);
		m_builder->writeEndElement();
		m_builder->writeStartElement("Final");
			expr.getFinalExpression().accept(*this);
		m_builder->writeEndElement();
		m_builder->writeStartElement("Step");
			expr.getStepExpression().accept(*this);
		m_builder->writeEndElement();
		m_builder->writeStartElement("Body");
			expr.getBodyExpression().accept(*this);
		m_builder->writeEndElement();	
	m_builder->writeEndElement();
}

void RepriseXml::visit(OPS::Reprise::Canto::HirFArgumentPairExpression& expr)
{
	m_builder->writeStartElement("Canto_HirFArgumentPairExpression");
		writeHelper(expr);
		m_builder->writeAttribute("name", expr.getName());
		m_builder->writeStartElement("Expression");
			expr.getValue().accept(*this);
		m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(Canto::HirFIntrinsicCallExpression& call)
{
	m_builder->writeStartElement("Canto_FIntrinsicCall");
		writeHelper(call);
		m_builder->writeAttribute("kind", call.getKind());
		writeCallArgumentsHelper(call);
	m_builder->writeEndElement();
}


//	RepriseXml - Statements visitors
void RepriseXml::visit(BlockStatement& block)
{
	m_builder->writeStartElement("BlockStatement");
		writeHelper(block);
		for (BlockStatement::Iterator it = block.getFirst(); it.isValid(); ++it)
		{
			it->accept(*this);
		}
	m_builder->writeEndElement();
}

void RepriseXml::visit(ForStatement& forStmt)
{
	m_builder->writeStartElement("ForStatement");
		writeHelper(forStmt);
		m_builder->writeStartElement("Init");
			forStmt.getInitExpression().accept(*this);
		m_builder->writeEndElement();
		m_builder->writeStartElement("Final");
			forStmt.getFinalExpression().accept(*this);
		m_builder->writeEndElement();
		m_builder->writeStartElement("Step");
			forStmt.getStepExpression().accept(*this);
		m_builder->writeEndElement();
		m_builder->writeStartElement("Body");
			forStmt.getBody().accept(*this);
		m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(WhileStatement& whileStmt)
{
	m_builder->writeStartElement("WhileStatement");
		writeHelper(whileStmt);
		m_builder->writeAttribute("pre_condition", whileStmt.isPreCondition());
		m_builder->writeStartElement("Condition");
			whileStmt.getCondition().accept(*this);
		m_builder->writeEndElement();
		m_builder->writeStartElement("Body");
			whileStmt.getBody().accept(*this);
		m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(IfStatement& ifStmt)
{
	m_builder->writeStartElement("IfStatement");
		writeHelper(ifStmt);
		m_builder->writeStartElement("Condition");
			ifStmt.getCondition().accept(*this);
		m_builder->writeEndElement();
		m_builder->writeStartElement("Then");
			ifStmt.getThenBody().accept(*this);
		m_builder->writeEndElement();
		m_builder->writeStartElement("Else");
			ifStmt.getElseBody().accept(*this);
		m_builder->writeEndElement();
	m_builder->writeEndElement();
}

void RepriseXml::visit(PlainCaseLabel& caseLabel)
{
	m_builder->writeStartElement("PlainCaseLabel");
		writeHelper(caseLabel);
		m_builder->writeAttribute("value", Strings::format("%" OPS_CRT_FORMAT_LONG_LONG_PREFIX "i", caseLabel.getValue()));
		m_builder->writeAttribute("default", caseLabel.isDefault());
		if (m_options.writeNCID)
		{
			m_builder->writeAttribute("statement_ncid", caseLabel.getStatement().getNCID());
		}
	m_builder->writeEndElement();
}

void RepriseXml::visit(PlainSwitchStatement& switchStmt)
{
	m_builder->writeStartElement("PlainSwitchStatement");
		writeHelper(switchStmt);
		m_builder->writeStartElement("Condition");
			switchStmt.getCondition().accept(*this);
		m_builder->writeEndElement();
		m_builder->writeStartElement("Body");
			switchStmt.getBody().accept(*this);
		m_builder->writeEndElement();
		for (int i = 0; i < switchStmt.getLabelCount(); ++i)
		{
			switchStmt.getLabel(i).accept(*this);
		}
	m_builder->writeEndElement();
}

void RepriseXml::visit(GotoStatement& gotoStmt)
{
	m_builder->writeStartElement("GotoStatement");
		writeHelper(gotoStmt);
		if (gotoStmt.getPointedStatement() != 0)
		{
			m_builder->writeAttribute("pointed_label", gotoStmt.getPointedStatement()->getLabel());
			if (m_options.writeNCID)
			{
				m_builder->writeAttribute("pointed_ncid", gotoStmt.getPointedStatement()->getNCID());
			}
		}
		else
		{
			m_builder->writeAttribute("pointed_label", 0);
		}
	m_builder->writeEndElement();
}

void RepriseXml::visit(ReturnStatement& returnStmt)
{
	m_builder->writeStartElement("ReturnStatement");
		writeHelper(returnStmt);
		returnStmt.getReturnExpression().accept(*this);
	m_builder->writeEndElement();
}

void RepriseXml::visit(Canto::HirBreakStatement& brkStmt)
{
    m_builder->writeStartElement("Canto_HirBreakStatement");
		writeHelper(brkStmt);
	m_builder->writeEndElement();
}

void RepriseXml::visit(Canto::HirContinueStatement& continueStmt)
{
    m_builder->writeStartElement("Canto_HirContinueStatement");
		writeHelper(continueStmt);
	m_builder->writeEndElement();
}

void RepriseXml::visit(ExpressionStatement& expr)
{
	m_builder->writeStartElement("ExpressionStatement");
		writeHelper(expr);
		expr.get().accept(*this);
	m_builder->writeEndElement();
}

void RepriseXml::visit(EmptyStatement& emptyStmt)
{
	m_builder->writeStartElement("EmptyStatement");
		writeHelper(emptyStmt);
	m_builder->writeEndElement();
}

void RepriseXml::visit(ASMStatement& asmStmt)
{
	m_builder->writeStartElement("ASMStatement");
		writeHelper(asmStmt);
		m_builder->writeAttribute("string", asmStmt.getASMString());
		switch (asmStmt.getASMType()) {
		case ASMStatement::InlineASMType::ASMTP_MS:
			m_builder->writeAttribute("asm_type", std::string("ASMTP_MS"));
			break;
		case ASMStatement::InlineASMType::ASMTP_GCC:
			m_builder->writeAttribute("asm_type", std::string("ASMTP_GCC"));
			break;
		}
	m_builder->writeEndElement();
}


//	RepriseXml - private methods
void RepriseXml::writeHelper(RepriseBase& repriseBase)
{
	if (m_options.writeNCID)
	{
		m_builder->writeAttribute("ncid", repriseBase.getNCID());
		if (m_options.writeNCIDofParent)
		{
			if (repriseBase.getParent() != 0)
			{
				m_builder->writeAttribute("ncid_of_parent", repriseBase.getParent()->getNCID());
			}
			else
			{
				m_builder->writeAttribute("ncid_of_parent", 0);
			}
		}
	}
	if (m_options.writeSourceCodeLocation)
	{
		SourceCodeManager& manager = RepriseContext::defaultContext().getSourceCodeManager();
		SourceCodeLocation location = manager.getLocation(repriseBase);
		if (location.FileId != -1)
		{
			std::string filePath = manager.getFilePath(location.FileId);
			m_builder->writeAttribute("location", OPS::Strings::format("%s:(r%d,c%d)-(r%d,c%d)",
																	   filePath.c_str(),
																	   location.Row.first,
																	   location.Column.first,
																	   location.Row.second,
																	   location.Column.second));
		}
		else
		{
			m_builder->writeAttribute("location", std::string("unknown"));
		}
	}
}

void RepriseXml::writeHelper(TypeBase& typeBase)
{
	writeHelper(static_cast<RepriseBase&>(typeBase));
	m_builder->writeAttribute("const", typeBase.isConst());
	m_builder->writeAttribute("volatile", typeBase.isVolatile());
}

void RepriseXml::writeHelper(StatementBase& stmtBase)
{
	writeHelper(static_cast<RepriseBase&>(stmtBase));
	if (stmtBase.hasLabel())
	{
		m_builder->writeAttribute("label_name", stmtBase.getLabel());
	}
}

void RepriseXml::writeCallArgumentsHelper(CallExpressionBase& callExprBase)
{
	for (int i = 0; i < callExprBase.getArgumentCount(); ++i)
	{
		m_builder->writeStartElement("Argument");
		callExprBase.getArgument(i).accept(*this);
		m_builder->writeEndElement();
	}
}

}
}
