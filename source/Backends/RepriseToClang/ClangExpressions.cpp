/// ClangExpressions.cpp
///   Create clang expressions from Reprise expressions.
/// Author: Denis Dubrov (dubrov@sfedu.ru)
/// Created: 12.04.2013

#include "ClangExpressions.h"
#include "ClangTypes.h"

#include "OPS_Core/Helpers.h"
#include "Reprise/Service/DeepWalker.h"
#include "Reprise/Declarations.h"
#include "Reprise/Expressions.h"

#include "OPS_Core/disable_llvm_warnings_begin.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/Basic/TargetInfo.h"
#include "OPS_Core/disable_llvm_warnings_end.h"

#include <string>
#include <utility>

using namespace OPS::Backends::Clang::Internal;
using namespace OPS::Reprise;

using namespace std;

#define OPS_CONVERT_OPCODE(nOpCode, repriseOp, clangOp) \
	case BasicCallExpression::repriseOp: \
	{ \
		nOpCode = clang::clangOp; \
		\
		break; \
	}

//////////////////////////////////////////////////////////////////////
// Internal

namespace
{
	class ExprWalker : public OPS::Reprise::Service::DeepWalker
	{
	public:
		//
		ExprWalker(
			clang::ASTContext &rASTContext,
			R2CDeclNodes& rR2CDeclNodes);
		//
	// Attributes
	public:
		//
		clang::Expr* getResultExpr() const;
		//
	// Overrides
	public:
		//
		virtual void visit(
			StrictLiteralExpression& rStrictLiteralExpression);
		//
		virtual void visit(
			ReferenceExpression& rReferenceExpression);
		//
		virtual void visit(
			SubroutineReferenceExpression& SubroutineReferenceExpression);
		//
		virtual void visit(
			BasicCallExpression& rBasicCallExpression);
		//
		virtual void visit(
			SubroutineCallExpression& rSubroutineCallExpression);
		//
		virtual void visit(
			TypeCastExpression& rTypeCastExpression);
		//
	private:
		//
		clang::ASTContext &m_rASTContext;
		R2CDeclNodes& m_rR2CDeclNodes;
		const clang::TargetInfo& m_rcTargetInfo;
		clang::Expr* m_pResultExpr;
	};
	
	typedef pair <clang::Expr*, clang::Expr*> ExprTuple;

	namespace Private
	{
		clang::Expr* getUnaryExprOperand(
			clang::ASTContext &rASTContext,
			R2CDeclNodes& rR2CDeclNodes,
			BasicCallExpression& rBasicCallExpression);

		ExprTuple getBinaryExprOperand(
			clang::ASTContext &rASTContext,
			R2CDeclNodes& rR2CDeclNodes,
			BasicCallExpression& rBasicCallExpression);

		clang::CastKind getCastKind(
			TypeBase* pTypeArg,
			TypeBase* pTypeCastTo);

		clang::Expr* usualUnaryConversions(
			clang::ASTContext& context,
			clang::Expr* E);

		ExprTuple usualArithmeticConversions(clang::ASTContext& context,
			clang::Expr* LHS,
			clang::Expr* RHS,
			bool isCompAssign);
	}
}    // namespace

using namespace Private;

ExprWalker::ExprWalker(
	clang::ASTContext &rASTContext,
	R2CDeclNodes& rR2CDeclNodes)
	: m_rASTContext(rASTContext),
		m_rR2CDeclNodes(rR2CDeclNodes),
		m_rcTargetInfo(rASTContext.getTargetInfo()),
		m_pResultExpr(0)
{
	//
}

clang::Expr* ExprWalker::getResultExpr() const
{
	assert(m_pResultExpr != 0);

	return m_pResultExpr;
}

