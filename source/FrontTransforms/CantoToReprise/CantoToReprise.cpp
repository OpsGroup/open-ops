#include <iostream>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <memory>

#include "FrontTransforms/CantoToReprise.h"
#include "Reprise/Reprise.h"
#include "Reprise/Canto.h"
#include "Reprise/Service/Service.h"
#include "FrontTransforms/ExpressionSimplifier.h"

using namespace std;
using namespace OPS::Reprise;
using namespace OPS::Reprise::Canto;
using namespace OPS::Reprise::Service;

namespace OPS
{
namespace Frontend
{
namespace C2R
{

template<class T>
class NodesFinder : public Service::DeepWalker
{
public:
	using Service::DeepWalker::visit;

	typedef list<T*> NodeListType;
	NodeListType NodeList;

	void visit(T& node)
	{
		NodeList.push_back(&node);		
	}
};


//  Constants and enums

//  Local classes
typedef std::map<Canto::HirCBasicType::HirCBasicKind, BasicType::BasicTypes> BindTableType;

class TypesConvertVisitor : public OPS::BaseVisitor
                         , public OPS::Visitor<Canto::HirCBasicType>
{
public:
    TypesConvertVisitor();
	void visit(Canto::HirCBasicType&);

private:
	ReprisePtr<BasicType> convert(const HirCBasicType&);
	BasicType::BasicTypes bind(Canto::HirCBasicType::HirCBasicKind);
	BindTableType m_table;
};


//  Functions declaration
static BindTableType createDefaultBindTable(void);

static void convertExprHelper(RepriseBase& repriseNode, bool general, bool other);

static void convertStmtExpr(RepriseBase& repriseNode);

static void convertSpecialExpression(HirCCallExpression& expr);

static void convertCleanPostfixExpression(HirCCallExpression& expr);

static void convertSideEffectExpression(HirCCallExpression& expr);

static void makeCorrespondingOperation(const HirCCallExpression::HirCOperatorKind kind, ReprisePtr<BasicCallExpression>& operExpr);

static StrictLiteralExpression* createOneLiteral(TypeBase& baseType);
//  Variables

//  Classes implementation
//		TypesConvertVisitor class implementation
TypesConvertVisitor::TypesConvertVisitor()
{
	m_table = createDefaultBindTable();
}

void TypesConvertVisitor::visit(Canto::HirCBasicType& cType)
{
	Editing::replaceType(cType, convert(cType));
}

ReprisePtr<BasicType> TypesConvertVisitor::convert(const HirCBasicType& type)
{
	ReprisePtr<BasicType> converted(BasicType::basicType(bind(type.getKind())));
	converted->setConst(type.isConst());
	converted->setVolatile(type.isVolatile());
	return converted;
}

BasicType::BasicTypes TypesConvertVisitor::bind(Canto::HirCBasicType::HirCBasicKind kind)
{
	BindTableType::const_iterator iter = m_table.find(kind);
	if (iter != m_table.end())
	{
		return iter->second;
	}
	else
		throw RepriseError("Unexpected kind.");
}

//		CommonExpressionsConvertWalker class 
class CommonExpressionsConvertWalker : public OPS::BaseVisitor
                                     , public OPS::Visitor<Canto::HirCCallExpression>
{
public:
	inline CommonExpressionsConvertWalker()	{}

	inline void visit(HirCCallExpression& call)
	{
		ReprisePtr<BasicCallExpression> newCall;
		switch (call.getKind())
		{
		case HirCCallExpression::HIRC_UNARY_PLUS:
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_UNARY_PLUS));
			break;
		case HirCCallExpression::HIRC_UNARY_MINUS:
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_UNARY_MINUS));
			break;

		case HirCCallExpression::HIRC_SIZE_OF:					// sizeof() operator
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_SIZE_OF));
			break;

		case HirCCallExpression::HIRC_TAKE_ADDRESS:				// &
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_TAKE_ADDRESS));
			break;
		case HirCCallExpression::HIRC_DE_REFERENCE:				// *
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_DE_REFERENCE));
			break;
		
		//	Binary
		case HirCCallExpression::HIRC_BINARY_PLUS:				// +
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS));
			break;
		case HirCCallExpression::HIRC_BINARY_MINUS:				// -
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS));
			break;
		case HirCCallExpression::HIRC_MULTIPLY:					// *
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY));
			break;
		case HirCCallExpression::HIRC_DIVISION:					// / 
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_DIVISION));
			break;
	//		case HirCCallExpression::HIRC_INTEGER_DIVISION:			// div
		case HirCCallExpression::HIRC_INTEGER_MOD:				// mod (%)
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_INTEGER_MOD));
			break;

		//	Assign
		case HirCCallExpression::HIRC_ASSIGN:					// =
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN));
			break;

		//	Equality
		case HirCCallExpression::HIRC_LESS:						// <
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_LESS));
			break;
		case HirCCallExpression::HIRC_GREATER:					// >
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_GREATER));
			break;
		case HirCCallExpression::HIRC_LESS_EQUAL:				// <=
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_LESS_EQUAL));
			break;
		case HirCCallExpression::HIRC_GREATER_EQUAL:				// >=
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_GREATER_EQUAL));
			break;
		case HirCCallExpression::HIRC_EQUAL:						// ==
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_EQUAL));
			break;
		case HirCCallExpression::HIRC_NOT_EQUAL:					// !=
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_NOT_EQUAL));
			break;

		//	Shifts
		case HirCCallExpression::HIRC_LEFT_SHIFT:				// <<
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_LEFT_SHIFT));
			break;
		case HirCCallExpression::HIRC_RIGHT_SHIFT:				// >>
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_RIGHT_SHIFT));
			break;

		//	Logical
		case HirCCallExpression::HIRC_LOGICAL_NOT:				// !
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_LOGICAL_NOT));
			break;
		case HirCCallExpression::HIRC_LOGICAL_AND:				// &&
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_LOGICAL_AND));
			break;
		case HirCCallExpression::HIRC_LOGICAL_OR:				// ||
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_LOGICAL_OR));
			break;

		//	Bitwise
		case HirCCallExpression::HIRC_BITWISE_NOT:				// ~
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_BITWISE_NOT));
			break;
		case HirCCallExpression::HIRC_BITWISE_AND:				// &
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_BITWISE_AND));
			break;
		case HirCCallExpression::HIRC_BITWISE_OR:				// |
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_BITWISE_OR));
			break;
		case HirCCallExpression::HIRC_BITWISE_XOR:				// ^
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_BITWISE_XOR));
			break;

		//	Special
		case HirCCallExpression::HIRC_ARRAY_ACCESS:				// []
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS));
			break;
		case HirCCallExpression::HIRC_COMMA:						// ,
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_COMMA));
			break;
		case HirCCallExpression::HIRC_CONDITIONAL:						// ?:
			newCall.reset(new BasicCallExpression(BasicCallExpression::BCK_CONDITIONAL));
			break;
        default:
				;
		}
		if (newCall.get() != 0)
		{
			for (int index = 0; index < call.getArgumentCount(); ++index)
			{
				newCall->addArgument(&call.getArgument(index));
			}
			Editing::replaceExpression(call, newCall);
		}
	}
};

//	Complex assign walker
class ComplexAssignWalker : public OPS::BaseVisitor
                          , public OPS::Visitor<HirCCallExpression>
{
public:
	typedef std::vector<HirCCallExpression*> ExprList;

	inline ComplexAssignWalker()
	{
	}

	void visit(HirCCallExpression& callExpr)
	{
		if (callExpr.isUnaryComplexAssign() || callExpr.isBinaryComplexAssign())
		{
			StatementBase* stmtBase = callExpr.obtainParentStatement();
			if (stmtBase == 0)
				throw RepriseError("Unexpected use of complex assign.");

			m_exprList.push_back(&callExpr);
		}
	}

	inline const ExprList& getExprList() const
	{
		return m_exprList;
	}

private:
	ExprList m_exprList;
};


