/*
    Reprise/ServiceFunctions.cpp - Reprise module, ServiceFunctions implementation

*/

//  Standard includes
#include <memory>

//  OPS includes

//  Local includes
#include "Reprise/ServiceFunctions.h"
#include "Reprise/Declarations.h"
#include "Reprise/Statements.h"

#include "Reprise/Service/Service.h"

//  Namespaces using

//  Defines and macros

//  Enter namespace
namespace OPS
{
namespace Reprise
{
namespace Editing
{
//  Constants and enums

//  Classes
class ExpressionTypeWalker : public Service::DeepWalker
{
public:
	enum WalkKind
	{
		WK_NORMAL,
		WK_PRECISE
	};

	explicit ExpressionTypeWalker(WalkKind walkKind) : m_walkKind(walkKind)
	{
	}

	ReprisePtr<TypeBase> getResult(void)
	{
		if (m_stack.empty())
			return ReprisePtr<TypeBase>();
		if (m_stack.size() != 1)
		{
			return ReprisePtr<TypeBase>();
		}
		return ReprisePtr<TypeBase>(m_stack[0]);
	}

	void visit(BasicLiteralExpression& expr)
	{
		// TODO: Rewrite method in proper way (actually we should eliminate BasicLiteralExpression nodes from program before call to this method).
		switch (expr.getLiteralType())
		{
		case BasicLiteralExpression::LT_UNDEFINED:
			throw RepriseError(Strings::format("BasicLiteralExpression::getResultType(). Unexpected literal type (%u).", expr.getLiteralType()));
		case BasicLiteralExpression::LT_CHAR:
			push(BasicType::charType());
			break;
		case BasicLiteralExpression::LT_WIDE_CHAR:
			push(BasicType::wideCharType());
			break;
		case BasicLiteralExpression::LT_INTEGER:
			push(BasicType::int32Type());
			break;
		case BasicLiteralExpression::LT_UNSIGNED_INTEGER:
			push(BasicType::uint32Type());
			break;
		case BasicLiteralExpression::LT_FLOAT:
			push(BasicType::float32Type());
			break;
		case BasicLiteralExpression::LT_BOOLEAN:
			push(BasicType::booleanType());
			break;
		case BasicLiteralExpression::LT_STRING:
			{
				if (m_walkKind == WK_NORMAL)
				{
					PtrType* resultType(new PtrType(BasicType::charType()));
					resultType->getPointedType().setConst(true);
					push(resultType);
				}
				else
				{
					push(BasicType::basicType(BasicType::BT_STRING));
				}
				break;
			}
		case BasicLiteralExpression::LT_WIDE_STRING:
			{
				if (m_walkKind == WK_NORMAL)
				{
					PtrType* resultType(new PtrType(BasicType::wideCharType()));
					resultType->getPointedType().setConst(true);
					push(resultType);
				}
				else
				{
					push(BasicType::basicType(BasicType::BT_WIDE_STRING));
				}
				break;
			}

			OPS_DEFAULT_CASE_LABEL
		}
		//throw NotEmplementedError("ExpressionTypeWalker::visit(BasicLiteralExpression&)");
	}
	
	void visit(StrictLiteralExpression& expr)
	{
		push(BasicType::basicType(expr.getLiteralType()));
	}

	void visit(CompoundLiteralExpression&)
	{
		throw NotEmplementedError("ExpressionTypeWalker::visit(CompoundLiteralExpression&)");
	}

	void visitReference(TypeBase& typeBase)
	{
		// TODO: Make proper implementation
		if(m_walkKind == WK_NORMAL)
		{
			if (PtrType* ptrType = typeBase.cast_ptr<PtrType>())
			{
				visitReference(ptrType->getPointedType());
				push(new PtrType(pop()));
				return;
			}
			if (ArrayType* arrayType = typeBase.cast_ptr<ArrayType>())
			{
				visitReference(arrayType->getBaseType());
				push(new PtrType(pop()));
				return;
			}
			push(typeBase.clone());
		}
		else
		{
			push(typeBase.clone());
		}
	}

	void visit(ReferenceExpression& expr)
	{
		visitReference(expr.getReference().getType());
	}
	
	void visit(SubroutineReferenceExpression& expr)
	{
		push(expr.getReference().getType().clone());
	}
	
	void visit(StructAccessExpression& expr)
	{
		visitReference(expr.getMember().getType());
	}
	
	void visit(EnumAccessExpression&)
	{
		// TODO: Make it in proper way. in C this is nearly true, but for other languages...
		push(BasicType::int32Type());
	}
	
	void visit(TypeCastExpression& expr)
	{
		push(expr.getCastType().clone());
	}

	BasicType::BasicTypes getPromotedKind(BasicType::BasicTypes kind)
	{
		if (kind == BasicType::BT_CHAR ||
			kind == BasicType::BT_WIDE_CHAR ||
			kind == BasicType::BT_INT8 ||
			kind == BasicType::BT_INT16 ||
			kind == BasicType::BT_UINT8 ||
			kind == BasicType::BT_UINT16
			)
		{
			return BasicType::BT_INT32;
		}
		else
		{
			return kind;
		}
	}

	BasicType* getPromotedType(ReprisePtr<TypeBase>& type)
	{
		BasicType& basicType = type->cast_to<BasicType>();
		BasicType::BasicTypes promotedKind = getPromotedKind(basicType.getKind());
		if (promotedKind != basicType.getKind() ||
			&basicType != type.get())
			return BasicType::basicType(promotedKind);

		type.release();
		return &basicType;
	}

