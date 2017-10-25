/// ClangTypes.cpp
///   Create clang types from Reprise types.
/// Author: Denis Dubrov (dubrov@sfedu.ru)
/// Created: 12.04.2013

#include "ClangTypes.h"

#include "ClangExpressions.h"

#include "OPS_Core/Helpers.h"
#include "Reprise/Service/DeepWalker.h"
#include "Reprise/Declarations.h"
#include "Reprise/Expressions.h"
#include "Reprise/Types.h"

#include "OPS_Core/disable_llvm_warnings_begin.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/Basic/Specifiers.h"
#include "clang/Basic/TargetInfo.h"
#include "OPS_Core/disable_llvm_warnings_end.h"

using namespace OPS::Backends::Clang::Internal;
using namespace OPS::Reprise;

//////////////////////////////////////////////////////////////////////
// Internal

namespace
{
	class TypeWalker : public OPS::Reprise::Service::DeepWalker
	{
	public:
		//
		TypeWalker(
			clang::ASTContext &rASTContext,
			R2CDeclNodes& rR2CDeclNodes);
		//
	// Attributes
	public:
		//
		clang::QualType getResultType() const;
		//
	// Overrides
	public:
		//
		virtual void visit(BasicType& rBasicType);
		virtual void visit(PtrType& rPtrType);
		virtual void visit(TypedefType& rTypedefType);
		virtual void visit(ArrayType& rArrayType);
		virtual void visit(StructType& rStructType);
		virtual void visit(SubroutineType& rSubroutineType);
		virtual void visit(DeclaredType& rDeclaredType);
		virtual void visit(VectorType& rVectorType);
		//
	private:
		//
		clang::ASTContext &m_rASTContext;
		R2CDeclNodes& m_rR2CDeclNodes;
		const clang::TargetInfo& m_rcTargetInfo;
		clang::QualType m_ResultType;
	};
}

TypeWalker::TypeWalker(
	clang::ASTContext &rASTContext,
	R2CDeclNodes& rR2CDeclNodes)
	: m_rASTContext(rASTContext),
		m_rR2CDeclNodes(rR2CDeclNodes),
		m_rcTargetInfo(rASTContext.getTargetInfo())
{
	//
}

clang::QualType TypeWalker::getResultType() const
{
	return m_ResultType;
}

void TypeWalker::visit(BasicType& rBasicType)
{
	switch (rBasicType.getKind())
	{
		case BasicType::BT_CHAR:
			//
			m_ResultType = m_rASTContext.CharTy;
			//
			break;
			//
		case BasicType::BT_WIDE_CHAR:
			//
			m_ResultType = m_rASTContext.WCharTy;
			//
			break;
			//
		case BasicType::BT_INT8:
			//
			m_ResultType = m_rASTContext.SignedCharTy;
			//
			break;
			//
		case BasicType::BT_INT16:
			//
			m_ResultType = m_rASTContext.ShortTy;
			//
			break;
			//
		case BasicType::BT_INT32:
			//
			m_ResultType = m_rASTContext.IntTy;
			//
			break;
			//
		case BasicType::BT_INT64:
			//
			m_ResultType = m_rASTContext.LongTy;
			//
			break;
			//
		case BasicType::BT_INT128:
			//
			m_ResultType = m_rASTContext.Int128Ty;
			//
			break;
			//
		case BasicType::BT_UINT8:
			//
			m_ResultType = m_rASTContext.UnsignedCharTy;
			//
			break;
			//
		case BasicType::BT_UINT16:
			//
			m_ResultType = m_rASTContext.UnsignedShortTy;
			//
			break;
			//
		case BasicType::BT_UINT32:
			//
			m_ResultType = m_rASTContext.UnsignedIntTy;
			//
			break;
			//
		case BasicType::BT_UINT64:
			//
			m_ResultType = m_rASTContext.UnsignedLongTy;
			//
			break;
			//
		case BasicType::BT_UINT128:
			//
			m_ResultType = m_rASTContext.UnsignedInt128Ty;
			//
			break;
			//
		case BasicType::BT_FLOAT32:
			//
			m_ResultType = m_rASTContext.FloatTy;
			//
			break;
			//
		case BasicType::BT_FLOAT64:
			//
			m_ResultType = m_rASTContext.DoubleTy;
			//
			break;
			//
		case BasicType::BT_BOOLEAN:
			//
			m_ResultType = m_rASTContext.BoolTy;
			//
			break;
			//
		case BasicType::BT_STRING:
			//
			m_ResultType = m_rASTContext.getPointerType(m_rASTContext.CharTy);
			//
			break;
			//
		case BasicType::BT_WIDE_STRING:
			//
			m_ResultType = m_rASTContext.getPointerType(m_rASTContext.WCharTy);
			//
			break;
			//
		case BasicType::BT_VOID:
			//
			m_ResultType = m_rASTContext.VoidTy;
			//
			break;
			//
		OPS_DEFAULT_CASE_LABEL;
			//
	}    // switch (rBasicType.getKind())
}