//	Local classes
class FirstStatementsMarks
{
public:
	enum MarksKind
	{
		KIND_FIRST_STMT = 1,
	};
	typedef MarksKind MarksKindType;

	inline static std::string kindToString(const MarksKind& kind)
	{
		switch (kind)
		{
		case KIND_FIRST_STMT:
			return "FIRST_STMT";
		default:
			throw ArgumentError(Strings::format("Unexpected mark (%i).", kind));
		}
	}

	inline static MarksKind stringToKind(const std::string& kindString)
	{
		if (kindString == "FIRST_STMT")
			return KIND_FIRST_STMT;
		else
			throw ArgumentError(Strings::format("Unexpected string mark '%s'.", kindString.c_str()));
	}
};

typedef Marker<SimpleMarkersStorage<FirstStatementsMarks> > SimpleFSMarker;

class FirstStatementMarker : public OPS::BaseVisitor
                           , public OPS::Visitor<BlockStatement>
{
public:
	inline FirstStatementMarker(SimpleFSMarker& marker) : m_marker(marker)
	{
	}

	inline void visit(BlockStatement& block)
	{
		if (!block.isEmpty())
		{
			m_marker.addMark(*block.getFirst(), FirstStatementsMarks::KIND_FIRST_STMT);
		}
	}

private:
	SimpleFSMarker& m_marker;
};

class ConstantChecker : public DeepWalker
{
public:
	ConstantChecker() : m_isConstant(true) {}
    bool isConstant() { return m_isConstant; }
	void visit(ReferenceExpression&) { m_isConstant = false; }
	void visit(SubroutineCallExpression&) { m_isConstant = false; }

private:
    bool m_isConstant;
};


class VariablesInitWalker : public DeepWalker
{
public:
	VariablesInitWalker();

	void visit(TranslationUnit&);

	void visit(VariableDeclaration&);
    void visit(Canto::HirCVariableInitStatement&);

    void visit(TypeDeclaration&);
	void visit(ExpressionStatement&);

private:
    void eraseVariableInit(HirCVariableInitStatement&);
	SimpleFSMarker m_marker;
    std::vector<HirCVariableInitStatement*> m_initStatements;
};

VariablesInitWalker::VariablesInitWalker()
{
}

static void setConstRecursive(TypeBase& type, bool isConst)
{
    type.setConst(isConst);
    if (ArrayType* array = type.cast_ptr<ArrayType>())
    {
        setConstRecursive(array->getBaseType(), isConst);
    }
    else if (PtrType* ptr = type.cast_ptr<PtrType>())
    {
        setConstRecursive(ptr->getPointedType(), isConst);
    }
}

void VariablesInitWalker::visit(TranslationUnit& unit)
{
	m_marker.clear();
    m_initStatements.clear();
	FirstStatementMarker fsm(m_marker);
    makePostOrderVisitorTraversal(unit, fsm);
	DeepWalker::visit(unit);

    for(size_t stmt = 0; stmt < m_initStatements.size(); ++stmt)
    {
        HirCVariableInitStatement& varInit = *m_initStatements[stmt];

        if (!varInit.getVariableDeclaration().getDeclarators().isStatic())
        {
			setConstRecursive(varInit.getVariableDeclaration().getType(), false);
            ReprisePtr<StatementBase> initStmt(createVariableInitStatement(varInit.getVariableDeclaration(),
                                                                             varInit.getInitExpression()));
            if (varInit.isConnectedToForStmt() && initStmt->is_a<ExpressionStatement>())
            {
                varInit.getConnectedForStmt().setInitExpression(&initStmt->cast_to<ExpressionStatement>().get());
                eraseVariableInit(varInit);
            }
            else
            {
                Editing::replaceStatement(varInit, initStmt);
            }
        }
        else
        {
            ReprisePtr<VariableDeclaration> varDecl(&varInit.getVariableDeclaration());
            // Move variable to global declarations
            {
                SubroutineDeclaration& subroutine = varInit.getRootBlock().getParent()->cast_to<SubroutineDeclaration>();
                Declarations::Iterator itVarDecl = subroutine.getDeclarations().convertToIterator(varDecl.get());
                subroutine.getDeclarations().erase(itVarDecl);
                unit.getGlobals().addBefore(unit.getGlobals().convertToIterator(&subroutine), varDecl.get());
                varDecl->resetDefinedBlock();
                unit.getGlobals().rename(varDecl.get(), generateUniqueIndentifier(varDecl->getName()));
            }

			ConstantChecker constChecker;
			varInit.accept(constChecker);

			if (!constChecker.isConstant())
			{
				setConstRecursive(varInit.getVariableDeclaration().getType(), false);
				// Create flag variable
				ReprisePtr<VariableDeclaration> flagVarDecl(new VariableDeclaration(BasicType::int32Type(),
															generateUniqueIndentifier(varDecl->getName()+"_init"),
															VariableDeclarators::DECL_STATIC,
															StrictLiteralExpression::createInt32(0)));
				unit.getGlobals().addBefore(unit.getGlobals().convertToIterator(varDecl.get()), flagVarDecl.get());

				// Create condition "flag == 0"
				ReprisePtr<ExpressionBase> condition(new BasicCallExpression(BasicCallExpression::BCK_EQUAL,
																			 new ReferenceExpression(*flagVarDecl),
																			 StrictLiteralExpression::createInt32(0)));

				// Create init done stmt "flag = 1;"
				ReprisePtr<ExpressionStatement> initDoneStmt(
							new ExpressionStatement(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN,
																			new ReferenceExpression(*flagVarDecl),
																			StrictLiteralExpression::createInt32(1))));

				// Create if statement "if (flag == 0)"
				ReprisePtr<IfStatement> ifStmt(new IfStatement(condition.release()));
				ifStmt->getThenBody().addLast(createVariableInitStatement(*varDecl, varInit.getInitExpression()));
				ifStmt->getThenBody().addLast(initDoneStmt.release());
				Editing::replaceStatement(varInit, ifStmt);
			}
			else
			{
				varDecl->setInitExpression(varInit.getInitExpression());
                eraseVariableInit(varInit);
			}
        }
    }
}

void VariablesInitWalker::visit(VariableDeclaration& variable)
{
	if (variable.hasNonEmptyInitExpression() &&
        variable.hasDefinedBlock())
		//!variable.getType().isConst()) - в спеках константы инициализируются выражениями, содержащими вхождения
	{
        BlockStatement& block = variable.getDefinedBlock();
        StatementBase* const firstStmt = m_marker.findMarkedStatementInBlock(variable.getDefinedBlock(), FirstStatementsMarks::KIND_FIRST_STMT);
        ReprisePtr<HirCVariableInitStatement> initStmt(new HirCVariableInitStatement(&variable, variable.detachInitExpression().release()));
        if (firstStmt != 0)
        {
            block.addBefore(block.convertToIterator(firstStmt),
                            initStmt.get());
        }
        else
        {
            block.addLast(initStmt.get());
        }
	}
}

void VariablesInitWalker::visit(HirCVariableInitStatement& varInit)
{
    m_initStatements.push_back(&varInit);
}

void VariablesInitWalker::eraseVariableInit(HirCVariableInitStatement& varInit)
{
    BlockStatement& parentBlock = varInit.getParent()->cast_to<BlockStatement>();
    parentBlock.erase(parentBlock.convertToIterator(&varInit));
}

static StatementBase* convertGenericInit(ExpressionBase& lvalue, ExpressionBase& initExpression);

static StatementBase* convertBasicInit(ExpressionBase& lvalue, ExpressionBase& initExpression)
{
    ReprisePtr<BasicCallExpression> initAssign(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN));
    initAssign->addArgument(&lvalue);
    initAssign->addArgument(&initExpression);
    return new ExpressionStatement(initAssign.release());
}