	void visitUnaryBasicCall(BasicCallExpression& expr)
	{
		switch (expr.getKind())
		{
			//	Unary
		case BasicCallExpression::BCK_UNARY_PLUS:
		case BasicCallExpression::BCK_UNARY_MINUS:
		case BasicCallExpression::BCK_BITWISE_NOT:		// ~
			{
				expr.getArgument(0).accept(*this);
				ReprisePtr<TypeBase> argType(pop());
				argType = ReprisePtr<TypeBase>(&desugarType(*argType));

				if (argType->is_a<VectorType>())
					push(argType.release());
				else
					push(getPromotedType(argType));
				break;
			}
		case BasicCallExpression::BCK_SIZE_OF:			// sizeof() operator
			// TODO: This one for example.
			push(BasicType::basicType(BasicType::BT_UINT32));
			break;

		case BasicCallExpression::BCK_TAKE_ADDRESS:		// &
			{
				expr.getArgument(0).accept(*this);
				TypeBase* argType = pop();
				push(new PtrType(argType));
				break;
			}

		case BasicCallExpression::BCK_DE_REFERENCE:		// *
			{
				expr.getArgument(0).accept(*this);
				std::unique_ptr<TypeBase> argumentType(pop());

				if (PtrType* ptrType = argumentType->cast_ptr<PtrType>())
				{
					push(ptrType->getPointedType().clone());
				}
				else if (ArrayType* arrayType = argumentType->cast_ptr<ArrayType>())
				{
					push(arrayType->getBaseType().clone());
				}
				else
					throw RepriseError("Unexpected argument type in DE_REF expression.");
				break;
			}
		case BasicCallExpression::BCK_LOGICAL_NOT:		// !
			{
                push(BasicType::int32Type());
				break;
			}
		default:
			throw NotEmplementedError(Strings::format("Unknown unary kind (%u). ExpressionTypeWalker::visitUnaryBasicCall()", expr.getKind()));
		}
	}

    int getBasicKindRank(BasicType::BasicTypes kind)
    {
        switch(kind)
        {
        case BasicType::BT_INT32: return 0;
        case BasicType::BT_UINT32: return 1;
        case BasicType::BT_INT64: return 2;
        case BasicType::BT_UINT64: return 3;
        case BasicType::BT_INT128: return 4;
        case BasicType::BT_UINT128: return 5;
        case BasicType::BT_FLOAT32: return 6;
        case BasicType::BT_FLOAT64: return 7;
        OPS_DEFAULT_CASE_LABEL
        }
        return -1;
    }

	void visitBinaryBasicCall(BasicCallExpression& expr)
	{
		//	Binary

		switch (expr.getKind())
		{
		case BasicCallExpression::BCK_BINARY_PLUS:		// +
		case BasicCallExpression::BCK_BINARY_MINUS:		// -
		case BasicCallExpression::BCK_MULTIPLY:			// *
		case BasicCallExpression::BCK_DIVISION:			// / 
		case BasicCallExpression::BCK_INTEGER_DIVISION:	// div
		case BasicCallExpression::BCK_INTEGER_MOD:		// mod (%)
            //	Bitwise
        case BasicCallExpression::BCK_BITWISE_AND:		// &
        case BasicCallExpression::BCK_BITWISE_OR:		// |
        case BasicCallExpression::BCK_BITWISE_XOR:		// ^
        {
            expr.getArgument(1).accept(*this);
            expr.getArgument(0).accept(*this);

			std::unique_ptr<TypeBase> arg0Type(pop()), arg1Type(pop());

			if (BasicType* arg0Basic = Editing::desugarType(*arg0Type).cast_ptr<BasicType>())
            {
				if(BasicType* arg1Basic = Editing::desugarType(*arg1Type).cast_ptr<BasicType>())
                {
                    BasicType::BasicTypes arg0PromotedKind = getPromotedKind(arg0Basic->getKind())
                                         ,arg1PromotedKind = getPromotedKind(arg1Basic->getKind());
                    if (arg0PromotedKind == arg1PromotedKind ||
                        getBasicKindRank(arg0PromotedKind) > getBasicKindRank(arg1PromotedKind))
                    {
                        push(BasicType::basicType(arg0PromotedKind));
                    }
                    else
                    {
                        push(BasicType::basicType(arg1PromotedKind));
                    }
                }
                else
                    push(arg1Type.release());
            }
            else
                push(arg0Type.release());
        }
        break;

		//	Assign
		case BasicCallExpression::BCK_ASSIGN:				// =
			expr.getArgument(0).accept(*this);
			break;

        //	Equality
		case BasicCallExpression::BCK_LESS:				// <
		case BasicCallExpression::BCK_GREATER:			// >
		case BasicCallExpression::BCK_LESS_EQUAL:		// <=
		case BasicCallExpression::BCK_GREATER_EQUAL:		// >=
		case BasicCallExpression::BCK_EQUAL:				// ==
		case BasicCallExpression::BCK_NOT_EQUAL:			// !=
            push(BasicType::int32Type());
			break;

        //	Shifts
		case BasicCallExpression::BCK_LEFT_SHIFT:			// <<
		case BasicCallExpression::BCK_RIGHT_SHIFT:		// >>
        {
			expr.getArgument(0).accept(*this);
			ReprisePtr<TypeBase> argType(pop());
			argType = ReprisePtr<TypeBase>(&desugarType(*argType));

			if (argType->is_a<VectorType>())
				push(argType.release());
			else
				push(getPromotedType(argType));
			break;
        }

				//	Logical
		case BasicCallExpression::BCK_LOGICAL_AND:		// &&
		case BasicCallExpression::BCK_LOGICAL_OR:			// ||
            push(BasicType::int32Type());
			break;

		default:
			throw NotEmplementedError(Strings::format("Unknown binary kind (%u). ExpressionTypeWalker::visitBinaryBasicCall()", expr.getKind()));
		}
	}

    void visitBasicCallSpecial(BasicCallExpression& expr)
	{
		//	Special
		switch (expr.getKind())
		{
		case BasicCallExpression::BCK_ARRAY_ACCESS:		// []
			{
				expr.getArgument(0).accept(*this);
				ReprisePtr<TypeBase> resType(pop());

				for (int unwrap = 0; unwrap < expr.getArgumentCount() - 1; ++unwrap)
				{
					TypeBase* desugaredType = &Editing::desugarType(*resType);
					if (PtrType* ptrType = desugaredType->cast_ptr<PtrType>())
					{
						resType = ReprisePtr<TypeBase>(&ptrType->getPointedType());
					}
					else if (ArrayType* arrayType = desugaredType->cast_ptr<ArrayType>())
					{
						resType = ReprisePtr<TypeBase>(&arrayType->getBaseType());
					}
					else if (VectorType* vectorType = desugaredType->cast_ptr<VectorType>())
					{
						resType = ReprisePtr<TypeBase>(&vectorType->getBaseType());
					}
					else
					{
						throw StateError("Unexpected type in array access");
					}
				}
				push(resType->clone());
				break;
			}
		case BasicCallExpression::BCK_COMMA:
			{
				expr.getArgument(0).accept(*this);
				break;
			}
		case BasicCallExpression::BCK_CONDITIONAL:
			{
				expr.getArgument(1).accept(*this);
				break;
			}
		default:
			throw NotEmplementedError(Strings::format("Unknown generic kind (%u). ExpressionTypeWalker::visitBasicCallGeneric()", expr.getKind()));
		}
	}

