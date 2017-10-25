/*
	Backends/OutToC/OutToC.cpp - out to C backend implementation

	Known issues:
		* Переменные и функции выводятся в порядке объявления без учета правила предварительного объявления.
		* Таблица перекодировки BasicType->HirC жестко зашита. Нужна возможность изменения.
*/

//  Standard includes
#include <sstream>
#include <limits>

//  OPS includes
#include "Backends/OutToC/OutToC.h"
#include "Reprise/Collections.h"
#include "Reprise/Layouts.h"
#include "Analysis/DeadDeclarations.h"
#include "OPS_Core/ExternalCompiler.h"

//  Local includes

//  Namespaces using
using namespace OPS::Reprise;
using namespace OPS::Reprise::Canto;

//  Defines and macros

//  Enter namespace
namespace OPS
{
namespace Backends
{
//  Constants and enums

//  Classes

class IncludeManager
{
	SourceCodeManager* m_sourceManager;

public:

	IncludeManager(SourceCodeManager& sourceManager)
		:m_sourceManager(&sourceManager)
	{
	}

	static bool isIncludeFile(const std::string &name)
	{
		const size_t extPos = name.find_last_of('.');
		if (extPos != std::string::npos)
		{
			std::string ext = name.substr(extPos + 1);
			return ext != "c";
		}
		else
		{
			// file have no extension
			return false;
		}
	}