static BasicCallExpression* isArrayAccess(ExpressionBase& expr)
{
	if (BasicCallExpression* bce = expr.cast_ptr<BasicCallExpression>())
		if (bce->getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
			return bce;
	return 0;
}

static ExpressionBase* createArrayAccess(ExpressionBase& lvalue, int index)
{
	if (BasicCallExpression* arrayAccess = isArrayAccess(lvalue))
	{
		arrayAccess = arrayAccess->clone();
		arrayAccess->addArgument(StrictLiteralExpression::createInt32(index));
		return arrayAccess;
	}
	else
	{
		return new BasicCallExpression(
								BasicCallExpression::BCK_ARRAY_ACCESS,
								lvalue.clone(),
								StrictLiteralExpression::createInt32(index));
	}
}

static StatementBase* convertCompoundInit(ExpressionBase& lvalue, CompoundLiteralExpression& compound)
{
    ReprisePtr<TypeBase> lType(lvalue.getResultType());
    TypeBase& realType = Editing::desugarType(*lType);

    if (realType.is_a<PtrType>())
    {
        ReprisePtr<BlockStatement> block(new BlockStatement);
        for(int i = 0; i < compound.getValueCount(); ++i)
        {
			ReprisePtr<ExpressionBase> newLValue(createArrayAccess(lvalue, i));
            block->addLast(convertGenericInit(*newLValue, compound.getValue(i)));
        }
        return block.release();
    }
    else if (StructType* structType = realType.cast_ptr<StructType>())
    {
        ReprisePtr<BlockStatement> block(new BlockStatement);
        for(int i = 0; i < compound.getValueCount(); ++i)
        {
            ReprisePtr<ExpressionBase> newLValue(new StructAccessExpression(
                                                     *lvalue.clone(),
                                                     structType->getMember(i)));
            block->addLast(convertGenericInit(*newLValue, compound.getValue(i)));
        }
        return block.release();
    }
    else
    {
        ReprisePtr<TypeCastExpression> typeCast(new TypeCastExpression(lvalue.getResultType().release(),
                                                                       &compound));
        return convertBasicInit(lvalue, *typeCast);
    }
}

template<BasicType::BasicTypes, typename CharType>
	StrictLiteralExpression* createCharLiteral(CharType c);

template<>
	inline StrictLiteralExpression* createCharLiteral<BasicType::BT_CHAR, char>(char c)
	{ return StrictLiteralExpression::createChar(c); }

template<>
	inline StrictLiteralExpression* createCharLiteral<BasicType::BT_WIDE_CHAR, wchar_t>(wchar_t c)
	{ return StrictLiteralExpression::createWideChar(c); }

template<typename StringType, BasicType::BasicTypes basicType>
static StatementBase* convertArrayStringInit(ExpressionBase& lvalue, const StringType& str)
{
	ReprisePtr<BlockStatement> block(new BlockStatement);
	for(size_t i = 0; i <= str.length(); ++i)
	{
		ReprisePtr<ExpressionBase> newLValue(createArrayAccess(lvalue, i));
		ReprisePtr<StrictLiteralExpression> rValue(createCharLiteral<basicType>(str[i]));
		block->addLast(new ExpressionStatement(
						   new BasicCallExpression(BasicCallExpression::BCK_ASSIGN,
												   newLValue.get(),
												   rValue.get())));
	}
	return block.release();
}

static StatementBase* convertArrayStringInit(ExpressionBase& lvalue, StrictLiteralExpression& stringLiteral)
{
	OPS_ASSERT(stringLiteral.isString());

	if (stringLiteral.getLiteralType() == BasicType::BT_STRING)
	{
		return convertArrayStringInit<std::string, BasicType::BT_CHAR>(lvalue, stringLiteral.getString());
	}
	else if (stringLiteral.getLiteralType() == BasicType::BT_WIDE_STRING)
	{
		return convertArrayStringInit<std::wstring, BasicType::BT_WIDE_CHAR>(lvalue, stringLiteral.getWideString());
	}
	else
	{
		throw OPS::StateError("Unexpected string literal type in  convertArrayStringInit");
	}
}

static StatementBase* convertGenericInit(ExpressionBase& lvalue, ExpressionBase& initExpression)
{
    if (initExpression.is_a<CompoundLiteralExpression>())
    {
        return convertCompoundInit(lvalue, initExpression.cast_to<CompoundLiteralExpression>());
    }
	else if (StrictLiteralExpression* literal = initExpression.cast_ptr<StrictLiteralExpression>())
	{
		if (literal->isString() && Editing::getExpressionPreciseType(lvalue)->is_a<ArrayType>())
		{
			return convertArrayStringInit(lvalue, *literal);
		}
	}
	return convertBasicInit(lvalue, initExpression);
}

StatementBase* createVariableInitStatement(VariableDeclaration &variable, ExpressionBase &initExpression)
{
    ReprisePtr<ReferenceExpression> varRef(new ReferenceExpression(variable));
    return convertGenericInit(*varRef, initExpression);
}

StatementBase* convertVariableInit(Reprise::VariableDeclaration &variable, StatementBase *firstStatement)
{
    variable.getType().setConst(false);

    ReprisePtr<StatementBase> initStmt(createVariableInitStatement(variable, *variable.detachInitExpression()));

	BlockStatement& block = variable.getDefinedBlock();

	if (firstStatement != 0)
	{
		block.addBefore(block.convertToIterator(firstStatement), initStmt.get());
	}
	else
	{
		block.addLast(initStmt.get());
	}
	return initStmt.get();
}

//void VariablesInitWalker::visit(VariableDeclaration& variable)
//{
//	if (variable.hasNonEmptyInitExpression())
//	{
//		ReprisePtr<BasicCallExpression> initAssign(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN));
//		initAssign->addArgument(new ReferenceExpression(variable));
//		initAssign->addArgument(variable.detachInitExpression().release());
//		ReprisePtr<ExpressionStatement> initStmt(new ExpressionStatement(initAssign.release()));
//		BlockStatement& block = variable.getDefinedBlock();
//		if (!block.isEmpty())
//		{
//			StatementBase* const firstStmt = m_marker.findMarkedStatementInBlock(block, FirstStatementsMarks::KIND_FIRST_STMT);
//			OPS_ASSERT(firstStmt != 0)
//				block.addBefore(block.convertToIterator(firstStmt), initStmt.release());
//		}
//		else
//			block.addFirst(initStmt.release());
//	}
//}

void VariablesInitWalker::visit(TypeDeclaration&)
{
    // This is needed to avoid visiting type declarations
}

void VariablesInitWalker::visit(ExpressionStatement&)
{
	// To avoid visiting expression statements which cannot contain initializations
}


class BreakContinueConverter : public DeepWalker
{
public:
	inline BreakContinueConverter()
	{
	}

	inline void visit(TranslationUnit& unit)
	{
        m_continues.clear();
        m_breaks.clear();
        m_lastStatements.clear();
        m_afterLoopStatements.clear();

		DeepWalker::visit(unit);
		convertContinues();
		convertBreaks();
	}

    void visit(Canto::HirContinueStatement& contStmt)
	{
		m_continues.push_back(&contStmt);
	}

    void visit(Canto::HirBreakStatement& breakStmt)
	{
		m_breaks.push_back(&breakStmt);
	}

	void visit(TypeDeclaration&)
	{
		// to avoid visiting type declarations
	}

	void visit(VariableDeclaration&)
	{
		// to avoid visiting variables
	}

	void visit(ExpressionStatement&)
	{
		// to avoid visiting expression statetements
	}

private:
    typedef std::vector<Canto::HirContinueStatement*> ContinueListType;
    typedef std::vector<Canto::HirBreakStatement*> BreakListType;
	typedef std::map<StatementBase*, EmptyStatement*> TailLabelMapType;

	void convertContinues();
	void convertBreaks();

	EmptyStatement& labelLastLoopStatement(StatementBase& loopStmt);
	EmptyStatement& labelStatementAfter(StatementBase& loopOrSwitchStmt);
	static StatementBase& obtainWraparoundLoopAndSwitch(StatementBase& stmt, bool andSwitch);

	ContinueListType m_continues;
	BreakListType m_breaks;
	TailLabelMapType m_lastStatements;
	TailLabelMapType m_afterLoopStatements;
};

void BreakContinueConverter::convertContinues()
{
	for (ContinueListType::iterator current = m_continues.begin(); current != m_continues.end(); ++current)
	{
        Canto::HirContinueStatement& contStmt = **current;
		StatementBase& wrapAroundLoop = obtainWraparoundLoopAndSwitch(contStmt, false);
		EmptyStatement& labeledStmt = labelLastLoopStatement(wrapAroundLoop);
		ReprisePtr<StatementBase> gotoStmt(new GotoStatement(&labeledStmt));
		Editing::replaceStatement(contStmt, gotoStmt);
	}
}

void BreakContinueConverter::convertBreaks()
{
	for (BreakListType::iterator current = m_breaks.begin(); current != m_breaks.end(); ++current)
	{
        Canto::HirBreakStatement& breakStmt = **current;
		StatementBase& wrapAroundStmt = obtainWraparoundLoopAndSwitch(breakStmt, true);
		EmptyStatement& labeledStmt = labelStatementAfter(wrapAroundStmt);
		ReprisePtr<StatementBase> gotoStmt(new GotoStatement(&labeledStmt));
		Editing::replaceStatement(breakStmt, gotoStmt);
	}
}

EmptyStatement& BreakContinueConverter::labelLastLoopStatement(StatementBase& loopStmt)
{
	OPS_ASSERT(loopStmt.is_a<ForStatement>() || loopStmt.is_a<WhileStatement>())

	TailLabelMapType::const_iterator labelIter = m_lastStatements.find(&loopStmt);
	if (labelIter != m_lastStatements.end())
	{
		return *labelIter->second;
	}
	else
	{
		BlockStatement* loopBody = 0;
		{
			ForStatement* forStmt = loopStmt.cast_ptr<ForStatement>();
			if (forStmt != 0)
			{
				loopBody = &forStmt->getBody();
			}
			else
			{
				WhileStatement* whileStmt = loopStmt.cast_ptr<WhileStatement>();
				if (whileStmt != 0)
				{
					loopBody = &whileStmt->getBody();
				}
			}
		}
		if (loopBody == 0)
			throw RepriseError("Unexpected loop statement.");
		if (!loopBody->isEmpty())
		{
			StatementBase& lastStmt = *loopBody->getLast();
			if (lastStmt.is_a<EmptyStatement>() && lastStmt.hasLabel())
			{
				EmptyStatement& labeledStmt = lastStmt.cast_to<EmptyStatement>();
				m_lastStatements.insert(std::make_pair(&loopStmt, &labeledStmt));
				return labeledStmt;
			}
		}
		EmptyStatement* labeledStmt = new EmptyStatement();
		labeledStmt->setUniqueLabel();
		loopBody->addLast(labeledStmt);
		m_lastStatements.insert(std::make_pair(&loopStmt, labeledStmt));
		return *labeledStmt;
	}
}

EmptyStatement& BreakContinueConverter::labelStatementAfter(StatementBase& loopOrSwitchStmt)
{
    OPS_ASSERT(loopOrSwitchStmt.is_a<ForStatement>() || loopOrSwitchStmt.is_a<WhileStatement>() || loopOrSwitchStmt.is_a<PlainSwitchStatement>())

	TailLabelMapType::const_iterator labelIter = m_afterLoopStatements.find(&loopOrSwitchStmt);
	if (labelIter != m_afterLoopStatements.end())
	{
		return *labelIter->second;
	}
	else
	{
		BlockStatement& parentBlock = loopOrSwitchStmt.getParentBlock();
		BlockStatement::Iterator losStmt = parentBlock.convertToIterator(&loopOrSwitchStmt);
		losStmt++;
		if (losStmt.isValid())
		{
			EmptyStatement* emptyStmt = losStmt->cast_ptr<EmptyStatement>();
			if (emptyStmt != 0 && emptyStmt->hasLabel())
			{
				m_afterLoopStatements.insert(std::make_pair(&loopOrSwitchStmt, emptyStmt));
				return *emptyStmt;
			}
		}
		EmptyStatement* labeledStmt = new EmptyStatement();
		labeledStmt->setUniqueLabel();
		parentBlock.addBefore(losStmt, labeledStmt);
		m_afterLoopStatements.insert(std::make_pair(&loopOrSwitchStmt, labeledStmt));
		return *labeledStmt;
	}
}

StatementBase& BreakContinueConverter::obtainWraparoundLoopAndSwitch(StatementBase& stmt, bool andSwitch)
{
	StatementBase* current = &stmt;
	for (;;)
	{
		RepriseBase* baseParent = current->getParent();
		if (baseParent == 0)
		{
			throw RepriseError("Unexpected NULL parent found.");
		}
		if (baseParent->is_a<ForStatement>() || baseParent->is_a<WhileStatement>())
		{
			return baseParent->cast_to<StatementBase>();
		}
		if (andSwitch && baseParent->is_a<PlainSwitchStatement>())
		{
			return baseParent->cast_to<StatementBase>();
		}
		current = dynamic_cast<StatementBase*>(current->getParent());
		if (current == 0)
		{
			throw RepriseError("Unexpected NULL parent found.");
		}
	}
}

//  Global classes implementation

//  Global functions implementation
void convertTypes(RepriseBase& repriseNode)
{
    TypesConvertVisitor walker;
    OPS::Reprise::Service::makePostOrderVisitorTraversal(repriseNode, walker);
}

void convertExpressions(RepriseBase& repriseNode, bool common, bool general, bool others)
{
	if (common)
	{
		CommonExpressionsConvertWalker commonWalker;
        OPS::Reprise::Service::makePostOrderVisitorTraversal(repriseNode, commonWalker);

		if (general || others)
		{
            convertStmtExpr(repriseNode);
			convertExprHelper(repriseNode, general, others);
		}
	}
}

void convertLiterals(RepriseBase& repriseNode)
{
	// TODO: Do convertLiterals
	OPS_UNUSED(repriseNode)
}

void convertVariablesInit(RepriseBase& repriseNode)
{
	// TODO: Do convertVariablesInit
	OPS_UNUSED(repriseNode)
	VariablesInitWalker walker;
	repriseNode.accept(walker);
}

void convertBreakContinue(RepriseBase& repriseNode)
{
	OPS_UNUSED(repriseNode)
	BreakContinueConverter walker;
	repriseNode.accept(walker);
}

// Test conversions
/*
class TestConverter : public Service::DeepWalker
{
public:
	inline TestConverter()
	{
	}

	inline void visit(ProgramUnit& program)
	{
		DeepWalker::visit(program);
	}

	inline void visit(ForStatement& forStmt)
	{
		convertExpression(forStmt.getInitExpression());
		convertExpression(forStmt.getFinalExpression());
		convertExpression(forStmt.getStepExpression());
	}
	
	void visit(WhileStatement& whileStmt)
	{
		convertExpression(whileStmt.getCondition());
	}
	
	void visit(IfStatement& ifStmt)
	{
		convertExpression(ifStmt.getCondition());
	}

	void visit(SwitchStatement& switchStmt)
	{
		convertExpression(switchStmt.getCondition());
	}

	void visit(ReturnStatement& returnStmt)
	{
		convertExpression(returnStmt.getReturnExpression());
	}

	void visit(ExpressionStatement& exprStmt)
	{
		convertExpression(exprStmt.get());
	}

private:

	void convertExpression(ExpressionBase& expr);
};

//		TestConverter class implementation
void TestConverter::convertExpression(ExpressionBase& expr)
{
	std::cout << expr.dumpState() << std::endl;
}

void convertTest(ProgramUnit& programUnit)
{
}
*/


//	Local functions implementations
BindTableType createDefaultBindTable(void)
{
	BindTableType table;
	table.insert(std::make_pair(HirCBasicType::HCBK_CHAR, BasicType::BT_CHAR));
	table.insert(std::make_pair(HirCBasicType::HCBK_WIDE_CHAR, BasicType::BT_WIDE_CHAR));
	table.insert(std::make_pair(HirCBasicType::HCBK_SCHAR, BasicType::BT_INT8));
	table.insert(std::make_pair(HirCBasicType::HCBK_SHORT, BasicType::BT_INT16));
	table.insert(std::make_pair(HirCBasicType::HCBK_INT, BasicType::BT_INT32));
	table.insert(std::make_pair(HirCBasicType::HCBK_LONG, BasicType::BT_INT32));
    table.insert(std::make_pair(HirCBasicType::HCBK_LONG_LONG, BasicType::BT_INT64));
    table.insert(std::make_pair(HirCBasicType::HCBK_INT128, BasicType::BT_INT128));
	table.insert(std::make_pair(HirCBasicType::HCBK_UCHAR, BasicType::BT_UINT8));
	table.insert(std::make_pair(HirCBasicType::HCBK_USHORT, BasicType::BT_UINT16));
	table.insert(std::make_pair(HirCBasicType::HCBK_UINT, BasicType::BT_UINT32));
	table.insert(std::make_pair(HirCBasicType::HCBK_ULONG, BasicType::BT_UINT32));
	table.insert(std::make_pair(HirCBasicType::HCBK_ULONG_LONG, BasicType::BT_UINT64));
    table.insert(std::make_pair(HirCBasicType::HCBK_UINT128, BasicType::BT_UINT128));

	table.insert(std::make_pair(HirCBasicType::HCBK_FLOAT, BasicType::BT_FLOAT32));
	table.insert(std::make_pair(HirCBasicType::HCBK_DOUBLE, BasicType::BT_FLOAT64));
	table.insert(std::make_pair(HirCBasicType::HCBK_LONG_DOUBLE, BasicType::BT_FLOAT64));

	table.insert(std::make_pair(HirCBasicType::HCBK_BOOL, BasicType::BT_BOOLEAN));
	table.insert(std::make_pair(HirCBasicType::HCBK_VOID, BasicType::BT_VOID));

	return table;
}

void convertExprHelper(RepriseBase& repriseNode, bool general, bool other)
{
	ComplexAssignWalker cawalker;
    makePostOrderVisitorTraversal(repriseNode, cawalker);

	for (ComplexAssignWalker::ExprList::const_iterator exprIt = cawalker.getExprList().begin(); exprIt != cawalker.getExprList().end(); ++exprIt)
	{
		HirCCallExpression& expr = **exprIt;
		if (expr.getParent() == 0)
			throw RepriseError("Unexpected NULL statement in expression");
		bool converted = false;
		if (general)
		{
			if (expr.getParent()->is_a<ExpressionStatement>())
			{
				if (expr.getArgumentCount() == 0)
					throw RepriseError("Unexpected complex assign expression with no arguments");
				if (!Editing::hasSideEffects(expr.getArgument(0)))
				{
					convertSpecialExpression(expr);
					converted = true;
				}
			}
			else if (expr.getParent()->is_a<ForStatement>())
			{
				ForStatement& forStmt = expr.getParent()->cast_to<ForStatement>();
				if (&forStmt.getInitExpression() == &expr ||
					&forStmt.getStepExpression() == &expr)
				{
					if (expr.getArgumentCount() == 0)
						throw RepriseError("Unexpected complex assign expression with no arguments");
					if (!Editing::hasSideEffects(expr.getArgument(0)))
					{
						convertSpecialExpression(expr);
						converted = true;
					}
				}
			}
		}
		if (!converted && other)
		{
			if (expr.isBinaryComplexAssign() || expr.isUnaryComplexAssign())
			{
				if (expr.getArgumentCount() == 0)
					throw RepriseError("Unexpected complex assign expression with no arguments");
				if (!Editing::hasSideEffects(expr.getArgument(0)))
				{
					if (expr.getKind() == HirCCallExpression::HIRC_POSTFIX_MINUS_MINUS ||
						expr.getKind() == HirCCallExpression::HIRC_POSTFIX_PLUS_PLUS)
					{
						convertCleanPostfixExpression(expr);
					}
					else
					{
						convertSpecialExpression(expr);
					}
				}
				else
				{
					if (expr.getArgument(0).is_a<BasicCallExpression>())
					{
						BasicCallExpression& call = expr.getArgument(0).cast_to<BasicCallExpression>();
						if (call.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS ||
							call.getKind() == BasicCallExpression::BCK_DE_REFERENCE ||
							call.getKind() == BasicCallExpression::BCK_TAKE_ADDRESS)
						{
							convertSideEffectExpression(expr);
						}
						else
						{
							throw RepriseError("Unexpected argument of complex assignment. Not implemented.");
						}
					}
					else
					{
						convertSpecialExpression(expr);
					}
				}
			}
		}
	}
}

class StmtExprWalker : public OPS::BaseVisitor
                     , public OPS::Visitor<HirCStatementExpression>
{
public:
    typedef std::vector<HirCStatementExpression*> ExprList;

    inline StmtExprWalker()
    {
    }

    void visit(HirCStatementExpression& callExpr)
    {
        m_exprList.push_back(&callExpr);
    }

    inline const ExprList& getExprList() const
    {
        return m_exprList;
    }

private:
    ExprList m_exprList;
};

static StatementBase* getLastStatement(BlockStatement& block)
{
	StatementBase* lastStatement = &(*block.getLast());

	while(BlockStatement* lastBlock = lastStatement->cast_ptr<BlockStatement>())
		lastStatement = &*lastBlock->getLast();

	return lastStatement;
}

void convertStmtExpr(RepriseBase& repriseNode)
{
    StmtExprWalker stmtExprWalker;
    OPS::Reprise::Service::makePostOrderVisitorTraversal(repriseNode, stmtExprWalker);

    const StmtExprWalker::ExprList& exprList = stmtExprWalker.getExprList();
    for(size_t i = 0; i < exprList.size(); ++i)
    {
        HirCStatementExpression& stmtExpr = *exprList[i];
        StatementBase* parentStmt = stmtExpr.obtainParentStatement();
        BlockStatement& parentStmtBlock = parentStmt->getParentBlock();

        if (!stmtExpr.getParent()->is_a<ExpressionStatement>())
        {
			ExpressionStatement& lastStatement = getLastStatement(stmtExpr.getSubStatements())->cast_to<ExpressionStatement>();

            ReprisePtr<TypeBase> resultType = lastStatement.get().getResultType();
            resultType->setConst(false);

            VariableDeclaration& resultVar = Editing::createNewVariable(*resultType, parentStmtBlock, "SE");

            ReprisePtr<BasicCallExpression> assignExpr(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN));
            assignExpr->addArgument(new ReferenceExpression(resultVar));
            assignExpr->addArgument(&lastStatement.get());
            lastStatement.set(assignExpr.get());

            parentStmtBlock.addBefore(parentStmtBlock.convertToIterator(parentStmt), &stmtExpr.getSubStatements());
            Editing::replaceExpression(stmtExpr, ReprisePtr<ExpressionBase>(new ReferenceExpression(resultVar)));
        }
        else
        {
            ReprisePtr<StatementBase> subStatements(&stmtExpr.getSubStatements());
            Editing::replaceStatement(*parentStmt, subStatements);
        }
    }
}