void ExprWalker::visit(
	StrictLiteralExpression& rStrictLiteralExpression)
{
	const clang::TargetInfo& rcTargetInfo =
		m_rASTContext.getTargetInfo();

	switch (rStrictLiteralExpression.getLiteralType())
	{
		case BasicType::BT_CHAR:
		{
			m_pResultExpr = new (m_rASTContext) clang::CharacterLiteral(
				rStrictLiteralExpression.getChar(),
				clang::CharacterLiteral::Ascii,
				m_rASTContext.IntTy,
				clang::SourceLocation());

			break;
		}
		case BasicType::BT_WIDE_CHAR:
		{
			m_pResultExpr = new (m_rASTContext) clang::CharacterLiteral(
				rStrictLiteralExpression.getWideChar(),
				clang::CharacterLiteral::Wide,
				m_rASTContext.IntTy,
				clang::SourceLocation());

			break;
		}
		case BasicType::BT_INT8:
		{
			const llvm::APInt cLiteralValue(
				rcTargetInfo.getCharWidth(),
				rStrictLiteralExpression.getInt8(),
				true);

			m_pResultExpr = clang::IntegerLiteral::Create(
				m_rASTContext, cLiteralValue,
				m_rASTContext.SignedCharTy,
				clang::SourceLocation());

			break;
		}
		case BasicType::BT_INT16:
		{
			const llvm::APInt cLiteralValue(
				rcTargetInfo.getShortWidth(),
				rStrictLiteralExpression.getInt16(),
				true);

			m_pResultExpr = clang::IntegerLiteral::Create(
				m_rASTContext, cLiteralValue,
				m_rASTContext.ShortTy,
				clang::SourceLocation());

			break;
		}
		case BasicType::BT_INT32:
		{
			const llvm::APInt cLiteralValue(
				rcTargetInfo.getIntWidth(),
				rStrictLiteralExpression.getInt32(),
				true);

			m_pResultExpr = clang::IntegerLiteral::Create(
				m_rASTContext, cLiteralValue,
				m_rASTContext.IntTy,
				clang::SourceLocation());

			break;
		}
		case BasicType::BT_INT64:
		{
			const llvm::APInt cLiteralValue(
				rcTargetInfo.getLongWidth(),
				rStrictLiteralExpression.getInt64(),
				true);

			m_pResultExpr = clang::IntegerLiteral::Create(
				m_rASTContext, cLiteralValue,
				m_rASTContext.LongTy,
				clang::SourceLocation());

			break;
		}
		case BasicType::BT_INT128:
		{
			const llvm::APInt cLiteralValue(
				128,
				rStrictLiteralExpression.getInt64(),   // ???
				true);

			m_pResultExpr = clang::IntegerLiteral::Create(
				m_rASTContext, cLiteralValue,
				m_rASTContext.LongLongTy,
				clang::SourceLocation());

			break;
		}
		case BasicType::BT_UINT8:
		{
			const llvm::APInt cLiteralValue(
				rcTargetInfo.getCharWidth(),
				rStrictLiteralExpression.getUInt8(),
				false);

			m_pResultExpr = clang::IntegerLiteral::Create(
				m_rASTContext, cLiteralValue,
				m_rASTContext.UnsignedCharTy,
				clang::SourceLocation());

			break;
		}
		case BasicType::BT_UINT16:
		{
			const llvm::APInt cLiteralValue(
				rcTargetInfo.getShortWidth(),
				rStrictLiteralExpression.getUInt16(),
				false);

			m_pResultExpr = clang::IntegerLiteral::Create(
				m_rASTContext, cLiteralValue,
				m_rASTContext.UnsignedShortTy,
				clang::SourceLocation());

			break;
		}
		case BasicType::BT_UINT32:
		{
			const llvm::APInt cLiteralValue(
				rcTargetInfo.getIntWidth(),
				static_cast <uint64_t> (
					rStrictLiteralExpression.getUInt32()),
				false);

			m_pResultExpr = clang::IntegerLiteral::Create(
				m_rASTContext, cLiteralValue,
				m_rASTContext.UnsignedIntTy,
				clang::SourceLocation());

			break;
		}
		case BasicType::BT_UINT64:
		{
			const llvm::APInt cLiteralValue(
				rcTargetInfo.getLongWidth(),
				rStrictLiteralExpression.getUInt64(),
				false);

			m_pResultExpr = clang::IntegerLiteral::Create(
				m_rASTContext, cLiteralValue,
				m_rASTContext.UnsignedLongTy,
				clang::SourceLocation());

			break;
		}
		case BasicType::BT_UINT128:
		{
			const llvm::APInt cLiteralValue(
				128,
				rStrictLiteralExpression.getUInt64(),   // ???
				false);

			m_pResultExpr = clang::IntegerLiteral::Create(
				m_rASTContext, cLiteralValue,
				m_rASTContext.UnsignedLongLongTy,
				clang::SourceLocation());

			break;
		}
		case BasicType::BT_FLOAT32:
		{
			const llvm::APFloat cLiteralValue(
				rStrictLiteralExpression.getFloat32());

			m_pResultExpr = clang::FloatingLiteral::Create(
				m_rASTContext, cLiteralValue, true,
				m_rASTContext.FloatTy,
				clang::SourceLocation());

			break;
		}
		case BasicType::BT_FLOAT64:
		{
			const llvm::APFloat cLiteralValue(
				rStrictLiteralExpression.getFloat64());

			m_pResultExpr = clang::FloatingLiteral::Create(
				m_rASTContext, cLiteralValue, true,
				m_rASTContext.DoubleTy,
				clang::SourceLocation());

			break;
		}
		case BasicType::BT_STRING:
		{
			const string& rcStrLiteral =
				rStrictLiteralExpression.getString();

			const llvm::APInt cLiteralSize(
				rcTargetInfo.getTypeWidth(rcTargetInfo.getSizeType()),
				static_cast <uint64_t> (rcStrLiteral.size() + 1),
				false);

			const clang::QualType cLiteralType =
				m_rASTContext.getConstantArrayType(
					m_rASTContext.CharTy, cLiteralSize,
					clang::ArrayType::Normal, 0);

			m_pResultExpr = clang::StringLiteral::Create(
				m_rASTContext,
				rcStrLiteral,
				clang::StringLiteral::Ascii,
				false,
				cLiteralType,
				clang::SourceLocation());

			break;
		}
		case BasicType::BT_WIDE_STRING:
		{
			const wstring& rcStrLiteral =
				rStrictLiteralExpression.getWideString();
			const unsigned cuWCharBytes =
				rcTargetInfo.getWCharWidth() /
				rcTargetInfo.getCharWidth();
			const llvm::StringRef cStrLiteral(
				reinterpret_cast <const char*> (rcStrLiteral.data()),
				static_cast <uint64_t> (rcStrLiteral.size() * cuWCharBytes));

			const llvm::APInt cLiteralSize(
				rcTargetInfo.getTypeWidth(rcTargetInfo.getSizeType()),
				static_cast <uint64_t> (rcStrLiteral.size() + 1),
				false);

			const clang::QualType cLiteralType =
				m_rASTContext.getConstantArrayType(
					m_rASTContext.WCharTy, cLiteralSize,
					clang::ArrayType::Normal, 0);

			m_pResultExpr = clang::StringLiteral::Create(
				m_rASTContext,
				cStrLiteral,
				clang::StringLiteral::Wide,
				false,
				cLiteralType,
				clang::SourceLocation());

			break;
		}
		//
		OPS_DEFAULT_CASE_LABEL;
		//
	}    // switch (rStrictLiteralExpression.getLiteralType())
}

void ExprWalker::visit(
	ReferenceExpression& rReferenceExpression)
{
	const VariableDeclaration* pcVariableDeclaration =
		&rReferenceExpression.getReference();
	R2CDeclNodes::iterator i =
		m_rR2CDeclNodes.find(pcVariableDeclaration);
	clang::Decl* pDecl = 0;
	if (i != m_rR2CDeclNodes.end())
		pDecl = i->second;
	else
		assert(
			false &&
			"No Reprise typedef declaration found for the clang typedef decl");

	if (clang::ValueDecl* pValueDecl =
		llvm::dyn_cast_or_null <clang::ValueDecl> (pDecl))
	{
		m_pResultExpr = clang::DeclRefExpr::Create(
			m_rASTContext,
			clang::NestedNameSpecifierLoc(),
			clang::SourceLocation(),
			pValueDecl,
			false,
			clang::DeclarationNameInfo(),
			pValueDecl->getType(),
			clang::VK_LValue);
	}
}

void ExprWalker::visit(
	SubroutineReferenceExpression& SubroutineReferenceExpression)
{
	SubroutineDeclaration& rSubroutineDeclaration =
		SubroutineReferenceExpression.getReference();
	R2CDeclNodes::iterator i =
		m_rR2CDeclNodes.find(&rSubroutineDeclaration);
	clang::Decl* pDecl = 0;
	if (i != m_rR2CDeclNodes.end())
		pDecl = i->second;
	else
		assert(
			false &&
			"No Reprise subroutine declaration found for the clang function decl");

	clang::FunctionDecl* pFunctionDecl =
		llvm::dyn_cast <clang::FunctionDecl> (pDecl);
	OPS_ASSERT(pFunctionDecl != 0);

	m_pResultExpr = clang::DeclRefExpr::Create(
		m_rASTContext,
		clang::NestedNameSpecifierLoc(),
		clang::SourceLocation(),
		pFunctionDecl,
		false,
		clang::DeclarationNameInfo(),
		pFunctionDecl->getType(),
		clang::VK_RValue);
}