	void visit(BasicCallExpression& expr)
	{
		// TODO: Some operations need more precise analysis
		if (expr.getKind() == BasicCallExpression::BCK_ARRAY_ACCESS ||
			expr.getKind() == BasicCallExpression::BCK_COMMA ||
			expr.getKind() == BasicCallExpression::BCK_CONDITIONAL)
		{
            visitBasicCallSpecial(expr);
		}
		else
		{
			switch (expr.getArgumentCount())
			{
			case 1:
				visitUnaryBasicCall(expr);
				break;
			case 2:
				visitBinaryBasicCall(expr);
				break;
			OPS_DEFAULT_CASE_LABEL
			}
		}
	}
	
	void visit(SubroutineCallExpression& expr)
	{
		expr.getCallExpression().accept(*this);
        ReprisePtr<TypeBase> callExprType(&OPS::Reprise::Editing::desugarType(*pop()));

		while(callExprType->is_a<PtrType>())
		{
            callExprType = ReprisePtr<TypeBase>(&OPS::Reprise::Editing::desugarType(
                                                    callExprType->cast_to<PtrType>().getPointedType()));
		}
		OPS_ASSERT(callExprType->is_a<SubroutineType>());
		push(callExprType->cast_to<SubroutineType>().getReturnType().clone());
	}
	
	void visit(EmptyExpression&)
	{
		push(BasicType::voidType());
	}

	void visitUnaryCCall(Canto::HirCCallExpression& expr)
	{
		using namespace Canto;
		switch (expr.getKind())
		{
			//	Unary
		case HirCCallExpression::HIRC_UNARY_PLUS:
		case HirCCallExpression::HIRC_UNARY_MINUS:
			expr.getArgument(0).accept(*this);
			break;

		case HirCCallExpression::HIRC_SIZE_OF:			// sizeof() operator
			// TODO: This one for example.
			push(BasicType::basicType(BasicType::BT_UINT32));
			break;

		case HirCCallExpression::HIRC_TAKE_ADDRESS:		// &
			{
				expr.getArgument(0).accept(*this);
				TypeBase* argType = pop();
				push(new PtrType(argType));
				break;
			}

		case HirCCallExpression::HIRC_DE_REFERENCE:		// *
			{
				expr.getArgument(0).accept(*this);
				std::unique_ptr<TypeBase> argumentType(pop());

				if (PtrType* ptrType = argumentType->cast_ptr<PtrType>())
				{
					push(ptrType->getPointedType().clone());
				}
				else if (ArrayType* arrayType = argumentType->cast_ptr<ArrayType>())
				{
					push(arrayType->getBaseType().clone());
				}
				else
					throw RepriseError("Unexpected argument type in DE_REF expression.");
				break;
			}
		case HirCCallExpression::HIRC_PREFIX_PLUS_PLUS:
		case HirCCallExpression::HIRC_POSTFIX_PLUS_PLUS:
		case HirCCallExpression::HIRC_PREFIX_MINUS_MINUS:
		case HirCCallExpression::HIRC_POSTFIX_MINUS_MINUS:
			expr.getArgument(0).accept(*this);
			break;

		default:
			throw NotEmplementedError(Strings::format("Unknown unary kind (%u). ExpressionTypeWalker::visitUnaryCCall()", expr.getKind()));
		}
	}

	void visitBinaryCCall(Canto::HirCCallExpression& expr)
	{
		using namespace Canto;
		//	Binary
		switch (expr.getKind())
		{

		//	Binary
		case HirCCallExpression::HIRC_BINARY_PLUS:		// +
		case HirCCallExpression::HIRC_BINARY_MINUS:		// -
		case HirCCallExpression::HIRC_MULTIPLY:			// *
		case HirCCallExpression::HIRC_DIVISION:			// / 
		case HirCCallExpression::HIRC_INTEGER_MOD:		// mod (%)
	
		//	Assign
		case HirCCallExpression::HIRC_ASSIGN:				// =
		case HirCCallExpression::HIRC_PLUS_ASSIGN:				// +=
		case HirCCallExpression::HIRC_MINUS_ASSIGN:				// -=
		case HirCCallExpression::HIRC_MULTIPLY_ASSIGN:			// *=
		case HirCCallExpression::HIRC_DIVISION_ASSIGN:			// /=
		case HirCCallExpression::HIRC_MOD_ASSIGN:				// %=
		case HirCCallExpression::HIRC_LSHIFT_ASSIGN:				// <<=
		case HirCCallExpression::HIRC_RSHIFT_ASSIGN:				// >>=

		case HirCCallExpression::HIRC_BAND_ASSIGN:				// &=
		case HirCCallExpression::HIRC_BOR_ASSIGN:				// |=
		case HirCCallExpression::HIRC_BXOR_ASSIGN:				// ^=
			expr.getArgument(0).accept(*this);
			break;

		//	Equality
		case HirCCallExpression::HIRC_LESS:				// <
		case HirCCallExpression::HIRC_GREATER:			// >
		case HirCCallExpression::HIRC_LESS_EQUAL:		// <=
		case HirCCallExpression::HIRC_GREATER_EQUAL:		// >=
		case HirCCallExpression::HIRC_EQUAL:				// ==
		case HirCCallExpression::HIRC_NOT_EQUAL:			// !=
            push(BasicType::int32Type());
			break;

			//	Shifts
		case HirCCallExpression::HIRC_LEFT_SHIFT:			// <<
		case HirCCallExpression::HIRC_RIGHT_SHIFT:			// >>
			expr.getArgument(0).accept(*this);
			break;

			//	Logical
		case HirCCallExpression::HIRC_LOGICAL_NOT:		// !
		case HirCCallExpression::HIRC_LOGICAL_AND:		// &&
		case HirCCallExpression::HIRC_LOGICAL_OR:			// ||
            push(BasicType::int32Type());
			break;

			//	Bitwise
		case HirCCallExpression::HIRC_BITWISE_NOT:		// ~
		case HirCCallExpression::HIRC_BITWISE_AND:		// &
		case HirCCallExpression::HIRC_BITWISE_OR:			// |
		case HirCCallExpression::HIRC_BITWISE_XOR:		// ^
			expr.getArgument(0).accept(*this);
			break;

		default:
			throw NotEmplementedError(Strings::format("Unknown binary kind (%u). ExpressionTypeWalker::visitBinaryCCall()", expr.getKind()));
		}
	}