void convertSpecialExpression(HirCCallExpression& expr)
{
	if (expr.isUnaryComplexAssign())
	{
		if (expr.getArgumentCount() != 1)
			throw RepriseError("Unexpected ++ or -- expression with not 1 argument");

		ReprisePtr<ExpressionBase> assignExpression(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN));
		BasicCallExpression& assignExpr = assignExpression->cast_to<BasicCallExpression>();
		assignExpr.addArgument(&expr.getArgument(0));
		ReprisePtr<BasicCallExpression> operExpr;
		makeCorrespondingOperation(expr.getKind(), operExpr);
		operExpr->addArgument(expr.getArgument(0).clone());
		ReprisePtr<TypeBase> sleType(expr.getArgument(0).getResultType());
		StrictLiteralExpression* sle = createOneLiteral(*sleType);
		operExpr->addArgument(sle);
		assignExpr.addArgument(operExpr.release());
		Editing::replaceExpression(expr, assignExpression);
	}
	else if (expr.isBinaryComplexAssign())
	{
		if (expr.getArgumentCount() != 2)
			throw RepriseError("Unexpected complex assign expression with not 2 arguments");

		ReprisePtr<ExpressionBase> assignExpression(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN));
		BasicCallExpression& assignExpr = assignExpression->cast_to<BasicCallExpression>();
		assignExpr.addArgument(&expr.getArgument(0));

		ReprisePtr<BasicCallExpression> operExpr;
		makeCorrespondingOperation(expr.getKind(), operExpr);
		operExpr->addArgument(expr.getArgument(0).clone());
		operExpr->addArgument(&expr.getArgument(1));
		assignExpr.addArgument(operExpr.release());
		Editing::replaceExpression(expr, assignExpression);
	}
	else
		throw RepriseError("Unexpected expression in convert special expression function");
}