void ExprWalker::visit(
	BasicCallExpression& rBasicCallExpression)
{
	ReprisePtr <TypeBase> ptrTypeResult(
		rBasicCallExpression.getResultType());

	clang::QualType resultType = getClangQualType(
		m_rASTContext, *ptrTypeResult, m_rR2CDeclNodes);

	const int cnArgNum =
		rBasicCallExpression.getArgumentCount();
	const BasicCallExpression::BuiltinCallKind cnCallKind =
		rBasicCallExpression.getKind();

	switch (cnCallKind)
	{
		case BasicCallExpression::BCK_ARRAY_ACCESS:
		{
			assert(cnArgNum >= 2);

			ExpressionBase& rExprLeft =
				rBasicCallExpression.getArgument(0);
			m_pResultExpr = getClangExpr(
				m_rASTContext, rExprLeft, m_rR2CDeclNodes);

			for (int i = 1; i < cnArgNum; ++ i)
			{
				ExpressionBase& rExprIthIdx =
					rBasicCallExpression.getArgument(i);
				clang::Expr* pExprIthIdx = getClangExpr(
					m_rASTContext, rExprIthIdx, m_rR2CDeclNodes);

				m_pResultExpr = new (m_rASTContext) clang::ArraySubscriptExpr(
					m_pResultExpr,
					pExprIthIdx,
					resultType,
					clang::VK_LValue,
					clang::OK_Ordinary,
					clang::SourceLocation());
			}
			
			break;
		}
		default:
		{
			switch (cnArgNum)
			{
				case 1:
				{
					clang::UnaryOperator::Opcode nOpCode;
					clang::ExprValueKind nExprValueKind = clang::VK_RValue;
					ExpressionBase& rExpr =
						rBasicCallExpression.getArgument(0);

					switch (cnCallKind)
					{
						OPS_CONVERT_OPCODE(nOpCode, BCK_UNARY_PLUS, UO_Plus)
						OPS_CONVERT_OPCODE(nOpCode, BCK_UNARY_MINUS, UO_Minus)
						OPS_CONVERT_OPCODE(nOpCode, BCK_TAKE_ADDRESS, UO_AddrOf);
						case BasicCallExpression::BCK_DE_REFERENCE:
						{
							nOpCode = clang::UO_Deref;
							nExprValueKind = clang::VK_LValue;	//C99 Standard 6.5.3.2p4
							break;
						}
						OPS_CONVERT_OPCODE(nOpCode, BCK_LOGICAL_NOT, UO_LNot);
						OPS_CONVERT_OPCODE(nOpCode, BCK_BITWISE_NOT, UO_Not);
						case BasicCallExpression::BCK_SIZE_OF:
						{
							clang::Expr* pExpr = getClangExpr(
								m_rASTContext, rExpr, m_rR2CDeclNodes);

							m_pResultExpr = new (m_rASTContext) clang::UnaryExprOrTypeTraitExpr(
								clang::UnaryExprOrTypeTrait::UETT_SizeOf,
								pExpr,
								resultType,
								clang::SourceLocation(),
								clang::SourceLocation());
							return;
						}
						//
						OPS_DEFAULT_CASE_LABEL;
						//
					}    // switch (cnCallKind)

					bool needsLtoRconv = true;
					if (cnCallKind == BasicCallExpression::BCK_TAKE_ADDRESS)	// C99 Standard 6.3.2.1p2
						needsLtoRconv = false;

					clang::Expr* pExprArg = getClangExpr(
						m_rASTContext, rExpr, m_rR2CDeclNodes, needsLtoRconv);

					pExprArg = usualUnaryConversions(m_rASTContext, pExprArg);

					m_pResultExpr = new (m_rASTContext) clang::UnaryOperator(
						pExprArg,
						nOpCode,
						resultType,
						nExprValueKind,
						clang::OK_Ordinary,
						clang::SourceLocation());

					break;
				}
				case 2:
				{
					clang::BinaryOperator::Opcode nOpCode;
					ExpressionBase& rExprLeft =
						rBasicCallExpression.getArgument(0);
					ExpressionBase& rExprRight =
						rBasicCallExpression.getArgument(1);

					switch (cnCallKind)
					{
						OPS_CONVERT_OPCODE(nOpCode, BCK_BINARY_PLUS, BO_Add);
						OPS_CONVERT_OPCODE(nOpCode, BCK_BINARY_MINUS, BO_Sub);
						OPS_CONVERT_OPCODE(nOpCode, BCK_MULTIPLY, BO_Mul);
						OPS_CONVERT_OPCODE(nOpCode, BCK_DIVISION, BO_Div);
						OPS_CONVERT_OPCODE(nOpCode, BCK_INTEGER_DIVISION, BO_Div);
						OPS_CONVERT_OPCODE(nOpCode, BCK_INTEGER_MOD, BO_Rem);
						OPS_CONVERT_OPCODE(nOpCode, BCK_LESS, BO_LT);
						OPS_CONVERT_OPCODE(nOpCode, BCK_GREATER, BO_GT);
						OPS_CONVERT_OPCODE(nOpCode, BCK_LESS_EQUAL, BO_LE);
						OPS_CONVERT_OPCODE(nOpCode, BCK_GREATER_EQUAL, BO_GE);
						OPS_CONVERT_OPCODE(nOpCode, BCK_EQUAL, BO_EQ);
						OPS_CONVERT_OPCODE(nOpCode, BCK_NOT_EQUAL, BO_NE);

						OPS_CONVERT_OPCODE(nOpCode, BCK_ASSIGN, BO_Assign);
						OPS_CONVERT_OPCODE(nOpCode, BCK_LEFT_SHIFT, BO_Shl);
						OPS_CONVERT_OPCODE(nOpCode, BCK_RIGHT_SHIFT, BO_Shr);
						OPS_CONVERT_OPCODE(nOpCode, BCK_LOGICAL_AND, BO_LAnd);
						OPS_CONVERT_OPCODE(nOpCode, BCK_LOGICAL_OR, BO_LOr);
						OPS_CONVERT_OPCODE(nOpCode, BCK_BITWISE_AND, BO_And);
						OPS_CONVERT_OPCODE(nOpCode, BCK_BITWISE_OR, BO_Or);
						OPS_CONVERT_OPCODE(nOpCode, BCK_BITWISE_XOR, BO_Xor);
						OPS_CONVERT_OPCODE(nOpCode, BCK_COMMA, BO_Comma);
						//
						OPS_DEFAULT_CASE_LABEL;
						//
					}    // switch (cnCallKind)

					bool needLtoRconv = true;
					if (cnCallKind == BasicCallExpression::BCK_ASSIGN)	// C99 Standard 6.3.2.1p2
						needLtoRconv = false;

					clang::Expr* pExprLeft = getClangExpr(
						m_rASTContext, rExprLeft, m_rR2CDeclNodes, needLtoRconv);

					clang::Expr* pExprRight = getClangExpr(
						m_rASTContext, rExprRight, m_rR2CDeclNodes, true);

					///////////////////
					clang::QualType LHSType = pExprLeft->getType().getUnqualifiedType();
					clang::QualType RHSType = pExprRight->getType().getUnqualifiedType();

					switch(cnCallKind)
					{
						// C99 Standard 6.5.5p3
						case BasicCallExpression::BCK_MULTIPLY:
						case BasicCallExpression::BCK_DIVISION:
						case BasicCallExpression::BCK_INTEGER_DIVISION:
						case BasicCallExpression::BCK_INTEGER_MOD:
						// C99 Standard 6.5.7p3
						case BasicCallExpression::BCK_LEFT_SHIFT:
						case BasicCallExpression::BCK_RIGHT_SHIFT:
						// C99 Standard 6.5.10p3
						case BasicCallExpression::BCK_BITWISE_AND:
						// C99 Standard 6.5.11p3
						case BasicCallExpression::BCK_BITWISE_XOR:
						// C99 Standard 6.5.12p3
						case BasicCallExpression::BCK_BITWISE_OR:
						{
							ExprTuple args = usualArithmeticConversions(
								m_rASTContext, pExprLeft, pExprRight, false);
							pExprLeft = args.first;
							pExprRight = args.second;
							break;
						}
						case BasicCallExpression::BCK_BINARY_PLUS:
						{
							// C99 Standard 6.5.6p2
							if (LHSType->isPointerType() && RHSType->isIntegerType() ||
								RHSType->isPointerType() && LHSType->isIntegerType())
							{
								// nothing
							}
							else
							{
								ExprTuple args = usualArithmeticConversions(
									m_rASTContext, pExprLeft, pExprRight, false);
								pExprLeft = args.first;
								pExprRight = args.second;
							}
							break;
						}
						case BasicCallExpression::BCK_BINARY_MINUS:
						{
							// C99 Standard 6.5.6p3 bullit 1
							if (LHSType->isArithmeticType() && RHSType->isArithmeticType())
							{
								ExprTuple args = usualArithmeticConversions(
									m_rASTContext, pExprLeft, pExprRight, false);
								pExprLeft = args.first;
								pExprRight = args.second;
							}
							// C99 Standard 6.5.6p3 bullit 2
							if (LHSType->isPointerType() && RHSType->isPointerType())
							{
								// nothing
							}
							// C99 Standard 6.5.6p3 bullit 3
							if (LHSType->isPointerType() && RHSType->isIntegerType())
							{
								pExprRight = usualUnaryConversions(m_rASTContext, pExprRight);
							}
							break;
						}
						// C99 Standard 6.5.8
						case BasicCallExpression::BCK_LESS:
						case BasicCallExpression::BCK_LESS_EQUAL:
						case BasicCallExpression::BCK_GREATER:
						case BasicCallExpression::BCK_GREATER_EQUAL:
						// C99 Standard 6.5.9
						case BasicCallExpression::BCK_EQUAL:
						case BasicCallExpression::BCK_NOT_EQUAL:
						{
							if (LHSType->isArithmeticType() && RHSType->isArithmeticType())
							{
								ExprTuple args = usualArithmeticConversions(
									m_rASTContext, pExprLeft, pExprRight, false);
								pExprLeft = args.first;
								pExprRight = args.second;
							}
							break;
						}
						case BasicCallExpression::BCK_ASSIGN:
						{
							if (LHSType->isArithmeticType() && RHSType->isArithmeticType())
							{
								ExprTuple args = usualArithmeticConversions(
									m_rASTContext, pExprLeft, pExprRight, true);
								pExprLeft = args.first;
								pExprRight = args.second;
							}
							break;
						}
						default:
						{
							break;
						}
					}

					///////////////////

					m_pResultExpr = new (m_rASTContext) clang::BinaryOperator(
						pExprLeft,
						pExprRight,
						nOpCode,
						resultType,
						clang::VK_RValue,
						clang::OK_Ordinary,
						clang::SourceLocation(),
						false);
						
					break;
				}
				//
				OPS_DEFAULT_CASE_LABEL;
				//
			}    // switch (cnArgNum)
		}    // default
	}    // switch (cnCallKind)
}