void TypeWalker::visit(PtrType& rPtrType)
{
	clang::QualType pointedType = getClangQualType(
		m_rASTContext, rPtrType.getPointedType(), m_rR2CDeclNodes);

	m_ResultType = m_rASTContext.getPointerType(pointedType);

	if (rPtrType.isRestrict())
		m_ResultType = m_rASTContext.getRestrictType(m_ResultType);
}

void TypeWalker::visit(TypedefType& rTypedefType)
{
	clang::QualType typedefedType = getClangQualType(
		m_rASTContext, rTypedefType.getBaseType(), m_rR2CDeclNodes);

	TypeDeclaration* pTypeDeclaration =
		rTypedefType.getParent()->cast_ptr <TypeDeclaration> ();
	const clang::TypedefNameDecl* pcTypedefDecl = 0;
	R2CDeclNodes::const_iterator ci = m_rR2CDeclNodes.find(pTypeDeclaration);
	if (ci != m_rR2CDeclNodes.end())
		pcTypedefDecl =
			llvm::dyn_cast <clang::TypedefNameDecl> (ci->second);
	else
		OPS_ASSERT(
			false &&
			"No Reprise typedef declaration found for the clang typedef decl");

	m_ResultType = m_rASTContext.getTypedefType(pcTypedefDecl, typedefedType);
}

void TypeWalker::visit(ArrayType& rArrayType)
{
	clang::QualType elementType = getClangQualType(
		m_rASTContext, rArrayType.getBaseType(), m_rR2CDeclNodes);

	if (rArrayType.isFullType())
	{
		if (rArrayType.hasCountExpression())
		{
			clang::Expr* pSizeExpr = getClangExpr(
				m_rASTContext, rArrayType.getCountExpression(),
				m_rR2CDeclNodes);

			m_ResultType = m_rASTContext.getVariableArrayType(
				elementType, pSizeExpr, clang::ArrayType::Normal,
				0, clang::SourceRange());
		}
		else
		{
			const llvm::APInt cArraySize(
				m_rcTargetInfo.getTypeWidth(m_rcTargetInfo.getSizeType()),
				static_cast <uint64_t> (rArrayType.getElementCount()),
				false);

			m_ResultType = m_rASTContext.getConstantArrayType(
				elementType, cArraySize, clang::ArrayType::Normal, 0);
		}
	}
	else
		m_ResultType = m_rASTContext.getIncompleteArrayType(
			elementType, clang::ArrayType::Normal, 0);
}

void TypeWalker::visit(StructType& rStructType)
{
	// Since clang differs from Reprise in holding the
	//   struct contents information: RecordDecl contains
	//   fields info, not RecordType. So in Reprise all
	//   struct references are actually to DeclaredType

	OPS_UNREACHABLE;
}