void convertCleanPostfixExpression(HirCCallExpression& expr)
{
	OPS_ASSERT(expr.getKind() == HirCCallExpression::HIRC_POSTFIX_MINUS_MINUS ||expr.getKind() == HirCCallExpression::HIRC_POSTFIX_PLUS_PLUS);
	ReprisePtr<TypeBase> tempType(expr.getArgument(0).getResultType());
	VariableDeclaration& tempVar = Editing::createNewVariable(*tempType, expr.obtainParentStatement()->getParentBlock());

	ReprisePtr<BasicCallExpression> tempAssignExpr(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN));
	tempAssignExpr->addArgument(new ReferenceExpression(tempVar));
	tempAssignExpr->addArgument(&expr.getArgument(0));

	ReprisePtr<BasicCallExpression> operExpr;
	makeCorrespondingOperation(expr.getKind(), operExpr);
	operExpr->addArgument(new ReferenceExpression(tempVar));
	operExpr->addArgument(createOneLiteral(tempVar.getType()));

	ReprisePtr<BasicCallExpression> assignExpr(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN));
	assignExpr->addArgument(expr.getArgument(0).clone());
	assignExpr->addArgument(operExpr.release());

	ReprisePtr<BasicCallExpression> secondCommaExpr(new BasicCallExpression(BasicCallExpression::BCK_COMMA));
	secondCommaExpr->addArgument(tempAssignExpr.release());
	secondCommaExpr->addArgument(assignExpr.release());

	ReprisePtr<ExpressionBase> topCommaExpression(new BasicCallExpression(BasicCallExpression::BCK_COMMA));
	BasicCallExpression& topCommaExpr = topCommaExpression->cast_to<BasicCallExpression>();
	topCommaExpr.addArgument(secondCommaExpr.release());
	topCommaExpr.addArgument(new ReferenceExpression(tempVar));
	Editing::replaceExpression(expr, topCommaExpression);
}