void ExprWalker::visit(
	SubroutineCallExpression& rSubroutineCallExpression)
{
	ExpressionBase& rExpressionCall =
		rSubroutineCallExpression.getCallExpression();
	clang::Expr* pExpr = getClangExpr(
		m_rASTContext, rExpressionCall, m_rR2CDeclNodes);

	const int cnNumArgs =
		rSubroutineCallExpression.getArgumentCount();
	llvm::SmallVector <clang::Expr*, 30> arguments;
	arguments.reserve(static_cast <unsigned> (cnNumArgs));
	for (int i = 0; i < cnNumArgs; ++ i)
	{
		ExpressionBase& rExpressionArg =
			rSubroutineCallExpression.getArgument(i);
		clang::Expr* pExprArg = getClangExpr(
			m_rASTContext, rExpressionArg, m_rR2CDeclNodes);

		arguments.push_back(pExprArg);
	}

	m_pResultExpr = new (m_rASTContext) clang::CallExpr(
		m_rASTContext,
		pExpr,
		arguments,
		pExpr->getType(),
		clang::VK_RValue,
		clang::SourceLocation());
}

void ExprWalker::visit(
	TypeCastExpression& rTypeCastExpression)
{
	// implicit?
	ExpressionBase& rExprArg = rTypeCastExpression.getCastArgument();
	clang::Expr* pExpr = getClangExpr(
		m_rASTContext, rExprArg, m_rR2CDeclNodes, true);	// ??? true?
	clang::QualType CastType = getClangQualType(
		m_rASTContext, rTypeCastExpression.getCastType(), m_rR2CDeclNodes);
	clang::CastKind castKind = getCastKind(
		rTypeCastExpression.getCastArgument().getResultType().get(), &rTypeCastExpression.getCastType());

	m_pResultExpr = clang::CStyleCastExpr::Create(
		m_rASTContext,
		CastType,
		clang::VK_RValue,
		castKind,
		pExpr, new(m_rASTContext) clang::CXXCastPath(),
		m_rASTContext.getTrivialTypeSourceInfo(CastType),
		clang::SourceLocation(),
		clang::SourceLocation());
}

clang::Expr* Private::getUnaryExprOperand(
	clang::ASTContext &rASTContext,
	R2CDeclNodes& rR2CDeclNodes,
	BasicCallExpression& rBasicCallExpression)
{
	ExpressionBase& rExpr =
		rBasicCallExpression.getArgument(0);

	clang::Expr* pExpr = getClangExpr(
		rASTContext, rExpr, rR2CDeclNodes);

	return pExpr;
}

ExprTuple Private::getBinaryExprOperand(
	clang::ASTContext &rASTContext,
	R2CDeclNodes& rR2CDeclNodes,
	BasicCallExpression& rBasicCallExpression)
{
	ExpressionBase& rExprLeft =
		rBasicCallExpression.getArgument(0);
	ExpressionBase& rExprRight =
		rBasicCallExpression.getArgument(1);

	clang::Expr* pExprLeft = getClangExpr(
		rASTContext, rExprLeft, rR2CDeclNodes);
	clang::Expr* pExprRight = getClangExpr(
		rASTContext, rExprRight, rR2CDeclNodes);

	return ExprTuple(pExprLeft, pExprRight);
}