	void visitCCallGeneric(Canto::HirCCallExpression& expr)
	{
		using namespace Canto;
		//	Special
		switch (expr.getKind())
		{
		case HirCCallExpression::HIRC_ARRAY_ACCESS:		// []
			{
				expr.getArgument(0).accept(*this);
				ReprisePtr<TypeBase> resType(pop());

				for (int unwrap = 0; unwrap < expr.getArgumentCount() - 1; ++unwrap)
				{
					if (resType->is_a<PtrType>())
					{
						resType.reset(resType->cast_to<PtrType>().getPointedType().clone());
					}
					else if(resType->is_a<ArrayType>())
					{
						resType.reset(resType->cast_to<ArrayType>().clone());
					}
				}
				push(resType.release());
				break;
			}
		case HirCCallExpression::HIRC_COMMA:
			{
				expr.getArgument(0).accept(*this);
				break;
			}
		case HirCCallExpression::HIRC_CONDITIONAL:
			{
				// NOTE: 1st or 2nd argument!
				expr.getArgument(1).accept(*this);
				break;
			}
		default:
			throw NotEmplementedError(Strings::format("Unknown generic kind (%u). ExpressionTypeWalker::visitCCallGeneric()", expr.getKind()));
		}
	}

	void visit(Canto::HirCCallExpression& expr)
	{
		// TODO: Some operations need more precise analisys
		if (expr.getKind() == Canto::HirCCallExpression::HIRC_ARRAY_ACCESS ||
			expr.getKind() == Canto::HirCCallExpression::HIRC_COMMA ||
			expr.getKind() == Canto::HirCCallExpression::HIRC_CONDITIONAL)
		{
			visitCCallGeneric(expr);
		}
		else
		{
			switch (expr.getArgumentCount())
			{
			case 1:
				visitUnaryCCall(expr);
				break;
			case 2:
				visitBinaryCCall(expr);
				break;
			OPS_DEFAULT_CASE_LABEL
			}
		}
	}

	//void visit(Canto::HirFAltResultExpression&);
	//void visit(Canto::HirFAsteriskExpression&);
	//void visit(Canto::HirFDimensionExpression&);
	//void visit(Canto::HirFArrayShapeExpression&);
	//void visit(Canto::HirFArrayIndexRangeExpression&);
	//void visit(Canto::HirFImpliedDoExpression&);
	//void visit(Canto::HirFArgumentPairExpression&);
	//void visit(Canto::HirFIntrinsicCallExpression&);
	//void visit(Canto::HirFIntrinsicReferenceExpression&);

private:
	typedef std::vector<TypeBase*> ResultStack;

	void push(TypeBase* tb)
	{
		m_stack.push_back(tb);
	}

	TypeBase* pop(void)
	{
		if (m_stack.empty())
			throw RuntimeError("Operations stack is empty.");
		TypeBase* result = m_stack.back();
		m_stack.pop_back();
		return result;
	}