void convertSideEffectExpression(HirCCallExpression& expr)
{
	OPS_ASSERT(expr.isBinaryComplexAssign() || expr.isUnaryComplexAssign())
	if (expr.getArgument(0).is_a<BasicCallExpression>())
	{
		const bool postfix = (expr.getKind() == HirCCallExpression::HIRC_POSTFIX_MINUS_MINUS ||
			expr.getKind() == HirCCallExpression::HIRC_POSTFIX_PLUS_PLUS);
		BasicCallExpression& innerCall = expr.getArgument(0).cast_to<BasicCallExpression>();
		if (innerCall.getKind() == BasicCallExpression::BCK_DE_REFERENCE)
		{
			ReprisePtr<TypeBase> tempType(innerCall.getArgument(0).getResultType());
			VariableDeclaration& tempVar = Editing::createNewVariable(*tempType, expr.obtainParentStatement()->getParentBlock());

			ReprisePtr<BasicCallExpression> tempAssignExpr(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN,
				new ReferenceExpression(tempVar), &innerCall.getArgument(0)));

			ReprisePtr<BasicCallExpression> operExpr;
			makeCorrespondingOperation(expr.getKind(), operExpr);
			ReprisePtr<BasicCallExpression> deRefExpr(new BasicCallExpression(BasicCallExpression::BCK_DE_REFERENCE, 
				new ReferenceExpression(tempVar)));
			operExpr->addArgument(deRefExpr.get());
			if (expr.isBinaryComplexAssign())
				operExpr->addArgument(&expr.getArgument(1));
			else
				operExpr->addArgument(createOneLiteral(*deRefExpr->getResultType()));

			ReprisePtr<BasicCallExpression> assignExpr(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, 
				deRefExpr->clone(), operExpr.release()));
			ReprisePtr<ExpressionBase> commaExpression(new BasicCallExpression(BasicCallExpression::BCK_COMMA, 
				tempAssignExpr.release(), assignExpr.release()));
			if (postfix)
			{
				ReprisePtr<ExpressionBase> topCommaExpression(new BasicCallExpression(BasicCallExpression::BCK_COMMA, 
					commaExpression.release(), deRefExpr->clone()));
				Editing::replaceExpression(expr, topCommaExpression);
			}
			else
			{
				Editing::replaceExpression(expr, commaExpression);
			}
		}
		if (innerCall.getKind() == BasicCallExpression::BCK_TAKE_ADDRESS)
		{
			ReprisePtr<TypeBase> tempType(innerCall.getResultType());
			VariableDeclaration& tempVar = Editing::createNewVariable(*tempType, expr.obtainParentStatement()->getParentBlock());

			ReprisePtr<BasicCallExpression> tempAssignExpr(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, 
				new ReferenceExpression(tempVar), new BasicCallExpression(BasicCallExpression::BCK_TAKE_ADDRESS, &innerCall)));

			ReprisePtr<BasicCallExpression> operExpr;
			makeCorrespondingOperation(expr.getKind(), operExpr);
			ReprisePtr<BasicCallExpression> deRefExpr(new BasicCallExpression(BasicCallExpression::BCK_DE_REFERENCE, 
				new ReferenceExpression(tempVar)));
			operExpr->addArgument(deRefExpr.get());
			if (expr.isBinaryComplexAssign())
				operExpr->addArgument(&expr.getArgument(1));
			else
				operExpr->addArgument(createOneLiteral(*deRefExpr->getResultType()));

			ReprisePtr<BasicCallExpression> assignExpr(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, 
				deRefExpr->clone(), operExpr.release()));
			ReprisePtr<ExpressionBase> commaExpression(new BasicCallExpression(BasicCallExpression::BCK_COMMA,
				tempAssignExpr.release(), assignExpr.release()));
			if (postfix)
			{
				ReprisePtr<ExpressionBase> topCommaExpression(new BasicCallExpression(BasicCallExpression::BCK_COMMA, 
					commaExpression.release(), deRefExpr->clone()));
				Editing::replaceExpression(expr, topCommaExpression);
			}
			else
			{
				Editing::replaceExpression(expr, commaExpression);
			}
		}
		if (innerCall.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
		{
			ReprisePtr<TypeBase> tempArrayType(innerCall.getArgument(0).getResultType());
			ReprisePtr<TypeBase> tempArgType(innerCall.getArgument(1).getResultType());
			VariableDeclaration& tempArrayVar = Editing::createNewVariable(*tempArrayType, expr.obtainParentStatement()->getParentBlock());
			VariableDeclaration& tempArgVar = Editing::createNewVariable(*tempArgType, expr.obtainParentStatement()->getParentBlock());

			ReprisePtr<BasicCallExpression> tempArrayAssignExpr(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN,
				new ReferenceExpression(tempArrayVar), &innerCall.getArgument(0)));
			ReprisePtr<BasicCallExpression> tempArgAssignExpr(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN,
				new ReferenceExpression(tempArgVar), &innerCall.getArgument(1)));

			ReprisePtr<BasicCallExpression> operExpr;
			makeCorrespondingOperation(expr.getKind(), operExpr);
			if (postfix)
			{
				ReprisePtr<TypeBase> tempResultType(innerCall.getResultType());
				VariableDeclaration& tempResultVar = Editing::createNewVariable(*tempResultType, expr.obtainParentStatement()->getParentBlock());

				ReprisePtr<BasicCallExpression> arrAccessExpr(new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS, 
					new ReferenceExpression(tempArrayVar), new ReferenceExpression(tempArgVar)));
				ReprisePtr<BasicCallExpression> arrResultAssign(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN,
					new ReferenceExpression(tempResultVar), arrAccessExpr.get()));

				operExpr->addArgument(new ReferenceExpression(tempResultVar));
				if (expr.isBinaryComplexAssign())
					operExpr->addArgument(&expr.getArgument(1));
				else
					operExpr->addArgument(createOneLiteral(tempResultVar.getType()));

				ReprisePtr<BasicCallExpression> assignExpr(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, 
					arrAccessExpr->clone(), operExpr.release()));

				ReprisePtr<BasicCallExpression> secondCommaExpression(new BasicCallExpression(BasicCallExpression::BCK_COMMA, 
					tempArrayAssignExpr.release(), tempArgAssignExpr.release()));
				ReprisePtr<BasicCallExpression> thirdCommaExpression(new BasicCallExpression(BasicCallExpression::BCK_COMMA, 
					secondCommaExpression.release(), arrResultAssign.release()));
				ReprisePtr<BasicCallExpression> fourthCommaExpression(new BasicCallExpression(BasicCallExpression::BCK_COMMA,
					thirdCommaExpression.release(), assignExpr.release()));
				ReprisePtr<ExpressionBase> topCommaExpression(new BasicCallExpression(BasicCallExpression::BCK_COMMA, 
					fourthCommaExpression.release(), new ReferenceExpression(tempResultVar)));
				Editing::replaceExpression(expr, topCommaExpression);
			}
			else
			{
				ReprisePtr<BasicCallExpression> arrAccessExpr(new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS, 
					new ReferenceExpression(tempArrayVar), new ReferenceExpression(tempArgVar)));
				operExpr->addArgument(arrAccessExpr.get());
				if (expr.isBinaryComplexAssign())
					operExpr->addArgument(&expr.getArgument(1));
				else
					operExpr->addArgument(createOneLiteral(*arrAccessExpr->getResultType()));

				ReprisePtr<BasicCallExpression> assignExpr(new BasicCallExpression(BasicCallExpression::BCK_ASSIGN, 
					arrAccessExpr->clone(), operExpr.release()));

				ReprisePtr<BasicCallExpression> secondCommaExpression(new BasicCallExpression(BasicCallExpression::BCK_COMMA, 
					tempArrayAssignExpr.release(), tempArgAssignExpr.release()));
				ReprisePtr<ExpressionBase> topCommaExpression(new BasicCallExpression(BasicCallExpression::BCK_COMMA, 
					secondCommaExpression.release(), assignExpr.release()));
				Editing::replaceExpression(expr, topCommaExpression);
			}
		}
	}
	else
	{
		throw RepriseError("Unexpected call to convertSideEffectExpression()");
	}
}