clang::CastKind Private::getCastKind(
	TypeBase* pTypeArg,
	TypeBase* pTypeCastTo)
{
	if (pTypeArg->is_a <PtrType>())
	{
		if (pTypeCastTo->is_a <PtrType>())
		{
			PtrType* pPtrTypeCastTo = dynamic_cast <PtrType*>(pTypeCastTo);
			PtrType* pPtrTypeArg = dynamic_cast <PtrType*>(pTypeArg);
			if (pPtrTypeCastTo->getPointedType().isEqual(pPtrTypeArg->getPointedType()))
				return clang::CastKind::CK_NoOp;
			else
				return clang::CastKind::CK_BitCast;
		}
		else
		{
			BasicType* pBaseTypeCastTo  = dynamic_cast <BasicType*>(pTypeCastTo);
			OPS_ASSERT(pBaseTypeCastTo && "Only BasicType casts are supported");
			switch (pBaseTypeCastTo->getKind())
			{
			case BasicType::BT_CHAR:
			case BasicType::BT_INT8:
			case BasicType::BT_WIDE_CHAR:
			case BasicType::BT_INT16:
			case BasicType::BT_INT32:
			case BasicType::BT_INT64:
			case BasicType::BT_INT128:
			case BasicType::BT_UINT8:
			case BasicType::BT_UINT16:
			case BasicType::BT_UINT32:
			case BasicType::BT_UINT64:
			case BasicType::BT_UINT128:
				return clang::CastKind::CK_PointerToIntegral;
			default:
				OPS_ASSERT(!"Unexpected BasicType pBaseTypeCastTo");
			}
		}	// if (pTypeCastTo->is_a <PtrType>())
	}
	else	// if (pTypeArg->is_a <PtrType>())
	{
		if (pTypeCastTo->is_a <PtrType>())
		{
			BasicType* pBaseTypeArg  = dynamic_cast <BasicType*>(pTypeArg);
			OPS_ASSERT(pBaseTypeArg && "Only BasicType casts are supported");
			switch (pBaseTypeArg->getKind())
			{
			case BasicType::BT_CHAR:
			case BasicType::BT_INT8:
			case BasicType::BT_WIDE_CHAR:
			case BasicType::BT_INT16:
			case BasicType::BT_INT32:
			case BasicType::BT_INT64:
			case BasicType::BT_INT128:
			case BasicType::BT_UINT8:
			case BasicType::BT_UINT16:
			case BasicType::BT_UINT32:
			case BasicType::BT_UINT64:
			case BasicType::BT_UINT128:
				return clang::CastKind::CK_IntegralToPointer;
			default:
				OPS_ASSERT(!"Unexpected BasicType pBaseTypeCastTo");
			}
		}
		else
		{
			BasicType* pBaseTypeArg = dynamic_cast <BasicType*>(pTypeArg);
			BasicType* pBaseTypeCastTo  = dynamic_cast <BasicType*>(pTypeCastTo);


			OPS_ASSERT((pBaseTypeArg || pBaseTypeCastTo) &&
					   "Only BasicType casts are supported");

			switch(pBaseTypeArg->getKind())
			{
			case BasicType::BT_CHAR:
			case BasicType::BT_INT8:
				switch(pBaseTypeCastTo->getKind())
				{
				case BasicType::BT_CHAR:
				case BasicType::BT_INT8:
					return clang::CastKind::CK_NoOp;

				case BasicType::BT_WIDE_CHAR:
				case BasicType::BT_INT16:
				case BasicType::BT_INT32:
				case BasicType::BT_INT64:
				case BasicType::BT_INT128:
				case BasicType::BT_UINT8:
				case BasicType::BT_UINT16:
				case BasicType::BT_UINT32:
				case BasicType::BT_UINT64:
				case BasicType::BT_UINT128:
					return clang::CastKind::CK_IntegralCast;

				case BasicType::BT_FLOAT32:
				case BasicType::BT_FLOAT64:
					return clang::CastKind::CK_IntegralToFloating;

				case BasicType::BT_BOOLEAN:
					return clang::CastKind::CK_IntegralToBoolean;

				case BasicType::BT_VOID:
					return clang::CastKind::CK_ToVoid;
				default:
					OPS_ASSERT(!"Unexpected BasicType pBaseTypeCastTo");
				};

			case BasicType::BT_WIDE_CHAR:
			case BasicType::BT_INT32:
				switch(pBaseTypeCastTo->getKind())
				{
				case BasicType::BT_WIDE_CHAR:
				case BasicType::BT_INT32:
					return clang::CastKind::CK_NoOp;

				case BasicType::BT_CHAR:
				case BasicType::BT_INT8:
				case BasicType::BT_INT16:
				case BasicType::BT_INT64:
				case BasicType::BT_INT128:
				case BasicType::BT_UINT8:
				case BasicType::BT_UINT16:
				case BasicType::BT_UINT32:
				case BasicType::BT_UINT64:
				case BasicType::BT_UINT128:
					return clang::CastKind::CK_IntegralCast;

				case BasicType::BT_FLOAT32:
				case BasicType::BT_FLOAT64:
					return clang::CastKind::CK_IntegralToFloating;

				case BasicType::BT_BOOLEAN:
					return clang::CastKind::CK_IntegralToBoolean;

				case BasicType::BT_VOID:
					return clang::CastKind::CK_ToVoid;
				default:
					OPS_ASSERT(!"Unexpected BasicType pBaseTypeCastTo");
				};

			case BasicType::BT_INT16:
				switch(pBaseTypeCastTo->getKind())
				{
				case BasicType::BT_INT16:
					return clang::CastKind::CK_NoOp;

				case BasicType::BT_CHAR:
				case BasicType::BT_WIDE_CHAR:
				case BasicType::BT_INT8:
				case BasicType::BT_INT32:
				case BasicType::BT_INT64:
				case BasicType::BT_INT128:
				case BasicType::BT_UINT8:
				case BasicType::BT_UINT16:
				case BasicType::BT_UINT32:
				case BasicType::BT_UINT64:
				case BasicType::BT_UINT128:
					return clang::CastKind::CK_IntegralCast;

				case BasicType::BT_FLOAT32:
				case BasicType::BT_FLOAT64:
					return clang::CastKind::CK_IntegralToFloating;

				case BasicType::BT_BOOLEAN:
					return clang::CastKind::CK_IntegralToBoolean;

				case BasicType::BT_VOID:
					return clang::CastKind::CK_ToVoid;
				default:
					OPS_ASSERT(!"Unexpected BasicType pBaseTypeCastTo");
				};

			case BasicType::BT_INT64:
				switch(pBaseTypeCastTo->getKind())
				{
				case BasicType::BT_INT64:
					return clang::CastKind::CK_NoOp;

				case BasicType::BT_CHAR:
				case BasicType::BT_WIDE_CHAR:
				case BasicType::BT_INT8:
				case BasicType::BT_INT16:
				case BasicType::BT_INT32:
				case BasicType::BT_INT128:
				case BasicType::BT_UINT8:
				case BasicType::BT_UINT16:
				case BasicType::BT_UINT32:
				case BasicType::BT_UINT64:
				case BasicType::BT_UINT128:
					return clang::CastKind::CK_IntegralCast;

				case BasicType::BT_FLOAT32:
				case BasicType::BT_FLOAT64:
					return clang::CastKind::CK_IntegralToFloating;

				case BasicType::BT_BOOLEAN:
					return clang::CastKind::CK_IntegralToBoolean;

				case BasicType::BT_VOID:
					return clang::CastKind::CK_ToVoid;
				default:
					OPS_ASSERT(!"Unexpected BasicType pBaseTypeCastTo");
				};

			case BasicType::BT_INT128:
				switch(pBaseTypeCastTo->getKind())
				{

				case BasicType::BT_INT128:
					return clang::CastKind::CK_NoOp;

				case BasicType::BT_CHAR:
				case BasicType::BT_WIDE_CHAR:
				case BasicType::BT_INT8:
				case BasicType::BT_INT16:
				case BasicType::BT_INT32:
				case BasicType::BT_INT64:
				case BasicType::BT_UINT8:
				case BasicType::BT_UINT16:
				case BasicType::BT_UINT32:
				case BasicType::BT_UINT64:
				case BasicType::BT_UINT128:
					return clang::CastKind::CK_IntegralCast;

				case BasicType::BT_FLOAT32:
				case BasicType::BT_FLOAT64:
					return clang::CastKind::CK_IntegralToFloating;

				case BasicType::BT_BOOLEAN:
					return clang::CastKind::CK_IntegralToBoolean;

				case BasicType::BT_VOID:
					return clang::CastKind::CK_ToVoid;
				default:
					OPS_ASSERT(!"Unexpected BasicType pBaseTypeCastTo");
				};

			case BasicType::BT_UINT8:
				switch(pBaseTypeCastTo->getKind())
				{
				case BasicType::BT_UINT8:
					return clang::CastKind::CK_NoOp;

				case BasicType::BT_CHAR:
				case BasicType::BT_WIDE_CHAR:
				case BasicType::BT_INT8:
				case BasicType::BT_INT16:
				case BasicType::BT_INT32:
				case BasicType::BT_INT64:
				case BasicType::BT_INT128:
				case BasicType::BT_UINT16:
				case BasicType::BT_UINT32:
				case BasicType::BT_UINT64:
				case BasicType::BT_UINT128:
					return clang::CastKind::CK_IntegralCast;

				case BasicType::BT_FLOAT32:
				case BasicType::BT_FLOAT64:
					return clang::CastKind::CK_IntegralToFloating;

				case BasicType::BT_BOOLEAN:
					return clang::CastKind::CK_IntegralToBoolean;

				case BasicType::BT_VOID:
					return clang::CastKind::CK_ToVoid;
				default:
					OPS_ASSERT(!"Unexpected BasicType pBaseTypeCastTo");
				};

			case BasicType::BT_UINT16:
				switch(pBaseTypeCastTo->getKind())
				{
				case BasicType::BT_UINT16:
					return clang::CastKind::CK_NoOp;

				case BasicType::BT_CHAR:
				case BasicType::BT_WIDE_CHAR:
				case BasicType::BT_INT8:
				case BasicType::BT_INT16:
				case BasicType::BT_INT32:
				case BasicType::BT_INT64:
				case BasicType::BT_INT128:
				case BasicType::BT_UINT8:
				case BasicType::BT_UINT32:
				case BasicType::BT_UINT64:
				case BasicType::BT_UINT128:
					return clang::CastKind::CK_IntegralCast;

				case BasicType::BT_FLOAT32:
				case BasicType::BT_FLOAT64:
					return clang::CastKind::CK_IntegralToFloating;

				case BasicType::BT_BOOLEAN:
					return clang::CastKind::CK_IntegralToBoolean;

				case BasicType::BT_VOID:
					return clang::CastKind::CK_ToVoid;
				default:
					OPS_ASSERT(!"Unexpected BasicType pBaseTypeCastTo");
				};

			case BasicType::BT_UINT32:
				switch(pBaseTypeCastTo->getKind())
				{
				case BasicType::BT_UINT32:
					return clang::CastKind::CK_NoOp;

				case BasicType::BT_CHAR:
				case BasicType::BT_WIDE_CHAR:
				case BasicType::BT_INT8:
				case BasicType::BT_INT16:
				case BasicType::BT_INT32:
				case BasicType::BT_INT64:
				case BasicType::BT_INT128:
				case BasicType::BT_UINT8:
				case BasicType::BT_UINT16:
				case BasicType::BT_UINT64:
				case BasicType::BT_UINT128:
					return clang::CastKind::CK_IntegralCast;

				case BasicType::BT_FLOAT32:
				case BasicType::BT_FLOAT64:
					return clang::CastKind::CK_IntegralToFloating;

				case BasicType::BT_BOOLEAN:
					return clang::CastKind::CK_IntegralToBoolean;

				case BasicType::BT_VOID:
					return clang::CastKind::CK_ToVoid;
				default:
					OPS_ASSERT(!"Unexpected BasicType pBaseTypeCastTo");
				};

			case BasicType::BT_UINT64:
				switch(pBaseTypeCastTo->getKind())
				{
				case BasicType::BT_UINT64:
					return clang::CastKind::CK_NoOp;

				case BasicType::BT_CHAR:
				case BasicType::BT_WIDE_CHAR:
				case BasicType::BT_INT8:
				case BasicType::BT_INT16:
				case BasicType::BT_INT32:
				case BasicType::BT_INT64:
				case BasicType::BT_INT128:
				case BasicType::BT_UINT8:
				case BasicType::BT_UINT16:
				case BasicType::BT_UINT32:
				case BasicType::BT_UINT128:
					return clang::CastKind::CK_IntegralCast;

				case BasicType::BT_FLOAT32:
				case BasicType::BT_FLOAT64:
					return clang::CastKind::CK_IntegralToFloating;

				case BasicType::BT_BOOLEAN:
					return clang::CastKind::CK_IntegralToBoolean;

				case BasicType::BT_VOID:
					return clang::CastKind::CK_ToVoid;
				default:
					OPS_ASSERT(!"Unexpected BasicType pBaseTypeCastTo");
				};

			case BasicType::BT_UINT128:
				switch(pBaseTypeCastTo->getKind())
				{
				case BasicType::BT_UINT128:
					return clang::CastKind::CK_NoOp;

				case BasicType::BT_CHAR:
				case BasicType::BT_WIDE_CHAR:
				case BasicType::BT_INT8:
				case BasicType::BT_INT16:
				case BasicType::BT_INT32:
				case BasicType::BT_INT64:
				case BasicType::BT_INT128:
				case BasicType::BT_UINT8:
				case BasicType::BT_UINT16:
				case BasicType::BT_UINT32:
				case BasicType::BT_UINT64:
					return clang::CastKind::CK_IntegralCast;

				case BasicType::BT_FLOAT32:
				case BasicType::BT_FLOAT64:
					return clang::CastKind::CK_IntegralToFloating;

				case BasicType::BT_BOOLEAN:
					return clang::CastKind::CK_IntegralToBoolean;

				case BasicType::BT_VOID:
					return clang::CastKind::CK_ToVoid;
				default:
					OPS_ASSERT(!"Unexpected BasicType pBaseTypeCastTo");
				};

			case BasicType::BT_FLOAT32:
				switch(pBaseTypeCastTo->getKind())
				{
				case BasicType::BT_FLOAT32:
					return clang::CastKind::CK_NoOp;

				case BasicType::BT_CHAR:
				case BasicType::BT_WIDE_CHAR:
				case BasicType::BT_INT8:
				case BasicType::BT_INT16:
				case BasicType::BT_INT32:
				case BasicType::BT_INT64:
				case BasicType::BT_INT128:
				case BasicType::BT_UINT8:
				case BasicType::BT_UINT16:
				case BasicType::BT_UINT32:
				case BasicType::BT_UINT64:
				case BasicType::BT_UINT128:
					return clang::CastKind::CK_FloatingToIntegral;

				case BasicType::BT_FLOAT64:
					return clang::CastKind::CK_FloatingCast;

				case BasicType::BT_BOOLEAN:
					return clang::CastKind::CK_FloatingToBoolean;

				case BasicType::BT_VOID:
					return clang::CastKind::CK_ToVoid;
				default:
					OPS_ASSERT(!"Unexpected BasicType pBaseTypeCastTo");
				};

			case BasicType::BT_FLOAT64:
				switch(pBaseTypeCastTo->getKind())
				{
				case BasicType::BT_FLOAT64:
					return clang::CastKind::CK_NoOp;

				case BasicType::BT_CHAR:
				case BasicType::BT_WIDE_CHAR:
				case BasicType::BT_INT8:
				case BasicType::BT_INT16:
				case BasicType::BT_INT32:
				case BasicType::BT_INT64:
				case BasicType::BT_INT128:
				case BasicType::BT_UINT8:
				case BasicType::BT_UINT16:
				case BasicType::BT_UINT32:
				case BasicType::BT_UINT64:
				case BasicType::BT_UINT128:
					return clang::CastKind::CK_FloatingToIntegral;

				case BasicType::BT_FLOAT32:
					return clang::CastKind::CK_FloatingCast;

				case BasicType::BT_BOOLEAN:
					return clang::CastKind::CK_FloatingToBoolean;

				case BasicType::BT_VOID:
					return clang::CastKind::CK_ToVoid;
				default:
					OPS_ASSERT(!"Unexpected BasicType pBaseTypeCastTo");
				};

			case BasicType::BT_BOOLEAN:
				switch(pBaseTypeCastTo->getKind())
				{
				case BasicType::BT_BOOLEAN:
					return clang::CastKind::CK_NoOp;

				case BasicType::BT_CHAR:
				case BasicType::BT_WIDE_CHAR:
				case BasicType::BT_INT8:
				case BasicType::BT_INT16:
				case BasicType::BT_INT32:
				case BasicType::BT_INT64:
				case BasicType::BT_INT128:
				case BasicType::BT_UINT8:
				case BasicType::BT_UINT16:
				case BasicType::BT_UINT32:
				case BasicType::BT_UINT64:
				case BasicType::BT_UINT128:
					return clang::CastKind::CK_IntegralCast;

				case BasicType::BT_FLOAT32:
				case BasicType::BT_FLOAT64:
					return clang::CastKind::CK_IntegralToFloating;

				case BasicType::BT_VOID:
					return clang::CastKind::CK_ToVoid;
				default:
					OPS_ASSERT(!"Unexpected BasicType pBaseTypeCastTo");
				};

			default:
				OPS_ASSERT(!"Unexpected BasicType pBaseTypeArg");
			};

		}	// else if (pTypeCastTo->is_a <PtrType>())
	}	// else if (pTypeArg->is_a <PtrType>())

	return clang::CastKind::CK_NoOp; // TODO обработка ВСЕХ остальных случаев
}