    /// Возвращает путь к ПЕРВОМУ в дереве инклюдов include-файлу, если узел находится в include-файле,
	/// иначе - возвращает пустую строку
	std::string getIncludeName(Reprise::RepriseBase& node)
	{
		SourceCodeLocation location = m_sourceManager->getLocation(node);
        if (location.HeadIncludeFileId != -1)
		{
            std::string filePath = m_sourceManager->getFilePath(location.HeadIncludeFileId);
			if (isIncludeFile(filePath))
			{
                if (filePath.find("polybench.h") != std::string::npos) return "<polybench.h>";
                std::string incPath;
                bool stdInclude = false;
                // определяем, является ли стандартным инклюдом и оборачиваем в угловые кавычки
                std::list<std::string> incDirs = OPS::ExternalCompiler::getGCCorMSVSIncludePaths();
                for(std::list<std::string>::iterator it = incDirs.begin(); it != incDirs.end(); ++it)
                {
                    std::string& dir = *it;
                    std::string::size_type pos = filePath.find(dir);
                    if (pos != std::string::npos)
                    {
                        stdInclude = true;
                        incPath = "<" + filePath.substr(pos+dir.size()+1) + ">";
                        break;
                    }
                }
                if (!stdInclude) incPath = "\"" + filePath + "\"";
                return incPath;
			}
		}
		return std::string();
	}
};

//  Functions declaration

//  Variables

//  Classes implementation

//  Global classes implementation

OutToCSettings::OutToCSettings()
	:hideBuiltins(true)
	,useIncludes(false)
	,prettyExpressions(true)
	,usedDeclarations(0)
{
	typeMap[BasicType::BT_UNDEFINED] = HirCBasicType::HCBK_UNDEFINED;
	typeMap[BasicType::BT_CHAR] = HirCBasicType::HCBK_CHAR;
	typeMap[BasicType::BT_WIDE_CHAR] = HirCBasicType::HCBK_WIDE_CHAR;
	typeMap[BasicType::BT_INT8] = HirCBasicType::HCBK_SCHAR;
	typeMap[BasicType::BT_INT16] = HirCBasicType::HCBK_SHORT;
	typeMap[BasicType::BT_INT32] = HirCBasicType::HCBK_INT;
	typeMap[BasicType::BT_INT64] = HirCBasicType::HCBK_LONG_LONG;
    typeMap[BasicType::BT_INT128] = HirCBasicType::HCBK_INT128;
	typeMap[BasicType::BT_UINT8] = HirCBasicType::HCBK_UCHAR;
	typeMap[BasicType::BT_UINT16] = HirCBasicType::HCBK_USHORT;
	typeMap[BasicType::BT_UINT32] = HirCBasicType::HCBK_UINT;
	typeMap[BasicType::BT_UINT64] = HirCBasicType::HCBK_ULONG_LONG;
    typeMap[BasicType::BT_UINT128] = HirCBasicType::HCBK_UINT128;
	typeMap[BasicType::BT_FLOAT32] = HirCBasicType::HCBK_FLOAT;
	typeMap[BasicType::BT_FLOAT64] = HirCBasicType::HCBK_DOUBLE;
	typeMap[BasicType::BT_BOOLEAN] = HirCBasicType::HCBK_BOOL;
	typeMap[BasicType::BT_STRING] = HirCBasicType::HCBK_UNDEFINED;
	typeMap[BasicType::BT_WIDE_STRING] = HirCBasicType::HCBK_UNDEFINED;
	typeMap[BasicType::BT_VOID] = HirCBasicType::HCBK_VOID;
}

OutToC::OutToC(std::ostream& outStream, unsigned int indention)
	:m_indentionSize(indention)
	,m_outStream(outStream)
	,m_includeManager(new IncludeManager(RepriseContext::defaultContext().getSourceCodeManager()))
{
	m_outStream.imbue(std::locale::classic()); // use "C" locale
	m_outStream.precision(std::numeric_limits<double>::digits10 + 2); // maximum precision
	m_outStream << std::showpoint;
}

OutToC::~OutToC()
{
	delete m_includeManager;
}

OutToCSettings& OutToC::getSettings()
{
	return m_settings;
}

const OutToCSettings& OutToC::getSettings() const
{
	return m_settings;
}

std::string OutToC::expressionToString(const OPS::Reprise::ExpressionBase &expr)
{
	std::stringstream ss;
	OutToC out(ss);
	const_cast<ExpressionBase&>(expr).accept(out);
	return ss.str();
}

int OutToC::getExpressionPriority(OPS::Reprise::ExpressionBase& expr)
{
	if (BasicCallExpression* basicExpr = expr.cast_ptr<BasicCallExpression>())
	{
		switch(basicExpr->getKind())
		{
		case BasicCallExpression::BCK_UNARY_PLUS: return 2;
		case BasicCallExpression::BCK_UNARY_MINUS: return 2;
		case BasicCallExpression::BCK_SIZE_OF: return 2;
		case BasicCallExpression::BCK_TAKE_ADDRESS: return 2;
		case BasicCallExpression::BCK_DE_REFERENCE: return 2;
		case BasicCallExpression::BCK_BINARY_PLUS: return 4;
		case BasicCallExpression::BCK_BINARY_MINUS: return 4;
		case BasicCallExpression::BCK_MULTIPLY: return 3;
		case BasicCallExpression::BCK_DIVISION: return 3;
		case BasicCallExpression::BCK_INTEGER_DIVISION: return 3;
		case BasicCallExpression::BCK_INTEGER_MOD: return 3;
		case BasicCallExpression::BCK_ASSIGN: return 14;
		case BasicCallExpression::BCK_LESS: return 6;
		case BasicCallExpression::BCK_GREATER: return 6;
		case BasicCallExpression::BCK_LESS_EQUAL: return 6;
		case BasicCallExpression::BCK_GREATER_EQUAL: return 6;
		case BasicCallExpression::BCK_EQUAL: return 7;
		case BasicCallExpression::BCK_NOT_EQUAL: return 7;
		case BasicCallExpression::BCK_LEFT_SHIFT: return 5;
		case BasicCallExpression::BCK_RIGHT_SHIFT: return 5;
		case BasicCallExpression::BCK_LOGICAL_NOT: return 2;
		case BasicCallExpression::BCK_LOGICAL_AND: return 11;
		case BasicCallExpression::BCK_LOGICAL_OR: return 12;
		case BasicCallExpression::BCK_BITWISE_NOT: return 2;
		case BasicCallExpression::BCK_BITWISE_AND: return 8;
		case BasicCallExpression::BCK_BITWISE_OR: return 10;
		case BasicCallExpression::BCK_BITWISE_XOR: return 9;
		case BasicCallExpression::BCK_ARRAY_ACCESS: return 1;
		case BasicCallExpression::BCK_COMMA: return 15;
		case BasicCallExpression::BCK_CONDITIONAL: return 13;
		OPS_DEFAULT_CASE_LABEL
		}
	}
	else if (HirCCallExpression* hircExpr = expr.cast_ptr<HirCCallExpression>())
	{
		switch(hircExpr->getKind())
		{
		case HirCCallExpression::HIRC_UNARY_PLUS: return 2;
		case HirCCallExpression::HIRC_UNARY_MINUS: return 2;
		case HirCCallExpression::HIRC_SIZE_OF: return 2;
		case HirCCallExpression::HIRC_TAKE_ADDRESS: return 2;
		case HirCCallExpression::HIRC_DE_REFERENCE: return 2;
		case HirCCallExpression::HIRC_PREFIX_PLUS_PLUS: return 2;
		case HirCCallExpression::HIRC_POSTFIX_PLUS_PLUS: return 1;
		case HirCCallExpression::HIRC_PREFIX_MINUS_MINUS: return 2;
		case HirCCallExpression::HIRC_POSTFIX_MINUS_MINUS: return 1;
		case HirCCallExpression::HIRC_BINARY_PLUS: return 4;
		case HirCCallExpression::HIRC_BINARY_MINUS: return 4;
		case HirCCallExpression::HIRC_MULTIPLY: return 3;
		case HirCCallExpression::HIRC_DIVISION: return 3;
		case HirCCallExpression::HIRC_INTEGER_MOD: return 3;
		case HirCCallExpression::HIRC_ASSIGN: return 14;
		case HirCCallExpression::HIRC_PLUS_ASSIGN: return 14;
		case HirCCallExpression::HIRC_MINUS_ASSIGN: return 14;
		case HirCCallExpression::HIRC_MULTIPLY_ASSIGN: return 14;
		case HirCCallExpression::HIRC_DIVISION_ASSIGN: return 14;
		case HirCCallExpression::HIRC_MOD_ASSIGN: return 14;
		case HirCCallExpression::HIRC_LSHIFT_ASSIGN: return 14;
		case HirCCallExpression::HIRC_RSHIFT_ASSIGN: return 14;
		case HirCCallExpression::HIRC_BAND_ASSIGN: return 14;
		case HirCCallExpression::HIRC_BOR_ASSIGN: return 14;
		case HirCCallExpression::HIRC_BXOR_ASSIGN: return 14;
		case HirCCallExpression::HIRC_LESS: return 6;
		case HirCCallExpression::HIRC_GREATER: return 6;
		case HirCCallExpression::HIRC_LESS_EQUAL: return 6;
		case HirCCallExpression::HIRC_GREATER_EQUAL: return 6;
		case HirCCallExpression::HIRC_EQUAL: return 7;
		case HirCCallExpression::HIRC_NOT_EQUAL: return 7;
		case HirCCallExpression::HIRC_LEFT_SHIFT: return 5;
		case HirCCallExpression::HIRC_RIGHT_SHIFT: return 5;
		case HirCCallExpression::HIRC_LOGICAL_NOT: return 2;
		case HirCCallExpression::HIRC_LOGICAL_AND: return 11;
		case HirCCallExpression::HIRC_LOGICAL_OR: return 12;
		case HirCCallExpression::HIRC_BITWISE_NOT: return 2;
		case HirCCallExpression::HIRC_BITWISE_AND: return 8;
		case HirCCallExpression::HIRC_BITWISE_OR: return 10;
		case HirCCallExpression::HIRC_BITWISE_XOR: return 9;
		case HirCCallExpression::HIRC_ARRAY_ACCESS: return 1;
		case HirCCallExpression::HIRC_COMMA: return 15;
		case HirCCallExpression::HIRC_CONDITIONAL: return 13;
		OPS_DEFAULT_CASE_LABEL
		}
	}
	else if (expr.is_a<TypeCastExpression>())
	{
		return 2;
	}
	else if (expr.is_a<SubroutineCallExpression>() || expr.cast_ptr<StructAccessExpression>())
	{
		return 1;
	}

	return 0;
}


void OutToC::increaseIndent()
{
	m_indent += std::string(m_indentionSize, ' ');
}

void OutToC::decreaseIndent()
{
	if (m_indent.size() > 0)
		m_indent.erase(m_indent.size()-m_indentionSize);
}

void OutToC::outLabel(StatementBase& statement, bool needStmt)
{
	if (statement.hasLabel())
	{
		newLine(statement.getLabel());
		m_outStream << ": ";
		if (needStmt)
			m_outStream << ";";
	}

	StmtToCaseMap::const_iterator labeled = m_stmtToCase.find(&statement);
	if (labeled != m_stmtToCase.end())
	{
		decreaseIndent();
		for (CasesList::const_iterator caseIt = labeled->second.begin(); caseIt != labeled->second.end(); ++caseIt)
		{
			if (!(*caseIt)->isDefault())
			{
				newLine("case ");
				m_outStream << (*caseIt)->getValue() << ":";
			}
			else
			{
				newLine("default:");
			}
		}
		increaseIndent();
	}
}

const std::string& OutToC::getIndent() const
{
	return m_indent;
}

void OutToC::newLine(const std::string& text)
{
	m_outStream << std::endl;

	if (m_indentionSize)
		m_outStream << getIndent();

	m_outStream << text;
}

bool OutToC::exprNeedBraces(Reprise::ExpressionBase &expr)
{
	if (m_settings.prettyExpressions == false)
		return true;

	if (RepriseBase* parent = expr.getParent())
	{
		if (ExpressionBase* parentExpr = parent->cast_ptr<ExpressionBase>())
		{
			int parentPriority = OutToC::getExpressionPriority(*parentExpr),
					exprPriority = OutToC::getExpressionPriority(expr);
			return  parentPriority <= exprPriority;
		}
	}

	return false;
}

bool OutToC::isUsed(Reprise::DeclarationBase &decl)
{
	return m_settings.usedDeclarations == 0 ||
		   m_settings.usedDeclarations->find(&decl) != m_settings.usedDeclarations->end();
}

void OutToC::visit(TranslationUnit& unit)
{
	unit.getGlobals().accept(*this);
}

static bool isBuiltin(const std::string& name)
{
    return 0 == name.find("__builtin") ||
           name == "__int128_t" ||
           name == "__uint128_t";
}

void OutToC::visit(Declarations& declarations)
{
	for(Declarations::Iterator it = declarations.getFirst(); it.isValid(); ++it)
	{
		bool skipDecl = false;

		if (m_settings.hideBuiltins)
		{
			if (isBuiltin(it->getName()))
				skipDecl = true;
		}
        if (m_settings.hideBuiltins && it->hasNote("runtime")) continue;

        if (m_settings.useIncludes)
		{
			std::string includeName = m_includeManager->getIncludeName(*it);
			if (!includeName.empty())
			{
				if (m_includedFiles.find(includeName) == m_includedFiles.end())
				{
					newLine("#include " + includeName);
					m_includedFiles.insert(includeName);
				}
				skipDecl = true;
			}
		}

		if (skipDecl != true)
			it->accept(*this);
	}
}

void OutToC::visit(VariableDeclaration& variable)
{
	if (!isUsed(variable) && !variable.hasNonEmptyInitExpression())
		return;

	newLine();

	const std::string declList = VariableDeclarators::declListOfFlagsToString(variable.getDeclarators().getDeclaration());

	if (!declList.empty())
	{
		m_outStream << declList << " ";
	}

	m_outStream << composeDecl(variable.getName(), variable.getType());

	if (variable.hasNonEmptyInitExpression())
	{
		m_outStream << " = ";
		variable.getInitExpression().accept(*this);
	}
	m_outStream << ";";
}

//"переводит" определение новых типов
void OutToC::visit(TypeDeclaration& typeDecl)
{
	if (!isUsed(typeDecl))
		return;

	newLine();

	if (typeDecl.isExtern())
		m_outStream << "extern ";
	if (typeDecl.isStatic())
		m_outStream << "static ";

	m_outStream << composeDecl(typeDecl.getName(), typeDecl.getType()) << ";";
}

std::string OutToC::composeDecl(const std::string &name, Reprise::TypeBase &itemType)
{
	m_declStack.push(name);
	itemType.accept(*this);
	std::string decl = m_declStack.top();
	m_declStack.pop();
	return decl;
}

void OutToC::printTypeOnly(Reprise::TypeBase& t)
{
    m_declStack.push("");
    t.accept(*this);
    std::string decl = m_declStack.top();
    m_declStack.pop();
    m_outStream << decl;
}

void OutToC::outConstVolatile(TypeBase &typeBase)
{
	if (typeBase.isConst() && !typeBase.is_a<ArrayType>())
		m_declStack.top() = "const " + m_declStack.top();
	if (typeBase.isVolatile())
		m_declStack.top() = "volatile " + m_declStack.top();
}

void OutToC::visit(SubroutineDeclaration& body)
{
	if (!isUsed(body))
		return;

	// Пустая строка перед каждой функцией
	newLine();

	std::string flags = VariableDeclarators::declListOfFlagsToString(body.getDeclarators().getDeclaration());

	if (!flags.empty())
		m_outStream << flags << " ";

	m_outStream << composeDecl(body.getName(), body.getType());

	// Если у функции есть реализация, то выводим её
	if (body.hasImplementation())
	{
		body.getBodyBlock().accept(*this);
	}
	else
	{
		m_outStream << ";";
	}
}

void OutToC::visit(BlockStatement& block)
{
	outLabel(block, true);
	newLine("{");
	increaseIndent();

	// Выводим описания переменных и типов
	Declarations& declarations = block.getDeclarations();
	const bool isRootBlock = &block == &block.getRootBlock();

	for(Declarations::Iterator it = declarations.getFirst(); it.isValid(); ++it)
	{
		if (it->is_a<VariableDeclaration>())
		{
			VariableDeclaration& varDecl = it->cast_to<VariableDeclaration>();
			// Если переменная объявлена в этом блоке
			if (varDecl.hasDefinedBlock() && (&varDecl.getDefinedBlock() == &block))
			{
				varDecl.accept(*this);
			}
		}
		else if (isRootBlock) // it is a root block
		{
			it->accept(*this);
		}
	}

	// Выводим операторы
	for (BlockStatement::Iterator iter = block.getFirst(); iter.isValid(); iter.goNext())
	{
		iter->accept(*this);
	}

	decreaseIndent();
	newLine("}");
}

void OutToC::visit(ForStatement& forStmt)
{
	outLabel(forStmt);
	newLine("for (");
	forStmt.getInitExpression().accept(*this);
	m_outStream << "; ";
	forStmt.getFinalExpression().accept(*this);
	m_outStream << "; ";
	forStmt.getStepExpression().accept(*this);
	m_outStream << ")";
	forStmt.getBody().accept(*this);
}

void OutToC::visit(WhileStatement& whileStmt)
{
	outLabel(whileStmt);
	if (whileStmt.isPreCondition())
	{
		newLine("while (");
		whileStmt.getCondition().accept(*this);
		m_outStream << ")";
		whileStmt.getBody().accept(*this);
	}
	else
	{
		newLine("do");
		whileStmt.getBody().accept(*this);
		m_outStream << "while (";
		whileStmt.getCondition().accept(*this);
		m_outStream << ");";
	}
}

void OutToC::visit(PlainCaseLabel& caseLabel)
{
	m_stmtToCase[&caseLabel.getStatement()].push_back(&caseLabel);
}

void OutToC::visit(PlainSwitchStatement& switchStmt)
{
	outLabel(switchStmt);
	newLine("switch (");
	switchStmt.getCondition().accept(*this);
	m_outStream << ")";

	for(int i = 0; i < switchStmt.getLabelCount(); ++i)
		switchStmt.getLabel(i).accept(*this);

	switchStmt.getBody().accept(*this);
}

void OutToC::visit(IfStatement& ifStmt)
{
	outLabel(ifStmt);
	newLine("if (");
	ifStmt.getCondition().accept(*this);
	m_outStream << ")";
	ifStmt.getThenBody().accept(*this);

	if (!ifStmt.getElseBody().isEmpty())
	{
		newLine("else");
		ifStmt.getElseBody().accept(*this);
	}
}

void OutToC::visit(GotoStatement& gotoStmt)
{
	outLabel(gotoStmt);
	newLine("goto ");
	OPS_ASSERT(gotoStmt.getPointedStatement() != 0)
	m_outStream << gotoStmt.getPointedStatement()->getLabel() << ";";
}

void OutToC::visit(ReturnStatement& returnStmt)
{
	outLabel(returnStmt);
	newLine("return ");
	returnStmt.getReturnExpression().accept(*this);
	m_outStream << ";";
}

void OutToC::visit(Canto::HirBreakStatement& brkStmt)
{
	outLabel(brkStmt);
	newLine("break;");
}

void OutToC::visit(Canto::HirContinueStatement& continueStmt)
{
	outLabel(continueStmt);
	newLine("continue;");
}

void OutToC::visit(Canto::HirCVariableInitStatement& varInit)
{
    outLabel(varInit);
    newLine(varInit.getVariableDeclaration().getName());
    m_outStream << " = ";
    varInit.getInitExpression().accept(*this);
    m_outStream << ";";
}

void OutToC::visit(ExpressionStatement& expr)
{
	outLabel(expr);
	newLine();
	expr.get().accept(*this);
	m_outStream << ";";
}

void OutToC::visit(ASMStatement& asmStmt)
{
	outLabel(asmStmt);
	newLine();
	if(asmStmt.getASMType() == ASMStatement::ASMTP_GCC)
	{
		m_outStream << Strings::format("__asm (\"%s\");", asmStmt.getASMString().c_str());
	} else {
		OPS_ASSERT(!"MS inline assembler is not supported yet");
	}
}

void OutToC::visit(EmptyStatement& emptyStmt)
{
	outLabel(emptyStmt);
	newLine(";");
}

void OutToC::visit(HirCBasicType& basicType)
{
	outConstVolatile(basicType);

    /*if (basicType.getKind() == HirCBasicType::HCBK_BOOL)
        m_declStack.top() = "bool " + m_declStack.top();
    else*/
    m_declStack.top() = HirCBasicType::basicKindToString(basicType.getKind(), false) + " " + m_declStack.top();
}

HirCBasicType::HirCBasicKind OutToC::basicTypeToCType(BasicType::BasicTypes basicType)
{
	OutToCSettings::BasicTypeToHirCType::const_iterator it = m_settings.typeMap.find(basicType);

	if (it != m_settings.typeMap.end())
	{
		return it->second;
	}
	else
	{
		throw RuntimeError(OPS::Strings::format("Unexpected basic type: %d", (int)basicType));
	}
}

void OutToC::visit(BasicType& basicType)
{
	outConstVolatile(basicType);

    /*if (basicType.getKind() == BasicType::BT_BOOLEAN)
		m_declStack.top() = "bool " + m_declStack.top();
    else*/
		m_declStack.top() = HirCBasicType::basicKindToString(basicTypeToCType(basicType.getKind()), false) + " " + m_declStack.top();
}

void OutToC::visit(PtrType& ptr)
{
	outConstVolatile(ptr);

	if (ptr.isRestrict())
		m_declStack.top() = " restrict " + m_declStack.top();

	m_declStack.top() = "*" + m_declStack.top();

	ptr.getPointedType().accept(*this);
}

void OutToC::visit(TypedefType& typeDef)
{
	outConstVolatile(typeDef);

	typeDef.getBaseType().accept(*this);

	m_declStack.top() = "typedef " + m_declStack.top();
}

void OutToC::visit(ArrayType& arrayType)
{
	outConstVolatile(arrayType);

	if (!m_declStack.top().empty() &&
		m_declStack.top()[0] == '*')
	{
		m_declStack.top() = "(" + m_declStack.top() + ")";
	}

	m_declStack.top() += "[";
	if (arrayType.getElementCount() > 0)
	{
		m_declStack.top() += OPS::Strings::format("%i", arrayType.getElementCount());
	}
    else if (arrayType.hasCountExpression())
    {
        m_declStack.top() += expressionToString(arrayType.getCountExpression());
    }
	m_declStack.top() += "]";
	arrayType.getBaseType().accept(*this);
}

void OutToC::visit(StructMemberDescriptor& memberDescriptor)
{
	newLine();
	m_outStream << composeDecl(memberDescriptor.getName(),
				 memberDescriptor.getType());

	if (memberDescriptor.getBitsLimit() > 0)
	{
		m_outStream << " : " << memberDescriptor.getBitsLimit();
	}
	else if (memberDescriptor.hasLimitExpression())
	{
		m_outStream << " : ";
		memberDescriptor.getLimitExpression().accept(*this);
	}
	m_outStream << ";";
}

void OutToC::visit(StructType& structType)
{
	outConstVolatile(structType);

	if (structType.isUnion())
		m_outStream << "union ";
	else
		m_outStream << "struct ";

	if (structType.getParent()->is_a<TypeDeclaration>())
	{
		m_outStream << m_declStack.top();
		m_declStack.top().clear();
	}

	if (structType.isFullType())
	{
		m_outStream << " {";

		increaseIndent();

		for (int index = 0; index < structType.getMemberCount(); ++index)
		{
			structType.getMember(index).accept(*this);
		}
		decreaseIndent();

		newLine("}");
	}
}

void OutToC::visit(Reprise::EnumMemberDescriptor& memberDescriptor)
{
	newLine(memberDescriptor.getName());
	m_outStream << " = " << memberDescriptor.getValue() << ",";
}

void OutToC::visit(EnumType& enumType)
{
	outConstVolatile(enumType);

	m_outStream << "enum ";

	if (enumType.getParent()->is_a<TypeDeclaration>())
	{
		m_outStream << m_declStack.top();
		m_declStack.top().clear();
	}
	m_outStream << "{";

	increaseIndent();

	for (int index = 0; index < enumType.getMemberCount(); ++index)
	{
		enumType.getMember(index).accept(*this);
	}

	decreaseIndent();
	newLine("}");
}

void OutToC::outSubroutineParameters(Reprise::SubroutineType &subroutineType)
{
	m_declStack.top() += "(";

	for (int index = 0; index < subroutineType.getParameterCount(); ++index)
	{
		if (index > 0)
			m_declStack.top() += ", ";
		m_declStack.top() += composeDecl(subroutineType.getParameter(index).getName(),
											  subroutineType.getParameter(index).getType());
	}

	if (subroutineType.isVarArg())
	{
		if (subroutineType.getParameterCount() > 0)
			m_declStack.top() += ", ";
		m_declStack.top() += "...";
	}

	m_declStack.top() += ")";
}

void OutToC::visit(SubroutineType& subroutineType)
{
	const bool needBraces = !m_declStack.top().empty() && m_declStack.top()[0] == '*';

    const std::string callTypeText = subroutineType.getCallingText(subroutineType.getCallingKind());
    m_declStack.top() = callTypeText + (callTypeText.empty() ? "" : " ") + m_declStack.top();
	outConstVolatile(subroutineType);

	if (needBraces)
	{
		m_declStack.top() = "(" + m_declStack.top() + ")";
	}
	outSubroutineParameters(subroutineType);
	subroutineType.getReturnType().accept(*this);
}

void OutToC::visit(DeclaredType& declared)
{
	outConstVolatile(declared);

	std::string prefix;
	if (declared.getDeclaration().getType().is_a<StructType>())
	{
		if (declared.getDeclaration().getType().cast_to<StructType>().isUnion())
			prefix = "union ";
		else
			prefix = "struct ";
	}
	else if (declared.getDeclaration().getType().is_a<EnumType>())
		prefix = "enum ";

	m_declStack.top() = prefix + declared.getDeclaration().getName() + " " + m_declStack.top();
}

void OutToC::visit(VectorType& vectorType)
{
    m_declStack.push("");
    vectorType.getBaseType().accept(*this);
    std::string baseType = m_declStack.top();
    m_declStack.pop();

    vectorType.getBaseType().accept(*this);
	m_declStack.top() = Strings::format(" __attribute__ ((vector_size (%d*sizeof(%s))))",
															vectorType.getElementCount(), baseType.c_str())
						+ m_declStack.top();
}

void OutToC::visit(BasicLiteralExpression& literalExpr)
{
	// максимальный приоритет - скобки не нужны
	m_outStream << literalExpr.getLiteralValueAsString(false);
}

void OutToC::visit(StrictLiteralExpression& literalExpr)
{
	// максимальный приоритет - скобки не нужны
	switch(literalExpr.getLiteralType())
	{
	case BasicType::BT_CHAR:
		if (literalExpr.getChar() >= 0x20)
			m_outStream << "'" << StrictLiteralExpression::stringToEscapedString(std::string(1, literalExpr.getChar())) << "'";
		else
			m_outStream << (int)literalExpr.getChar();
		break;
	case BasicType::BT_WIDE_CHAR:
		if (literalExpr.getWideChar() >= 0x20)
			m_outStream << "L'" << StrictLiteralExpression::stringToEscapedString(std::wstring(1, literalExpr.getWideChar())) << "'";
		else
			m_outStream << (int)literalExpr.getChar();
		break;
	case BasicType::BT_INT8:
		m_outStream << sdword(literalExpr.getInt8());
		break;
	case BasicType::BT_INT16:
		m_outStream << literalExpr.getInt16();
		break;
	case BasicType::BT_INT32:
		m_outStream << literalExpr.getInt32();
		break;
	case BasicType::BT_INT64:
		m_outStream << literalExpr.getInt64();
		break;
	case BasicType::BT_UINT8:
		m_outStream << dword(literalExpr.getUInt8());
		break;
	case BasicType::BT_UINT16:
		m_outStream << literalExpr.getUInt16();
		break;
	case BasicType::BT_UINT32:
		m_outStream << literalExpr.getUInt32();
		break;
	case BasicType::BT_UINT64:
		m_outStream << literalExpr.getUInt64();
		break;

	case BasicType::BT_FLOAT32:
		m_outStream << literalExpr.getLiteralValueAsString(false);
		break;
	case BasicType::BT_FLOAT64:
		m_outStream << literalExpr.getLiteralValueAsString(false);
		break;

	case BasicType::BT_BOOLEAN:
		m_outStream << literalExpr.getBoolean();
		break;

	case BasicType::BT_STRING:
		m_outStream << "\"" << StrictLiteralExpression::stringToEscapedString(literalExpr.getString()) << "\"";
		break;
	case BasicType::BT_WIDE_STRING:
		m_outStream << "L\"" << StrictLiteralExpression::stringToEscapedString(literalExpr.getWideString()) << "\"";
		break;
	default:
		throw RepriseError(Strings::format("Unexpected strict literal type (%u).", literalExpr.getLiteralType()));
	}
}

void OutToC::visit(CompoundLiteralExpression& literalExpr)
{
	// максимальный приоритет - скобки не нужны
	m_outStream << "{";
	for (int index = 0; index < literalExpr.getValueCount(); ++index)
	{
		if (index > 0)
			m_outStream << ", ";
		literalExpr.getValue(index).accept(*this);
	}
	m_outStream << "}";
}

void OutToC::visit(ReferenceExpression& referenceExpr)
{
	// максимальный приоритет - скобки не нужны
	m_outStream << referenceExpr.getReference().getName();
}

void OutToC::visit(SubroutineReferenceExpression& referenceExpr)
{
	// максимальный приоритет - скобки не нужны
	m_outStream << referenceExpr.getReference().getName();
}

void OutToC::visit(StructAccessExpression& structAccess)
{
	// Microsoft extensions support - anonymous structure members
	if (structAccess.getMember().getName().empty())
	{
		structAccess.getStructPointerExpression().accept(*this);
		return;
	}

	// максимальный приоритет - скобки не нужны
	const bool needBraces = exprNeedBraces(structAccess);
	if (needBraces) m_outStream << "(";
	structAccess.getStructPointerExpression().accept(*this);
	m_outStream << "." << structAccess.getMember().getName();
	if (needBraces) m_outStream << ")";
}

void OutToC::visit(EnumAccessExpression& enumAccess)
{
	// максимальный приоритет - скобки не нужны
	m_outStream << enumAccess.getMember().getName();
}

void OutToC::visit(TypeCastExpression& typeCast)
{
	const bool needBraces = exprNeedBraces(typeCast);
	if (needBraces)	m_outStream << "(";
	m_outStream << "(" << composeDecl("", typeCast.getCastType()) << ")";
	typeCast.getCastArgument().accept(*this);
	if (needBraces) m_outStream << ")";
}

void OutToC::visit(BasicCallExpression& basic)
{
	const bool needBraces = exprNeedBraces(basic);

	if (needBraces) m_outStream << "(";

	switch (basic.getKind())
	{
		//	Unary
	case BasicCallExpression::BCK_UNARY_PLUS:
		basic.getArgument(0).accept(*this);
		break;
	case BasicCallExpression::BCK_UNARY_MINUS:
		m_outStream << "-";
		basic.getArgument(0).accept(*this);
		break;

	case BasicCallExpression::BCK_SIZE_OF:			// sizeof() operator
		m_outStream << "sizeof(";
		basic.getArgument(0).accept(*this);
		m_outStream << ")";
		break;

	case BasicCallExpression::BCK_TAKE_ADDRESS:		// &
		m_outStream << "&";
		basic.getArgument(0).accept(*this);
		break;

	case BasicCallExpression::BCK_DE_REFERENCE:		// *
		m_outStream << "*";
		basic.getArgument(0).accept(*this);
		break;

		//	Binary
	case BasicCallExpression::BCK_BINARY_PLUS:		// +
		basic.getArgument(0).accept(*this);
		m_outStream << " + ";
		basic.getArgument(1).accept(*this);
		break;
	case BasicCallExpression::BCK_BINARY_MINUS:		// -
		basic.getArgument(0).accept(*this);
		m_outStream << " - ";
		basic.getArgument(1).accept(*this);
		break;
	case BasicCallExpression::BCK_MULTIPLY:			// *
		basic.getArgument(0).accept(*this);
		m_outStream << " * ";
		basic.getArgument(1).accept(*this);
		break;
	case BasicCallExpression::BCK_DIVISION:			// / 
		basic.getArgument(0).accept(*this);
		m_outStream << " / ";
		basic.getArgument(1).accept(*this);
		break;
		//	case BCK_INTEGER_DIVISION:	// div
		//		return "div";
	case BasicCallExpression::BCK_INTEGER_MOD:		// mod (%)
		basic.getArgument(0).accept(*this);
		m_outStream << " % ";
		basic.getArgument(1).accept(*this);
		break;

		//	Assign
	case BasicCallExpression::BCK_ASSIGN:				// =
		basic.getArgument(0).accept(*this);
		m_outStream << " = ";
		basic.getArgument(1).accept(*this);
		break;

		//	Equality
	case BasicCallExpression::BCK_LESS:				// <
		basic.getArgument(0).accept(*this);
		m_outStream << " < ";
		basic.getArgument(1).accept(*this);
		break;
	case BasicCallExpression::BCK_GREATER:			// >
		basic.getArgument(0).accept(*this);
		m_outStream << " > ";
		basic.getArgument(1).accept(*this);
		break;
	case BasicCallExpression::BCK_LESS_EQUAL:		// <=
		basic.getArgument(0).accept(*this);
		m_outStream << " <= ";
		basic.getArgument(1).accept(*this);
		break;
	case BasicCallExpression::BCK_GREATER_EQUAL:		// >=
		basic.getArgument(0).accept(*this);
		m_outStream << " >= ";
		basic.getArgument(1).accept(*this);
		break;
	case BasicCallExpression::BCK_EQUAL:				// ==
		basic.getArgument(0).accept(*this);
		m_outStream << " == ";
		basic.getArgument(1).accept(*this);
		break;
	case BasicCallExpression::BCK_NOT_EQUAL:			// !=
		basic.getArgument(0).accept(*this);
		m_outStream << " != ";
		basic.getArgument(1).accept(*this);
		break;

		//	Shifts
	case BasicCallExpression::BCK_LEFT_SHIFT:			// <<
		basic.getArgument(0).accept(*this);
		m_outStream << " << ";
		basic.getArgument(1).accept(*this);
		break;
	case BasicCallExpression::BCK_RIGHT_SHIFT:		// >>
		basic.getArgument(0).accept(*this);
		m_outStream << " >> ";
		basic.getArgument(1).accept(*this);
		break;

		//	Logical
	case BasicCallExpression::BCK_LOGICAL_NOT:		// !
		m_outStream << "!";
		basic.getArgument(0).accept(*this);
		break;
	case BasicCallExpression::BCK_LOGICAL_AND:		// &&
		basic.getArgument(0).accept(*this);
		m_outStream << " && ";
		basic.getArgument(1).accept(*this);
		break;
	case BasicCallExpression::BCK_LOGICAL_OR:			// ||
		basic.getArgument(0).accept(*this);
		m_outStream << " || ";
		basic.getArgument(1).accept(*this);
		break;

		//	Bitwise
	case BasicCallExpression::BCK_BITWISE_NOT:		// ~
		m_outStream << "~";
		basic.getArgument(0).accept(*this);
		break;
	case BasicCallExpression::BCK_BITWISE_AND:		// &
		basic.getArgument(0).accept(*this);
		m_outStream << " & ";
		basic.getArgument(1).accept(*this);
		break;
	case BasicCallExpression::BCK_BITWISE_OR:			// |
		basic.getArgument(0).accept(*this);
		m_outStream << " | ";
		basic.getArgument(1).accept(*this);
		break;
	case BasicCallExpression::BCK_BITWISE_XOR:		// ^
		basic.getArgument(0).accept(*this);
		m_outStream << " ^ ";
		basic.getArgument(1).accept(*this);
		break;
		//	Special
	case BasicCallExpression::BCK_ARRAY_ACCESS:		// []
		basic.getArgument(0).accept(*this);
		m_outStream << "[";
		basic.getArgument(1).accept(*this);
		for (int index = 2; index < basic.getArgumentCount(); ++index)
		{
			m_outStream << "][";
			basic.getArgument(index).accept(*this);
		}
		m_outStream << "]";
		break;
	case BasicCallExpression::BCK_COMMA:		// ,
		basic.getArgument(0).accept(*this);
		m_outStream << ", ";
		basic.getArgument(1).accept(*this);
		break;
	case BasicCallExpression::BCK_CONDITIONAL:		// ?:
		basic.getArgument(0).accept(*this);
		m_outStream << " ? ";
		basic.getArgument(1).accept(*this);
		m_outStream << " : ";
		basic.getArgument(2).accept(*this);
		break;
		OPS_DEFAULT_CASE_LABEL
	}

	if (needBraces) m_outStream << ")";
}

void OutToC::visit(SubroutineCallExpression& subroutineCallExpression)
{
	// максимальный приоритет - скобки не нужны
    if (subroutineCallExpression.hasExplicitSubroutineDeclaration())
	{
		SubroutineReferenceExpression& subroutineReferenceExpression = dynamic_cast<SubroutineReferenceExpression&>(subroutineCallExpression.getCallExpression());
        subroutineReferenceExpression.accept(*this);
        m_outStream << "(";
    }
    else
    {
        // Call is implicit
        subroutineCallExpression.getCallExpression().accept(*this);
        m_outStream << "(";
    }
	for (int index = 0; index < subroutineCallExpression.getArgumentCount(); ++index)
	{
		if (index > 0)
			m_outStream << ", ";
		subroutineCallExpression.getArgument(index).accept(*this);
	}
    m_outStream << ")";
}

void OutToC::visit(EmptyExpression&)
{
}

void OutToC::visit(HirCCallExpression& basic)
{
	const bool needBraces = exprNeedBraces(basic);

	if (needBraces) m_outStream << "(";

	switch (basic.getKind())
	{
		//	Unary
	case HirCCallExpression::HIRC_UNARY_PLUS:
		basic.getArgument(0).accept(*this);
		break;
	case HirCCallExpression::HIRC_UNARY_MINUS:
		m_outStream << "-";
		basic.getArgument(0).accept(*this);
		break;

	case HirCCallExpression::HIRC_SIZE_OF:			// sizeof() operator
		m_outStream << "sizeof(";
		basic.getArgument(0).accept(*this);
		m_outStream << ")";
		break;

	case HirCCallExpression::HIRC_TAKE_ADDRESS:		// &
		m_outStream << "&";
		basic.getArgument(0).accept(*this);
		break;

	case HirCCallExpression::HIRC_DE_REFERENCE:		// *
		m_outStream << "*";
		basic.getArgument(0).accept(*this);
		break;

	case HirCCallExpression::HIRC_PREFIX_PLUS_PLUS:
		m_outStream << "++";
		basic.getArgument(0).accept(*this);
		break;
	case HirCCallExpression::HIRC_POSTFIX_PLUS_PLUS:
		basic.getArgument(0).accept(*this);
		m_outStream << "++";
		break;
	case HirCCallExpression::HIRC_PREFIX_MINUS_MINUS:
		m_outStream << "--";
		basic.getArgument(0).accept(*this);
		break;
	case HirCCallExpression::HIRC_POSTFIX_MINUS_MINUS:
		basic.getArgument(0).accept(*this);
		m_outStream << "--";
		break;

		//	Binary
	case HirCCallExpression::HIRC_BINARY_PLUS:		// +
		basic.getArgument(0).accept(*this);
		m_outStream << " + ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_BINARY_MINUS:		// -
		basic.getArgument(0).accept(*this);
		m_outStream << " - ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_MULTIPLY:			// *
		basic.getArgument(0).accept(*this);
		m_outStream << " * ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_DIVISION:			// / 
		basic.getArgument(0).accept(*this);
		m_outStream << " / ";
		basic.getArgument(1).accept(*this);
		break;
		//	case BCK_INTEGER_DIVISION:	// div
		//		return "div";
	case HirCCallExpression::HIRC_INTEGER_MOD:		// mod (%)
		basic.getArgument(0).accept(*this);
		m_outStream << " % ";
		basic.getArgument(1).accept(*this);
		break;

		//	Assign
	case HirCCallExpression::HIRC_ASSIGN:				// =
		basic.getArgument(0).accept(*this);
		m_outStream << " = ";
		basic.getArgument(1).accept(*this);
		break;

	case HirCCallExpression::HIRC_PLUS_ASSIGN:				// +=
		basic.getArgument(0).accept(*this);
		m_outStream << " += ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_MINUS_ASSIGN:				// -=
		basic.getArgument(0).accept(*this);
		m_outStream << " -= ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_MULTIPLY_ASSIGN:			// *=
		basic.getArgument(0).accept(*this);
		m_outStream << " *= ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_DIVISION_ASSIGN:			// /=
		basic.getArgument(0).accept(*this);
		m_outStream << " /= ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_MOD_ASSIGN:				// %=
		basic.getArgument(0).accept(*this);
		m_outStream << " %= ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_LSHIFT_ASSIGN:				// <<=
		basic.getArgument(0).accept(*this);
		m_outStream << " <<= ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_RSHIFT_ASSIGN:				// >>=
		basic.getArgument(0).accept(*this);
		m_outStream << " >>= ";
		basic.getArgument(1).accept(*this);
		break;

	case HirCCallExpression::HIRC_BAND_ASSIGN:				// &=
		basic.getArgument(0).accept(*this);
		m_outStream << " &= ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_BOR_ASSIGN:				// |=
		basic.getArgument(0).accept(*this);
		m_outStream << " |= ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_BXOR_ASSIGN:				// ^=
		basic.getArgument(0).accept(*this);
		m_outStream << " ^= ";
		basic.getArgument(1).accept(*this);
		break;

		//	Equality
	case HirCCallExpression::HIRC_LESS:				// <
		basic.getArgument(0).accept(*this);
		m_outStream << " < ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_GREATER:			// >
		basic.getArgument(0).accept(*this);
		m_outStream << " > ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_LESS_EQUAL:		// <=
		basic.getArgument(0).accept(*this);
		m_outStream << " <= ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_GREATER_EQUAL:		// >=
		basic.getArgument(0).accept(*this);
		m_outStream << " >= ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_EQUAL:				// ==
		basic.getArgument(0).accept(*this);
		m_outStream << " == ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_NOT_EQUAL:			// !=
		basic.getArgument(0).accept(*this);
		m_outStream << " != ";
		basic.getArgument(1).accept(*this);
		break;

		//	Shifts
	case HirCCallExpression::HIRC_LEFT_SHIFT:			// <<
		basic.getArgument(0).accept(*this);
		m_outStream << " << ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_RIGHT_SHIFT:		// >>
		basic.getArgument(0).accept(*this);
		m_outStream << " >> ";
		basic.getArgument(1).accept(*this);
		break;

		//	Logical
	case HirCCallExpression::HIRC_LOGICAL_NOT:		// !
		m_outStream << "!";
		basic.getArgument(0).accept(*this);
		break;
	case HirCCallExpression::HIRC_LOGICAL_AND:		// &&
		basic.getArgument(0).accept(*this);
		m_outStream << " && ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_LOGICAL_OR:			// ||
		basic.getArgument(0).accept(*this);
		m_outStream << " || ";
		basic.getArgument(1).accept(*this);
		break;

		//	Bitwise
	case HirCCallExpression::HIRC_BITWISE_NOT:		// ~
		m_outStream << "~";
		basic.getArgument(0).accept(*this);
		break;
	case HirCCallExpression::HIRC_BITWISE_AND:		// &
		basic.getArgument(0).accept(*this);
		m_outStream << " & ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_BITWISE_OR:			// |
		basic.getArgument(0).accept(*this);
		m_outStream << " | ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_BITWISE_XOR:		// ^
		basic.getArgument(0).accept(*this);
		m_outStream << " ^ ";
		basic.getArgument(1).accept(*this);
		break;
		//	Special
	case HirCCallExpression::HIRC_ARRAY_ACCESS:		// []
		basic.getArgument(0).accept(*this);
		m_outStream << "[";
		basic.getArgument(1).accept(*this);
		for (int index = 2; index < basic.getArgumentCount(); ++index)
		{
			m_outStream << "][";
			basic.getArgument(index).accept(*this);
		}
		m_outStream << "]";
		break;
	case HirCCallExpression::HIRC_COMMA:		// ,
		basic.getArgument(0).accept(*this);
		m_outStream << ", ";
		basic.getArgument(1).accept(*this);
		break;
	case HirCCallExpression::HIRC_CONDITIONAL:		// ? :
		basic.getArgument(0).accept(*this);
		m_outStream << " ? ";
		basic.getArgument(1).accept(*this);
		m_outStream << " : ";
		basic.getArgument(2).accept(*this);
		break;

		OPS_DEFAULT_CASE_LABEL
	}

	if (needBraces) m_outStream << ")";
}

//  Functions implementation

std::string outToCOneUnitWithMain(Reprise::TranslationUnit& u)
{
    std::stringstream s;
    OutToC cWriter(s);
    std::set<Reprise::DeclarationBase*> usedDecls;
    Reprise::SubroutineDeclaration* mainSub = u.getGlobals().findSubroutine("main");
    OPS_ASSERT(mainSub != 0);
    Analysis::obtainUsedDeclarations(&mainSub->getDefinition(), usedDecls);
    cWriter.getSettings().usedDeclarations = &usedDecls;
    cWriter.getSettings().useIncludes = true;
    cWriter.visit(u);
    return s.str();
}

//  Exit namespace
}
}