void makeCorrespondingOperation(const HirCCallExpression::HirCOperatorKind kind, ReprisePtr<BasicCallExpression>& operExpr)
{
	switch (kind)
	{
	case HirCCallExpression::HIRC_PREFIX_MINUS_MINUS:
	case HirCCallExpression::HIRC_POSTFIX_MINUS_MINUS:
		operExpr.reset(new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS));
		break;
	case HirCCallExpression::HIRC_PREFIX_PLUS_PLUS:
	case HirCCallExpression::HIRC_POSTFIX_PLUS_PLUS:
		operExpr.reset(new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS));
		break;

	case HirCCallExpression::HIRC_PLUS_ASSIGN:				// +=
		operExpr.reset(new BasicCallExpression(BasicCallExpression::BCK_BINARY_PLUS));
		break;
	case HirCCallExpression::HIRC_MINUS_ASSIGN:				// -=
		operExpr.reset(new BasicCallExpression(BasicCallExpression::BCK_BINARY_MINUS));
		break;
	case HirCCallExpression::HIRC_MULTIPLY_ASSIGN:			// *=
		operExpr.reset(new BasicCallExpression(BasicCallExpression::BCK_MULTIPLY));
		break;
	case HirCCallExpression::HIRC_DIVISION_ASSIGN:			// /=
		operExpr.reset(new BasicCallExpression(BasicCallExpression::BCK_DIVISION));
		break;
	case HirCCallExpression::HIRC_MOD_ASSIGN:				// %=
		operExpr.reset(new BasicCallExpression(BasicCallExpression::BCK_INTEGER_MOD));
		break;
	case HirCCallExpression::HIRC_LSHIFT_ASSIGN:				// <<=
		operExpr.reset(new BasicCallExpression(BasicCallExpression::BCK_LEFT_SHIFT));
		break;
	case HirCCallExpression::HIRC_RSHIFT_ASSIGN:				// >>=
		operExpr.reset(new BasicCallExpression(BasicCallExpression::BCK_RIGHT_SHIFT));
		break;

	case HirCCallExpression::HIRC_BAND_ASSIGN:				// &=
		operExpr.reset(new BasicCallExpression(BasicCallExpression::BCK_BITWISE_AND));
		break;
	case HirCCallExpression::HIRC_BOR_ASSIGN:				// |=
		operExpr.reset(new BasicCallExpression(BasicCallExpression::BCK_BITWISE_OR));
		break;
	case HirCCallExpression::HIRC_BXOR_ASSIGN:				// ^=
		operExpr.reset(new BasicCallExpression(BasicCallExpression::BCK_BITWISE_XOR));
		break;

		OPS_DEFAULT_CASE_LABEL
	}
}

ArrayType* getRepriseArray(HirFArrayType* fortranArray)
{
	unique_ptr<ArrayType> result;
 	int rank = fortranArray->getShape().getRank();

	for (int i = 0; i < rank; i++)
	{
		if (result.get() == 0)
			result.reset(new ArrayType(fortranArray->getBaseType().clone()));
		else
			result.reset(new ArrayType(result.release()));
		

		HirFDimensionExpression& nextDimension = fortranArray->getShape().getDimension(i);		
		ReprisePtr<ExpressionBase> lowerBoundExpr(OPS::ExpressionSimplifier::Simplifier().simplify(&nextDimension.getLowerBound()));
		ReprisePtr<ExpressionBase> upperBoundExpr(OPS::ExpressionSimplifier::Simplifier().simplify(&(nextDimension.getUpperBound())));
		StrictLiteralExpression* lowerBoundStrict = lowerBoundExpr->cast_ptr<StrictLiteralExpression>();
		StrictLiteralExpression* upperBoundStrict = upperBoundExpr->cast_ptr<StrictLiteralExpression>();
		
		if (lowerBoundStrict == 0 || upperBoundStrict == 0 || !lowerBoundStrict->isInteger()
			|| !upperBoundStrict->isInteger())
			return 0;
		
		int lowerValue = lowerBoundStrict->getInt32();
		int upperValue = upperBoundStrict->getInt32();

		if (lowerValue != 0)
			return 0;
		
		int bound = upperValue + 1;
		result->setElementCount(bound);
	}
	return result.release();
}