void TypeWalker::visit(SubroutineType& rSubroutineType)
{
	clang::QualType returnType = getClangQualType(
		m_rASTContext, rSubroutineType.getReturnType(), m_rR2CDeclNodes);
	clang::CallingConv callingConv;
	switch (rSubroutineType.getCallingKind())
	{
		case SubroutineType::CK_DEFAULT:
			//
			callingConv = clang::CC_Default;
			//
			break;
			//
		case SubroutineType::CK_FASTCALL:
			//
			callingConv = clang::CC_X86FastCall;
			//
			break;
			//
		case SubroutineType::CK_STDCALL:
			//
			callingConv = clang::CC_X86StdCall;
			//
			break;
			//
		case SubroutineType::CK_CDECL:
			//
			callingConv = clang::CC_C;
			//
			break;
			//
		case SubroutineType::CK_THISCALL:
			//
			callingConv = clang::CC_X86ThisCall;
			//
			break;
			//
		case SubroutineType::CK_PASCAL:
			//
			callingConv = clang::CC_X86Pascal;
			//
			break;
			//
		default:
			//
			OPS_ASSERT(false && "Unreachable");
			//
			break;
	}

	clang::FunctionType::ExtInfo extInfo;
	extInfo = extInfo.withCallingConv(callingConv);

	if (rSubroutineType.isArgsKnown())
	{
		clang::FunctionProtoType::ExtProtoInfo extProtoInfo;
		extProtoInfo.ExtInfo = extInfo;
		extProtoInfo.Variadic = rSubroutineType.isVarArg();

		const int cnArgs = rSubroutineType.getParameterCount();
		const unsigned cuArgs = static_cast <unsigned> (cnArgs);
		llvm::SmallVector <clang::QualType, 40> arguments;
		arguments.reserve(cuArgs);
		for (int i = 0; i < cnArgs; ++ i)
		{
			clang::QualType argType = getClangQualType(
				m_rASTContext,
				rSubroutineType.getParameter(i).getType(),
				m_rR2CDeclNodes);

			arguments.push_back(argType);
		}

		m_ResultType = m_rASTContext.getFunctionType(
			returnType, arguments, extProtoInfo);
	}
	else
		m_ResultType = m_rASTContext.getFunctionNoProtoType(
			returnType, extInfo);
}

void TypeWalker::visit(DeclaredType& rDeclaredType)
{
	const TypeDeclaration *pcTypeDeclaration =
		&rDeclaredType.getDeclaration();
	const clang::Decl* pcDecl = 0;
	R2CDeclNodes::const_iterator ci =
		m_rR2CDeclNodes.find(pcTypeDeclaration);
	if (ci != m_rR2CDeclNodes.end())
		pcDecl = ci->second;
	else
		OPS_ASSERT(
			false &&
			"No Reprise typedef declaration found for the clang typedef decl");

	if (const clang::TypedefNameDecl* pcTypedefDecl =
		llvm::dyn_cast_or_null <clang::TypedefNameDecl> (pcDecl))
	{
		m_ResultType = m_rASTContext.getTypedefType(pcTypedefDecl);
	}

	if (const clang::RecordDecl* pcRecordDecl =
		llvm::dyn_cast_or_null <clang::RecordDecl> (pcDecl))
	{
		m_ResultType = m_rASTContext.getRecordType(pcRecordDecl);
	}
}

void TypeWalker::visit(VectorType& rVectorType)
{
	const unsigned cuNumElements = static_cast <const unsigned> (
		rVectorType.getElementCount());
	clang::QualType elementType = getClangQualType(
		m_rASTContext, rVectorType.getBaseType(), m_rR2CDeclNodes);

	m_ResultType = m_rASTContext.getVectorType(
		elementType, cuNumElements, clang::VectorType::GenericVector);
}

//////////////////////////////////////////////////////////////////////
// getClangStorageClass()

clang::StorageClass OPS::Backends::Clang::Internal::getClangStorageClass(
	const VariableDeclarators& rcVariableDeclarators)
{
	if (rcVariableDeclarators.isExtern())
		return clang::SC_Extern;

	if (rcVariableDeclarators.isStatic())
		return clang::SC_Static;

	return clang::SC_None;
}

//////////////////////////////////////////////////////////////////////
// getClangQualType()

clang::QualType OPS::Backends::Clang::Internal::getClangQualType(
	clang::ASTContext &rASTContext,
	TypeBase& rType,
	R2CDeclNodes& rR2CDeclNodes)
{
	TypeWalker walker(rASTContext, rR2CDeclNodes);
	rType.accept(walker);
	clang::QualType resultType = walker.getResultType();

	if (rType.isConst())
		resultType = rASTContext.getConstType(resultType);

	if (rType.isVolatile())
		resultType = rASTContext.getVolatileType(resultType);

	//OPS_ASSERT(!resultType.isNull());
	assert(!resultType.isNull());

	return resultType;
}

// End of File