	WalkKind m_walkKind;
	ResultStack m_stack;
};

//  Functions declaration
static bool subsHelper(ReprisePtr<ExpressionBase>& current, const std::string& note, ReprisePtr<ExpressionBase> destinationExpr);
static bool checkTypeVolatile(TypeBase& type);

//  Variables

//  Classes implementation

//  Global classes implementation

//  Functions implementation
VariableDeclaration& createNewVariable(const TypeBase& type, BlockStatement& block, const std::string& partName)
{
	SubroutineDeclaration& subroutine = block.getRootBlock().getParent()->cast_to<SubroutineDeclaration>();
	VariableDeclaration* tempVarDecl = new VariableDeclaration(type.clone(), generateUniqueIndentifier(partName));
	tempVarDecl->setDefinedBlock(block);
	subroutine.getDeclarations().addVariable(tempVarDecl);
	return *tempVarDecl;
}

VariableDeclaration& createNewGlobalVariable(const TypeBase& type, TranslationUnit& unit, const std::string& partName)
{
	VariableDeclaration* tempVarDecl = new VariableDeclaration(type.clone(), generateUniqueIndentifier(partName));
	unit.getGlobals().addVariable(tempVarDecl);
	return *tempVarDecl;
}

ReprisePtr<ExpressionBase> replaceExpression(ExpressionBase& sourceExpr, ReprisePtr<ExpressionBase> destinationExpr)
{
	ReprisePtr<ExpressionBase> replacedExpr(&sourceExpr);
	if (sourceExpr.getParent() == 0)
	{
		replacedExpr = destinationExpr;
	}
	// SubroutineCallExpression must be visited before CallExpressionBase!
	else if (sourceExpr.getParent()->is_a<SubroutineCallExpression>())
	{
		SubroutineCallExpression& parent = sourceExpr.getParent()->cast_to<SubroutineCallExpression>();
		if (&parent.getCallExpression() == &sourceExpr)
			parent.setCallExpression(destinationExpr);
		else
			parent.replaceArgument(sourceExpr, destinationExpr);
	}
	else if (sourceExpr.getParent()->is_a<CallExpressionBase>())
	{
		CallExpressionBase& parentExpression = sourceExpr.getParent()->cast_to<CallExpressionBase>();
		parentExpression.replaceArgument(sourceExpr, destinationExpr);
	}
	else if (sourceExpr.getParent()->is_a<StatementBase>())
	{
		StatementBase& parentStatement = sourceExpr.getParent()->cast_to<StatementBase>();
		parentStatement.replaceExpression(sourceExpr, destinationExpr);
	}
	else if (sourceExpr.getParent()->is_a<StructAccessExpression>())
	{
		StructAccessExpression& parent = sourceExpr.getParent()->cast_to<StructAccessExpression>();
		if (&parent.getStructPointerExpression() != &sourceExpr)
			throw RepriseError("Inconsistent parents.");
		parent.setStructPointerExpression(destinationExpr);
	}
	else if (sourceExpr.getParent()->is_a<CompoundLiteralExpression>())
	{
		CompoundLiteralExpression& parent = sourceExpr.getParent()->cast_to<CompoundLiteralExpression>();
		parent.replaceValue(sourceExpr, destinationExpr);
	}
	else if (sourceExpr.getParent()->is_a<TypeCastExpression>())
	{
		TypeCastExpression& parent = sourceExpr.getParent()->cast_to<TypeCastExpression>();
		if (&parent.getCastArgument() != &sourceExpr)
			throw RepriseError("Inconsistent parents.");
		parent.setCastArgument(destinationExpr);
	}
	else if (sourceExpr.getParent()->is_a<IReplaceChildExpression>())
	{
		IReplaceChildExpression& parent = sourceExpr.getParent()->cast_to<IReplaceChildExpression>();
		parent.replaceChildExpression(sourceExpr, destinationExpr);
	}
	else if (sourceExpr.getParent()->is_a<VariableDeclaration>())
	{
		VariableDeclaration& parent = sourceExpr.getParent()->cast_to<VariableDeclaration>();
		if (&parent.getInitExpression() != &sourceExpr)
			throw RepriseError("Inconsistent parents.");
		parent.setInitExpression(*destinationExpr.release());
	}
	else if (sourceExpr.getParent()->is_a<ArrayType>())
	{
		ArrayType& parent = sourceExpr.getParent()->cast_to<ArrayType>();
		if (&parent.getCountExpression() != &sourceExpr)
			throw RepriseError("Inconsistent parents.");
		parent.setCountExpression(destinationExpr.release());
	}
	else
	{
		throw RepriseError(std::string("Unexpected expression parent: ") + typeid(*sourceExpr.getParent()).name());
	}

	return replacedExpr;
}

void substituteExpression(ReprisePtr<ExpressionBase>& baseExpr, const std::string& note, ReprisePtr<ExpressionBase> destinationExpr)
{
	OPS_ASSERT(baseExpr->getParent() == 0)
	subsHelper(baseExpr, note, destinationExpr);
}

bool subsHelper(ReprisePtr<ExpressionBase>& current, const std::string& note, ReprisePtr<ExpressionBase> destinationExpr)
{
	if (current->hasNote(note))
	{
		if (current->getParent() != 0)
		{
			CallExpressionBase& call = current->getParent()->cast_to<CallExpressionBase>();
			call.replaceArgument(*current, destinationExpr);
		}
		else
		{
			current = destinationExpr;
		}
		return true;
	}
	else
	{
		if (current->is_a<CallExpressionBase>())
		{
			CallExpressionBase& call = current->cast_to<CallExpressionBase>();
			bool result = false;
			for (int index = 0; index < call.getArgumentCount(); ++index)
			{
				ReprisePtr<ExpressionBase> base(&call.getArgument(index));
				result |= subsHelper(base, note, destinationExpr);
			}
			return result;
		}
		return false;
	}
}

bool isExpressionsEqual(const ExpressionBase& expr1, const ExpressionBase& expr2)
{
	return expr1.isEqual(expr2);
}


bool hasSideEffects(ExpressionBase& expr)
{
	if (expr.is_a<ReferenceExpression>())
	{
		ReferenceExpression& refExpr = expr.cast_to<ReferenceExpression>();

		if (checkTypeVolatile(refExpr.getReference().getType()))
			return true;
		return false;
	}
	else if (expr.is_a<CallExpressionBase>())
	{
		CallExpressionBase& callBase = expr.cast_to<CallExpressionBase>();
		for (int index = 0; index < callBase.getArgumentCount(); ++index)
		{
			if (hasSideEffects(callBase.getArgument(index)))
			{
				return true;
			}
		}
		if (expr.is_a<BasicCallExpression>())
		{
			BasicCallExpression& call = expr.cast_to<BasicCallExpression>();
			if (call.getKind() == BasicCallExpression::BCK_ASSIGN)
			{
				return true;
			}
			return false;
		}
		else if (expr.is_a<Canto::HirCCallExpression>())
		{
			using namespace Canto;
			HirCCallExpression& call = expr.cast_to<HirCCallExpression>();
			switch (call.getKind())
			{
			case HirCCallExpression::HIRC_PREFIX_PLUS_PLUS:			// ++()
			case HirCCallExpression::HIRC_POSTFIX_PLUS_PLUS:		// ()++
			case HirCCallExpression::HIRC_PREFIX_MINUS_MINUS:		// --()
			case HirCCallExpression::HIRC_POSTFIX_MINUS_MINUS:		// ()--

			case HirCCallExpression::HIRC_ASSIGN:					// =
			case HirCCallExpression::HIRC_PLUS_ASSIGN:				// +=
			case HirCCallExpression::HIRC_MINUS_ASSIGN:				// -=
			case HirCCallExpression::HIRC_MULTIPLY_ASSIGN:			// *=
			case HirCCallExpression::HIRC_DIVISION_ASSIGN:			// /=
			case HirCCallExpression::HIRC_MOD_ASSIGN:				// %=
			case HirCCallExpression::HIRC_LSHIFT_ASSIGN:			// <<=
			case HirCCallExpression::HIRC_RSHIFT_ASSIGN:			// >>=
			case HirCCallExpression::HIRC_BAND_ASSIGN:				// &=
			case HirCCallExpression::HIRC_BOR_ASSIGN:				// |=
			case HirCCallExpression::HIRC_BXOR_ASSIGN:				// ^=
				return true;

			default:
				return false;
			}
		}
		else if (expr.is_a<SubroutineCallExpression>())
		{
			return true;
		}
		return false;
	}
	else if (expr.is_a<StructAccessExpression>())
	{
		StructAccessExpression& accessExpr = expr.cast_to<StructAccessExpression>();
		if (checkTypeVolatile(accessExpr.getMember().getType()))
			return true;
		return hasSideEffects(accessExpr.getStructPointerExpression());
	}
	else if (expr.is_a<LiteralExpression>())
	{
		return false;
	}
	else if (expr.is_a<EnumAccessExpression>())
	{
		EnumAccessExpression& accessExpr = expr.cast_to<EnumAccessExpression>();
		if (accessExpr.getMember().getEnum().isVolatile())
			return true;
		return false;
	}
	else if (expr.is_a<SubroutineReferenceExpression>())
	{
		return false;
	}
	else if (expr.is_a<TypeCastExpression>())
	{
		TypeCastExpression& castExpr = expr.cast_to<TypeCastExpression>();
		if (checkTypeVolatile(castExpr.getCastType()))
			return true;
		return hasSideEffects(castExpr.getCastArgument());
	}
	else if (expr.is_a<EmptyExpression>())
	{
		return false;
	}
	throw RepriseError("Should not be here. Unexpected expression.");
}

ReprisePtr<TypeBase> getExpressionType(const ExpressionBase& expression)
{
	ExpressionTypeWalker walker(ExpressionTypeWalker::WK_NORMAL);
	const_cast<ExpressionBase&>(expression).accept(walker);
	return walker.getResult();
}

ReprisePtr<TypeBase> getExpressionPreciseType(const ExpressionBase& expression)
{
	ExpressionTypeWalker walker(ExpressionTypeWalker::WK_PRECISE);
	const_cast<ExpressionBase&>(expression).accept(walker);
	return walker.getResult();
}

TypeBase& desugarType(TypeBase& type)
{
    TypeBase* currentType = &type;
    while (currentType->is_a<DeclaredType>() || currentType->is_a<TypedefType>())
    {
        DeclaredType* dt = currentType->cast_ptr<DeclaredType>();
        if (dt != 0)
        {
            TypeDeclaration& tdecl = dt->getDeclaration();
            currentType = &tdecl.getType();
        }
        TypedefType* tt = currentType->cast_ptr<TypedefType>();
        if (tt != 0)
        {
            currentType = &tt->getBaseType();
        }
    }
    return *currentType;
}

const TypeBase& desugarType(const TypeBase &type)
{
    return desugarType(const_cast<TypeBase&>(type));
}


ReprisePtr<TypeBase> replaceType(TypeBase &sourceType, ReprisePtr<TypeBase> destinationType)
{
	ReprisePtr<TypeBase> replacedType(&sourceType);
	RepriseBase* parent = sourceType.getParent();

	if (parent == 0)
	{
		replacedType = destinationType;
	}
	else if (StructMemberDescriptor* member = parent->cast_ptr<StructMemberDescriptor>())
	{
		member->setType(destinationType.get());
	}
	else if (ParameterDescriptor* param = parent->cast_ptr<ParameterDescriptor>())
	{
		param->setType(destinationType.get());
	}
	else if (SubroutineType* subroutine = parent->cast_ptr<SubroutineType>())
	{
		subroutine->setReturnType(destinationType.get());
	}
	else if (PtrType* ptr = parent->cast_ptr<PtrType>())
	{
		ptr->setPointedType(destinationType.get());
	}
	else if (ArrayType* array = parent->cast_ptr<ArrayType>())
	{
		array->setBaseType(destinationType.get());
	}
	else if (VariableDeclaration* var = parent->cast_ptr<VariableDeclaration>())
	{
		var->setType(destinationType.get());
	}
	else if (TypedefType* typed = parent->cast_ptr<TypedefType>())
	{
		typed->setBaseType(destinationType.get());
	}
	else if (TypeCastExpression* typecast = parent->cast_ptr<TypeCastExpression>())
	{
		typecast->setCastType(destinationType.get());
	}
	else if (VectorType* vector = parent->cast_ptr<VectorType>())
	{
		vector->setBaseType(destinationType.get());
	}
	else if (TypeDeclaration* typedecl = parent->cast_ptr<TypeDeclaration>())
	{
		typedecl->setType(destinationType.get());
	}
	else
	{
		throw RepriseError("Unexpected parent for Canto::HirCBasicType");
	}
	replacedType->setParent(0);
	return replacedType;
}

class GotoStatementsWalker : public Service::DeepWalker
{
public:
	typedef std::list<GotoStatement*> GotoStatementList;