clang::Expr* Private::usualUnaryConversions(
	clang::ASTContext& context,
	clang::Expr* E)
{
	//////////////////////////
	/// clang SemaExpr.cpp l766
	clang::QualType Ty = E->getType();
	OPS_ASSERT(!Ty.isNull() && "UsualUnaryConversions - missing type");

	if (Ty->isIntegralOrUnscopedEnumerationType())
	{
		clang::QualType PTy = context.isPromotableBitField(E);
		if (!PTy.isNull())
		{
			return clang::ImplicitCastExpr::Create(
				context,
				PTy,
				clang::CastKind::CK_IntegralCast,
				E,
				new(context) clang::CXXCastPath(),
				clang::VK_RValue);
		}
		if (Ty->isPromotableIntegerType())
		{
			clang::QualType PT = context.getPromotedIntegerType(Ty);
			return clang::ImplicitCastExpr::Create(
				context,
				PT,
				clang::CastKind::CK_IntegralCast,
				E,
				new(context) clang::CXXCastPath(),
				clang::VK_RValue);
		}
	}
	return E;
	//////////////////////////
}

ExprTuple Private::usualArithmeticConversions(
	clang::ASTContext& context,
	clang::Expr* LHS,
	clang::Expr* RHS,
	bool isCompAssign)
{
	//////////////////////////
	/// clang SemaExpr.cpp l1315

	if (!isCompAssign)
	{
		LHS = usualUnaryConversions(context, LHS);
	}

	RHS = usualUnaryConversions(context, RHS);

	clang::QualType LHSType =
		context.getCanonicalType(LHS->getType()).getUnqualifiedType();
	clang::QualType RHSType =
		context.getCanonicalType(RHS->getType()).getUnqualifiedType();
	if (const clang::AtomicType *AtomicLHS = LHSType->getAs<clang::AtomicType>())
		LHSType = AtomicLHS->getValueType();

	if (LHSType == RHSType)
		return ExprTuple(LHS, RHS);

	clang::QualType LHSUnpromotedType = LHSType;
	if (LHSType->isPromotableIntegerType())
		LHSType = context.getPromotedIntegerType(LHSType);
	clang::QualType LHSBitfieldPromoteTy = context.isPromotableBitField(LHS);
	if (!LHSBitfieldPromoteTy.isNull())
		LHSType = LHSBitfieldPromoteTy;
	if (LHSType != LHSUnpromotedType && !isCompAssign)
		LHS = clang::ImplicitCastExpr::Create(
			context,
			LHSType,
			clang::CastKind::CK_IntegralCast,
			LHS,
			new(context) clang::CXXCastPath(),
			clang::VK_RValue);
	else
		LHSType = LHSUnpromotedType;

	if (LHSType == RHSType)
		return ExprTuple(LHS, RHS);

	// one or both of lhs and rhs - float
	if (LHSType->isRealFloatingType() || RHSType->isRealFloatingType())
	{
		bool LHSFloat = LHSType->isRealFloatingType();
		bool RHSFloat = RHSType->isRealFloatingType();

		if (LHSFloat && RHSFloat)	// floating () floating
		{
			int order = context.getFloatingTypeOrder(LHSType, RHSType);
			if (order > 0)		// double () float
			{
				RHS = clang::ImplicitCastExpr::Create(	// float -> double
					context,
					LHSType,
					clang::CastKind::CK_FloatingCast,
					RHS,
					new(context) clang::CXXCastPath(),
					clang::VK_RValue);
				return ExprTuple(LHS, RHS);
			}

			if (!isCompAssign)	// float () double
			{
				LHS = clang::ImplicitCastExpr::Create(	// float -> double
					context,
					RHSType,
					clang::CastKind::CK_FloatingCast,
					LHS,
					new(context) clang::CXXCastPath(),
					clang::VK_RValue);
			}
			else // float = double
			{
				RHS = clang::ImplicitCastExpr::Create(	// double -> float
					context,
					LHSType,
					clang::CastKind::CK_FloatingCast,
					RHS,
					new(context) clang::CXXCastPath(),
					clang::VK_RValue);
			}
			return ExprTuple(LHS, RHS);
		}

		if (LHSFloat)	// floating () integer
		{
			if (RHSType->isIntegerType())
			{
				RHS = clang::ImplicitCastExpr::Create(	// int -> floating
					context,
					LHSType,
					clang::CastKind::CK_IntegralToFloating,
					RHS,
					new(context) clang::CXXCastPath(),
					clang::VK_RValue);
				return ExprTuple(LHS, RHS);
			}
		}

		if (!isCompAssign)	// int () floating
		{
			LHS = clang::ImplicitCastExpr::Create(	// int -> floating
				context,
				RHSType,
				clang::CastKind::CK_IntegralToFloating,
				LHS,
				new(context) clang::CXXCastPath(),
				clang::VK_RValue);

		}
		else	// int = floating
		{
			RHS = clang::ImplicitCastExpr::Create(	// floating -> int
				context,
				LHSType,
				clang::CastKind::CK_FloatingToIntegral,
				RHS,
				new(context) clang::CXXCastPath(),
				clang::VK_RValue);
		}
		return ExprTuple(LHS, RHS);
	}

	// lhs and rhs - integers
	int order = context.getIntegerTypeOrder(LHSType, RHSType);
	bool LHSSigned = LHSType->hasSignedIntegerRepresentation();
	bool RHSSigned = RHSType->hasSignedIntegerRepresentation();
	if (LHSSigned == RHSSigned)
	{
		if (order >= 0)
		{
			RHS = clang::ImplicitCastExpr::Create(
				context,
				LHSType,
				clang::CastKind::CK_IntegralCast,
				RHS,
				new(context) clang::CXXCastPath(),
				clang::VK_RValue);
			return ExprTuple(LHS, RHS);
		}
		else if (!isCompAssign)
		{
			LHS = clang::ImplicitCastExpr::Create(
				context,
				RHSType,
				clang::CastKind::CK_IntegralCast,
				LHS,
				new(context) clang::CXXCastPath(),
				clang::VK_RValue);
		}
		else
		{
			RHS = clang::ImplicitCastExpr::Create(
				context,
				LHSType,
				clang::CastKind::CK_IntegralCast,
				RHS,
				new(context) clang::CXXCastPath(),
				clang::VK_RValue);
		}
		return ExprTuple(LHS, RHS);
	}
	else if (order != (LHSSigned ? 1 : -1))
	{
		if (RHSSigned)
		{
			RHS = clang::ImplicitCastExpr::Create(
				context,
				LHSType,
				clang::CastKind::CK_IntegralCast,
				RHS,
				new(context) clang::CXXCastPath(),
				clang::VK_RValue);
			return ExprTuple(LHS, RHS);
		}
		else if (!isCompAssign)
		{
			LHS = clang::ImplicitCastExpr::Create(
				context,
				RHSType,
				clang::CastKind::CK_IntegralCast,
				LHS,
				new(context) clang::CXXCastPath(),
				clang::VK_RValue);
		}
		else
		{
			RHS = clang::ImplicitCastExpr::Create(
				context,
				LHSType,
				clang::CastKind::CK_IntegralCast,
				RHS,
				new(context) clang::CXXCastPath(),
				clang::VK_RValue);
		}
		return ExprTuple(LHS, RHS);
	}
	else if (context.getIntWidth(LHSType) != context.getIntWidth(RHSType))
	{
		if (LHSSigned)
		{
			RHS = clang::ImplicitCastExpr::Create(
				context,
				LHSType,
				clang::CastKind::CK_IntegralCast,
				RHS,
				new(context) clang::CXXCastPath(),
				clang::VK_RValue);
			return ExprTuple(LHS, RHS);
		}
		else if (!isCompAssign)
		{
			LHS = clang::ImplicitCastExpr::Create(
				context,
				RHSType,
				clang::CastKind::CK_IntegralCast,
				LHS,
				new(context) clang::CXXCastPath(),
				clang::VK_RValue);
		}
		else
		{
			RHS = clang::ImplicitCastExpr::Create(
				context,
				LHSType,
				clang::CastKind::CK_IntegralCast,
				RHS,
				new(context) clang::CXXCastPath(),
				clang::VK_RValue);
		}
		return ExprTuple(LHS, RHS);
	}
	clang::QualType result =
		context.getCorrespondingUnsignedType(LHSSigned ? LHSType : RHSType);
	RHS = clang::ImplicitCastExpr::Create(
		context,
		result,
		clang::CastKind::CK_IntegralCast,
		RHS,
		new(context) clang::CXXCastPath(),
		clang::VK_RValue);
	if (!isCompAssign)
	{
		LHS = clang::ImplicitCastExpr::Create(
			context,
			result,
			clang::CastKind::CK_IntegralCast,
			LHS,
			new(context) clang::CXXCastPath(),
			clang::VK_RValue);
	}
	return ExprTuple(LHS, RHS);
	//////////////////////////
}

//////////////////////////////////////////////////////////////////////
// getClangExpr()

clang::Expr* OPS::Backends::Clang::Internal::getClangExpr(
	clang::ASTContext &rASTContext,
	ExpressionBase& rExpressionBase,
	R2CDeclNodes& rR2CDeclNodes,
	bool needLtoRconversion)
{
	ExprWalker walker(rASTContext, rR2CDeclNodes);
	rExpressionBase.accept(walker);
	clang::Expr* pResultExpr = walker.getResultExpr();
	if (needLtoRconversion)
	{
		if (pResultExpr->getValueKind() == clang::ExprValueKind::VK_LValue)
			return clang::ImplicitCastExpr::Create(
				rASTContext,
				pResultExpr->getType(),
				clang::CK_LValueToRValue,
				pResultExpr,
				new(rASTContext) clang::CXXCastPath(),
				clang::VK_RValue);
	}
	return pResultExpr;
}

// End of File