ExpressionBase* getArrayItemReference(BasicCallExpression* fortranItemReference)
{
	BasicCallExpression* result = 0;
	
	if (fortranItemReference->getArgument(1).is_a<HirFArrayIndexRangeExpression>() == false)
		return fortranItemReference;

	HirFArrayIndexRangeExpression* rightExpression =
		fortranItemReference->getArgument(1).cast_ptr<HirFArrayIndexRangeExpression>();

	result = new BasicCallExpression(BasicCallExpression::BCK_ARRAY_ACCESS, fortranItemReference->getArgument(0).clone());
	for (int i = 0; i < rightExpression->getArgumentCount(); i++)
		result->addArgument(rightExpression->getArgument(i).clone());
	return result;
}

class ArrayAccessSearcher : public DeepWalker
{
public:
	using DeepWalker::visit;

	void visit(BasicCallExpression& basicCallExpression)
	{
		for (int index = 0; index < basicCallExpression.getArgumentCount(); ++index)
			basicCallExpression.getArgument(index).accept(*this);	

		if (basicCallExpression.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS)
			arrayAccessNodes.push_back(&basicCallExpression);				
	}		

	void start()
	{	
		it = arrayAccessNodes.begin();
	}

	BasicCallExpression* getNextArrayAccessNode()
	{
		if (it != arrayAccessNodes.end())
		{
			BasicCallExpression* result = *it;
			++it;
			return result;
		}
		return 0;
	}
		
private:
	typedef std::list<BasicCallExpression*>  NodesContainer;
	NodesContainer arrayAccessNodes;
	NodesContainer::iterator it;
	
};



void convertHirFArrays(RepriseBase& repriseNode)
{		
	ArrayAccessSearcher searcher;	
	repriseNode.accept(searcher);
	
	searcher.start();
	BasicCallExpression* nextArrayAccessExpression = 0;
	while ((nextArrayAccessExpression = searcher.getNextArrayAccessNode()) != 0)
	{			
		ExpressionBase* newArrayItemReference = getArrayItemReference(nextArrayAccessExpression);
		if (newArrayItemReference == 0)  continue;

		RepriseBase* parent = nextArrayAccessExpression->getParent();		
		
		for(int i = 0; i < parent->getChildCount(); i++)
			if (&parent->getChild(i) == nextArrayAccessExpression)
				OPS::Reprise::Editing::replaceExpression(*nextArrayAccessExpression, ReprisePtr<ExpressionBase>(newArrayItemReference));	
	}	

	// Changing variables type (from HirFArrayType to ArrayType)...
	NodesFinder<VariableDeclaration> varDeclsFinder;
	repriseNode.accept(varDeclsFinder);
	
	int i = -1;
	for (NodesFinder<VariableDeclaration>::NodeListType::iterator iter = varDeclsFinder.NodeList.begin(); 
			iter !=  varDeclsFinder.NodeList.end(); ++iter)
	{
		i++;
		VariableDeclaration* nextVarDecl = *iter;
		if (nextVarDecl->getType().is_a<HirFArrayType>())
		{
			ArrayType* newArrayType = getRepriseArray(nextVarDecl->getType().cast_ptr<HirFArrayType>());

			if (newArrayType != 0)
			{
				//	throw RuntimeError("Fortran Canto error: Declared array type can't be parser to Reprise.");
				nextVarDecl->setType(newArrayType);
			}
			
		}		
	}
}

class StrictLiteralsWalker : public DeepWalker
{
public:
	using DeepWalker::visit;

	typedef std::vector<StrictLiteralExpression*> LiteralsList;

	inline void visit(StrictLiteralExpression& litExpr)
	{
		m_list.push_back(&litExpr);
		DeepWalker::visit(litExpr);
	}

	LiteralsList m_list;
};

void convertStrictToBasic(RepriseBase& repriseNode)
{
	StrictLiteralsWalker slw;
	repriseNode.accept(slw);
	for (StrictLiteralsWalker::LiteralsList::iterator it = slw.m_list.begin(); it != slw.m_list.end(); ++it)
	{
		StrictLiteralExpression& sle = **it;
		ReprisePtr<ExpressionBase> result;
		switch (sle.getLiteralType())
		{
		case BasicType::BT_CHAR:
			result.reset(BasicLiteralExpression::createChar(sle.getChar()));
			break;
		case BasicType::BT_WIDE_CHAR:
			result.reset(BasicLiteralExpression::createWideChar(sle.getWideChar()));
			break;
		case BasicType::BT_INT8:
			result.reset(BasicLiteralExpression::createInteger(sle.getInt8()));
			break;
		case BasicType::BT_INT16:
			result.reset(BasicLiteralExpression::createInteger(sle.getInt16()));
			break;
		case BasicType::BT_INT32:
			result.reset(BasicLiteralExpression::createInteger(sle.getInt32()));
			break;
		case BasicType::BT_INT64:
			result.reset(BasicLiteralExpression::createInteger(sle.getInt64()));
			break;
		case BasicType::BT_UINT8:
			result.reset(BasicLiteralExpression::createUnsignedInteger(sle.getUInt8()));
			break;
		case BasicType::BT_UINT16:
			result.reset(BasicLiteralExpression::createUnsignedInteger(sle.getUInt16()));
			break;
		case BasicType::BT_UINT32:
			result.reset(BasicLiteralExpression::createUnsignedInteger(sle.getUInt32()));
			break;
		case BasicType::BT_UINT64:
			result.reset(BasicLiteralExpression::createUnsignedInteger(sle.getUInt64()));
			break;

		case BasicType::BT_FLOAT32:
			result.reset(BasicLiteralExpression::createFloat(sle.getFloat32()));
			break;
		case BasicType::BT_FLOAT64:
			result.reset(BasicLiteralExpression::createFloat(sle.getFloat64()));
			break;

		case BasicType::BT_BOOLEAN:
			result.reset(BasicLiteralExpression::createBoolean(sle.getBoolean()));
			break;

		case BasicType::BT_STRING:
			result.reset(BasicLiteralExpression::createString(sle.getString()));
			break;
		case BasicType::BT_WIDE_STRING:
			result.reset(BasicLiteralExpression::createWideString(sle.getWideString()));
			break;

		case BasicType::BT_VOID:
			break;

			OPS_DEFAULT_CASE_LABEL
		}

		Editing::replaceExpression(**it, result);
	}
}

StrictLiteralExpression* createOneLiteral(TypeBase& baseType)
{
	StrictLiteralExpression* literal = 0;
	if (baseType.is_a<BasicType>())
	{
		BasicType& argumentType = baseType.cast_to<BasicType>();
		literal = new StrictLiteralExpression(argumentType.getKind());
		literal->setOne();
		return literal;
	}
	else if (baseType.is_a<PtrType>())
	{
		literal = new StrictLiteralExpression(BasicType::BT_INT32);
		literal->setOne();
		return literal;
	}
	else if (baseType.is_a<EnumType>())
	{
		literal = new StrictLiteralExpression(BasicType::BT_INT32);
		literal->setOne();
		return literal;
	}
	else if (baseType.is_a<TypedefType>())
	{
		return createOneLiteral(baseType.cast_to<TypedefType>().getBaseType());
	}
	else if (baseType.is_a<DeclaredType>())
	{
		return createOneLiteral(baseType.cast_to<DeclaredType>().getDeclaration().getType());
	}
	else
	{
		throw RepriseError("Unexpected expression type in createOneLiteral()");
	}
}


}
}
}