	GotoStatementsWalker()
	{
	}

	void reset()
	{
		m_gotoStatements.clear();
	}

	virtual void visit(GotoStatement& gotoStmt)
	{
		m_gotoStatements.push_back(&gotoStmt);
	}

	GotoStatementList m_gotoStatements;
};

std::list<GotoStatement*> findAllGotos(BlockStatement& rootBlock)
{
	GotoStatementsWalker walker;
	rootBlock.accept(walker);
	return walker.m_gotoStatements;
}

ReprisePtr<StatementBase> replaceStatement(StatementBase& sourceStmt, ReprisePtr<StatementBase> destinationStmt)
{
	ReprisePtr<StatementBase> replacedStmt(&sourceStmt);
	if (&sourceStmt.getRootBlock() == &sourceStmt)
	{
		if (sourceStmt.getParent()->is_a<SubroutineDeclaration>())
		{
			SubroutineDeclaration& subrDecl = sourceStmt.getParent()->cast_to<SubroutineDeclaration>();
			if (destinationStmt->is_a<BlockStatement>())
			{
				BlockStatement* dest = &destinationStmt.release()->cast_to<BlockStatement>();
				ReprisePtr<BlockStatement> destBlock(dest);
				subrDecl.setBodyBlock(destBlock);
			}
			else
				throw RepriseError("Destination statement must be a BlockStatement.");
		}
		else
			throw RepriseError("Unexpected statement parent.");
	}
	else
	{
		if (sourceStmt.hasLabel())
		{
			destinationStmt->setLabel(sourceStmt.getLabel());
			GotoStatementsWalker::GotoStatementList gotos = findAllGotos(sourceStmt.getRootBlock());
			for (GotoStatementsWalker::GotoStatementList::iterator it = gotos.begin(); it != gotos.end(); ++it)
			{
				if ((*it)->getPointedStatement() == &sourceStmt)
					(*it)->setPointedStatement(destinationStmt.get());
			}
			sourceStmt.setLabel("");
		}
		{
			// Search for case labels on statement
			RepriseBase* parent = sourceStmt.getParent();
			while (parent != 0 && parent->is_a<StatementBase>())
			{
				if (PlainSwitchStatement* switchStmt = parent->cast_ptr<PlainSwitchStatement>())
				{
					int labelCount = switchStmt->getLabelCount();
					for (int label = 0; label < labelCount; ++label)
					{
						if (&switchStmt->getLabel(label).getStatement() == &sourceStmt)
						{
							switchStmt->getLabel(label).setStatement(destinationStmt.get());
						}
					}
				}
				parent = parent->getParent();
			}
		}

		if (BlockStatement* parentBlock = sourceStmt.getParent()->cast_ptr<BlockStatement>())
		{
			BlockStatement::Iterator replacedStmtIter = parentBlock->convertToIterator(&sourceStmt);
			parentBlock->replace(replacedStmtIter, destinationStmt.release());
		}
		else if (ForStatement* parentFor = sourceStmt.getParent()->cast_ptr<ForStatement>())
		{
			parentFor->setBody(&destinationStmt->cast_to<BlockStatement>());
		}
		else
		{
			throw OPS::RuntimeError("Unexpected kind of parent statement in replaceStatement()");
		}
	}

	replacedStmt->setParent(0);
	return replacedStmt;
}

ReprisePtr<StatementBase> replaceProgramFragment(ProgramFragment& sourceFragment, ReprisePtr<StatementBase> destinationStmt)
{
    if (sourceFragment.isEmpty())
        throw OPS::RuntimeError("Empty fragment has been passed to replaceProgramFragment");

    if (sourceFragment.getFirst().hasLabel())
    {
        destinationStmt->setLabel(sourceFragment.getFirst().getLabel());
        GotoStatementsWalker::GotoStatementList gotos = findAllGotos(sourceFragment.getFirst().getRootBlock());
        for (GotoStatementsWalker::GotoStatementList::iterator it = gotos.begin(); it != gotos.end(); ++it)
        {
            if ((*it)->getPointedStatement() == &sourceFragment.getFirst())
                (*it)->setPointedStatement(destinationStmt.get());
        }
        sourceFragment.getFirst().setLabel("");
    }

    BlockStatement& fragmentBlock = sourceFragment.getStatementsBlock();

    fragmentBlock.addAfter(sourceFragment.getLastIterator(), destinationStmt.release());

    if (sourceFragment.isSingleStatement())
    {
        ReprisePtr<StatementBase> sourceStatement(&sourceFragment.getFirst());
        fragmentBlock.erase(&sourceFragment.getFirst());
        sourceStatement->setParent(0);
        return sourceStatement;
    }
    else
    {
        ReprisePtr<BlockStatement> blockStmt(new BlockStatement);
        BlockStatement::Iterator endIter = sourceFragment.getAfterLastIterator();
        BlockStatement::Iterator itStmt = sourceFragment.getFirstIterator();
        for(; itStmt != endIter;)
        {
            ReprisePtr<StatementBase> stmt(&*itStmt);
            BlockStatement::Iterator tmpIt = itStmt++;
            fragmentBlock.erase(tmpIt);
            blockStmt->addLast(stmt.get());
        }
        return blockStmt;
    }
}

//	Static functions
bool checkTypeVolatile(TypeBase& type)
{
	if (type.is_a<Canto::HirCBasicType>())
	{
		return type.isVolatile();
	}
	else if (type.is_a<BasicType>())
	{
		return type.isVolatile();
	}
	else if (type.is_a<PtrType>())
	{
		return type.isVolatile() || checkTypeVolatile(type.cast_to<PtrType>().getPointedType());
	}
	else if (type.is_a<TypedefType>())
	{
		return type.isVolatile() || checkTypeVolatile(type.cast_to<TypedefType>().getBaseType());
	}
	else if (type.is_a<ArrayType>())
	{
		return type.isVolatile() || checkTypeVolatile(type.cast_to<ArrayType>().getBaseType());
	}
	else if (type.is_a<VectorType>())
	{
		return type.isVolatile() || checkTypeVolatile(type.cast_to<VectorType>().getBaseType());
	}
	else if (type.is_a<StructType>())
	{
		return type.isVolatile();
	}
	else if (type.is_a<EnumType>())
	{
		return type.isVolatile();
	}
	else if (type.is_a<SubroutineType>())
	{
		return type.isVolatile();
	}
	else if (type.is_a<DeclaredType>())
	{
		if (type.isVolatile())
			return true;
		DeclaredType& declType = type.cast_to<DeclaredType>();
		return checkTypeVolatile(declType.getDeclaration().getType());
	}
	else
		throw RepriseError("Unknown type node.");
}


bool forIsBasic(const ForStatement& forStmt)
{
	// for (i = ...; ...; ...)
	const ExpressionBase& initExpr = forStmt.getInitExpression();
	if (!initExpr.is_a<BasicCallExpression>())
	{
		return false;
	}
	const BasicCallExpression& initAssignExpr = initExpr.cast_to<BasicCallExpression>();
	if (initAssignExpr.getKind() != BasicCallExpression::BCK_ASSIGN || !initAssignExpr.getArgument(0).is_a<ReferenceExpression>())
	{
		return false;
	}
	const ReferenceExpression& initCounter = initAssignExpr.getArgument(0).cast_to<ReferenceExpression>();

	// for (...; i < ...; ...)
	const ExpressionBase& finalExpr = forStmt.getFinalExpression();
	if (!finalExpr.is_a<BasicCallExpression>())
	{
		return false;
	}
	const BasicCallExpression& finalLessExpr = finalExpr.cast_to<BasicCallExpression>();
	if (finalLessExpr.getKind() != BasicCallExpression::BCK_LESS || !finalLessExpr.getArgument(0).is_a<ReferenceExpression>())
	{
		return false;
	}
	const ReferenceExpression& finalCounter = finalLessExpr.getArgument(0).cast_to<ReferenceExpression>();

	// for (...; ...; i = i + ...)
	const ExpressionBase& stepExpr = forStmt.getStepExpression();
	if (!stepExpr.is_a<BasicCallExpression>())
	{
		return false;
	}
	const BasicCallExpression& stepAssignExpr = stepExpr.cast_to<BasicCallExpression>();
	if (stepAssignExpr.getKind() != BasicCallExpression::BCK_ASSIGN || !stepAssignExpr.getArgument(0).is_a<ReferenceExpression>() || !stepAssignExpr.getArgument(1).is_a<BasicCallExpression>())
	{
		return false;
	}
	const ReferenceExpression& stepLeftCounter = stepAssignExpr.getArgument(0).cast_to<ReferenceExpression>();
	const BasicCallExpression& stepPlusExpr = stepAssignExpr.getArgument(1).cast_to<BasicCallExpression>();
	if (stepPlusExpr.getKind() != BasicCallExpression::BCK_BINARY_PLUS || !stepPlusExpr.getArgument(0).is_a<ReferenceExpression>())
	{
		return false;
	}
	const ReferenceExpression& stepRightCounter = stepPlusExpr.getArgument(0).cast_to<ReferenceExpression>();

	// compare all counters
	if (!initCounter.isEqual(finalCounter) || !stepLeftCounter.isEqual(stepRightCounter) || !initCounter.isEqual(stepLeftCounter))
	{
		return false;
	}

	return true;
}

bool tryToGetSignedValue(const StrictLiteralExpression& literal, sqword* val)
{
	switch (literal.getLiteralType())
	{
	case BasicType::BT_INT8:
		*val = literal.getInt8();
		break;
	case BasicType::BT_INT16:
		*val = literal.getInt16();
		break;
	case BasicType::BT_INT32:
		*val = literal.getInt32();
		break;
	case BasicType::BT_INT64:
		*val = literal.getInt64();
		break;
	default:
		return false;
	}
	return true;
}

bool tryToGetUnsignedValue(const StrictLiteralExpression& literal, qword* val)
{
	switch (literal.getLiteralType())
	{
	case BasicType::BT_UINT8:
		*val = literal.getUInt8();
		break;
	case BasicType::BT_UINT16:
		*val = literal.getUInt16();
		break;
	case BasicType::BT_UINT32:
		*val = literal.getUInt32();
		break;
	case BasicType::BT_UINT64:
		*val = literal.getUInt64();
		break;
	default:
		return false;
	}
	return true;
}

template <typename T>
bool compare(const StrictLiteralExpression& literal, T val)
{
	sqword literalValue;
	qword literalUValue;
	if (tryToGetSignedValue(literal, &literalValue))
	{
		if (literalValue != val)
		{
			return false;
		}
	}
	else if (tryToGetUnsignedValue(literal, &literalUValue))
	{
		if (literalUValue != val)
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}

bool forHeaderIsCanonized(ForStatement& forStmt)
{
	if (!forIsBasic(forStmt))
	{
		return false;
	}

	// for (i = 0; ...; ...)
	const ExpressionBase& initExpr = getBasicForInitExpression(forStmt);
	if (!initExpr.is_a<StrictLiteralExpression>())
	{
		return false;
	}
	const StrictLiteralExpression& initLiteral = initExpr.cast_to<StrictLiteralExpression>();
	if (!compare<int>(initLiteral, 0))
	{
		return false;
	}

	// for (...; ...; i = i + 1)
	const ExpressionBase& stepExpr = getBasicForStep(forStmt);
	if (!stepExpr.is_a<StrictLiteralExpression>())
	{
		return false;
	}
	const StrictLiteralExpression& stepLiteral = stepExpr.cast_to<StrictLiteralExpression>();
	if (!compare<int>(stepLiteral, 1))
	{
		return false;
	}

	return true;
}

ReferenceExpression& getBasicForCounter(ForStatement& forStmt)
{
	OPS_ASSERT(forIsBasic(forStmt));
	BasicCallExpression& assignExpr = forStmt.getStepExpression().cast_to<BasicCallExpression>();

	return assignExpr.getArgument(0).cast_to<ReferenceExpression>();
}

ExpressionBase& getBasicForInitExpression(ForStatement& forStmt)
{
	OPS_ASSERT(forIsBasic(forStmt));
	BasicCallExpression& assignExpr = forStmt.getInitExpression().cast_to<BasicCallExpression>();

	return assignExpr.getArgument(1);
}

ExpressionBase& getBasicForFinalExpression(ForStatement& forStmt)
{
	OPS_ASSERT(forIsBasic(forStmt));
	BasicCallExpression& lessExpr = forStmt.getFinalExpression().cast_to<BasicCallExpression>();

	return lessExpr.getArgument(1);
}

ExpressionBase& getBasicForStep(ForStatement& forStmt)
{
	OPS_ASSERT(forIsBasic(forStmt));
	BasicCallExpression& assignExpr = forStmt.getStepExpression().cast_to<BasicCallExpression>();
	BasicCallExpression& plusExpr = assignExpr.getArgument(1).cast_to<BasicCallExpression>();

	return plusExpr.getArgument(1);
}

bool checkParentChildRelations(RepriseBase& node, bool shouldThrow)
{
	int childCount = node.getChildCount();

	for(int i = 0; i < childCount; ++i)
	{
		if (node.getChild(i).getParent() != &node)
		{
			if (shouldThrow)
				throw OPS::RuntimeError(OPS::Strings::format("Node ncid=%i does not own its %ith child ncid=%i", node.getNCID(), i, node.getChild(i).getNCID()));
			else
				return false;
		}

		if (!checkParentChildRelations(node.getChild(i), shouldThrow))
			return false;
	}

	return true;
}

void moveLabels(StatementBase &fromStmt, StatementBase &toStmt)
{
    if (fromStmt.hasLabel())
    {
        toStmt.setLabel(fromStmt.getLabel());
        fromStmt.setLabel("");
    }

    StatementBase* parentStmt = &fromStmt;
	while(nullptr != (parentStmt = parentStmt->getParent()->cast_ptr<StatementBase>()))
    {
        if (PlainSwitchStatement* switchParent = parentStmt->cast_ptr<PlainSwitchStatement>())
        {
            int caseCount = switchParent->getLabelCount();
            for(int i = 0; i < caseCount; ++i)
            {
                if (&switchParent->getLabel(i).getStatement() == &fromStmt)
                {
                    switchParent->getLabel(i).setStatement(&toStmt);
                }
            }
        }
    }
}

//  Exit namespace
}
}
}
