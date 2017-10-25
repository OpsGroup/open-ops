/// ClangWalker.cpp
///   RepriseToClang backend visitor.
/// Author: Denis Dubrov (dubrov@sfedu.ru)
/// Created: 19.02.2013

#include "ClangWalker.h"

#include "ClangExpressions.h"
#include "ClangStatements.h"
#include "ClangTypes.h"

#include "OPS_Core/disable_llvm_warnings_begin.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"

#include "llvm/Support/raw_ostream.h"
#include "OPS_Core/disable_llvm_warnings_end.h"

using namespace OPS::Backends::Clang::Internal;
using namespace OPS::Reprise;

using namespace std;

//////////////////////////////////////////////////////////////////////
// Globals

namespace
{
	clang::IdentifierInfo* getIdentifierInfo(
		OPS::Backends::Clang::ASTContexts& rGeneratedProgram,
		const string& rcName)
	{
		return &rGeneratedProgram.back()->Idents.get(rcName);
	}
}

//////////////////////////////////////////////////////////////////////
// ClangWalker

OPS::Backends::Clang::Internal::ClangWalker::ClangWalker(
	ASTContexts& rGeneratedProgram,
	Internal::ASTContextParams& rASTContextParams)
	:	m_rGeneratedProgram(rGeneratedProgram),
		m_rASTContextParams(rASTContextParams)
{
	//
}

//////////////////////////////////////////////////////////////////////
// ClangWalker overrides

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	ProgramUnit& rProgramUnit)
{
	putClangParent(rProgramUnit, NULL);

	m_rGeneratedProgram.reserve(
		rProgramUnit.getChildCount());

	DeepWalker::visit(rProgramUnit);
}

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	TranslationUnit& rTranslationUnit)
{
	// ASTContext

	ASTContextPtr ptrContext = m_rASTContextParams.createASTContext();
	m_rGeneratedProgram.push_back(ptrContext);

	putClangParent(
		rTranslationUnit,
		ptrContext->getTranslationUnitDecl());

	m_R2CDeclNodes.clear();

	m_DeclContexts.push(ptrContext->getTranslationUnitDecl());
	DeepWalker::visit(rTranslationUnit);
	m_DeclContexts.pop();
}

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	Declarations& rDeclarations)
{
	putClangParent(
		rDeclarations, m_DeclContexts.top());

	DeepWalker::visit(rDeclarations);
}

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	OPS::Reprise::VariableDeclaration& rVariableDeclaration)
{
	clang::ASTContext &rASTContext =
		*m_rGeneratedProgram.back();
	clang::DeclContext* pContext =
		getClangParent(rVariableDeclaration);
	clang::IdentifierInfo* pcIdentifierInfo = getIdentifierInfo(
		m_rGeneratedProgram, rVariableDeclaration.getName());
	clang::QualType qualType = getClangQualType(
		rVariableDeclaration.getType());

	clang::VarDecl::StorageClass storageWritten =
		getClangStorageClass(rVariableDeclaration.getDeclarators());
	
	// Not used since clang 3.3
	//
	// clang::VarDecl::StorageClass storageReal =
	// 	(clang::SC_Extern == storageWritten ?
	//	clang::SC_Extern : clang::SC_Static);

	clang::VarDecl* pDecl = clang::VarDecl::Create(
		*m_rGeneratedProgram.back(), pContext,
		clang::SourceLocation(), clang::SourceLocation(),
		pcIdentifierInfo, qualType,
		rASTContext.getTrivialTypeSourceInfo(qualType),
		storageWritten);

	if (rVariableDeclaration.hasNonEmptyInitExpression())
	{
		clang::Expr* pInitExpr = getClangExpr(
			rASTContext, rVariableDeclaration.getInitExpression(),
			m_R2CDeclNodes);

		pDecl->setInit(pInitExpr);
	}

	clang::DeclContext* pDeclContext =
		clang::Decl::castToDeclContext(m_DeclContexts.top());
	pDeclContext->addDecl(pDecl);

	putClangParent(rVariableDeclaration, pDecl);

	DeepWalker::visit(rVariableDeclaration);
}

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	TypeDeclaration& rTypeDeclaration)
{
	clang::ASTContext &rASTContext =
		*m_rGeneratedProgram.back();
	clang::DeclContext* pDeclContext =
		getClangParent(rTypeDeclaration);
	//clang::DeclContext* pDeclContext =
	//	clang::Decl::castToDeclContext(m_DeclContexts.top());
	clang::IdentifierInfo* pIdentifierInfo = getIdentifierInfo(
		m_rGeneratedProgram, rTypeDeclaration.getName());

	clang::Decl* pDecl = 0;

	if (TypedefType* pTypedefType =
		rTypeDeclaration.getType().cast_ptr <TypedefType> ())
	{
		clang::QualType qualType = getClangQualType(
			pTypedefType->getBaseType());

		//clang::TypedefDecl* pTypedefDecl = clang::TypedefDecl::Create(
		pDecl = clang::TypedefDecl::Create(
			rASTContext,
			pDeclContext,
			clang::SourceLocation(),
			clang::SourceLocation(),
			pIdentifierInfo,
			rASTContext.getTrivialTypeSourceInfo(qualType));

		//clang::QualType typedefType =	rASTContext.getTypedefType(
		//	pTypedefDecl, qualType);

		//pDecl = pTypedefDecl;
	}

	if (StructType* pStructType =
		rTypeDeclaration.getType().cast_ptr <StructType> ())
	{
		clang::TagTypeKind nTagKind =
			(pStructType->isUnion() ? clang::TTK_Union : clang::TTK_Struct);
		
		//clang::RecordDecl* pRecordDecl = clang::RecordDecl::Create(
		pDecl = clang::RecordDecl::Create(
			rASTContext,
			nTagKind,
			pDeclContext,
			clang::SourceLocation(),
			clang::SourceLocation(),
			pIdentifierInfo,
			(clang::RecordDecl*) 0);    // ToDo: prev decl

		//pDecl = pRecordDecl;
	}

	assert(pDecl);

	pDeclContext->addDecl(pDecl);
	m_R2CDeclNodes.insert(make_pair(&rTypeDeclaration, pDecl)); 

	m_DeclContexts.push(pDecl);
	DeepWalker::visit(rTypeDeclaration);
	m_DeclContexts.pop();
}

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	SubroutineDeclaration& rSubroutineDeclaration)
{
	// Type construction

	clang::ASTContext &rASTContext =
		*m_rGeneratedProgram.back();

	SubroutineType& rSubroutineType = rSubroutineDeclaration.getType();

	clang::DeclContext* pDeclContext =
		getClangParent(rSubroutineDeclaration);
	clang::DeclarationName declName(
		getIdentifierInfo(
			m_rGeneratedProgram,
			rSubroutineDeclaration.getName()));
	clang::QualType qualType = getClangQualType(
		rSubroutineType);

	clang::FunctionDecl::StorageClass storageWritten =
		getClangStorageClass(rSubroutineDeclaration.getDeclarators());
	
	// Not used since clang 3.3
	//
	// clang::FunctionDecl::StorageClass storageReal =
	// 	(clang::SC_Static == storageWritten ?
	// 	clang::SC_Static : clang::SC_Extern);

	clang::FunctionDecl* pDecl = clang::FunctionDecl::Create(
		rASTContext,
		pDeclContext,
		clang::SourceLocation(),
		clang::SourceLocation(),
		declName,
		qualType,
		rASTContext.getTrivialTypeSourceInfo(qualType),
		storageWritten);

	putClangParent(rSubroutineDeclaration, pDecl);

	llvm::SmallVector <clang::ParmVarDecl*, 40> params;
	const unsigned cuParams = pDecl->param_size();
	params.reserve(cuParams);
	const int cnParams =
		rSubroutineDeclaration.getType().getParameterCount();

	OPS_ASSERT(cnParams == static_cast <const int> (cuParams));

	const clang::FunctionProtoType* pcFunctionProtoType =
		qualType->castAs <clang::FunctionProtoType> ();
	for (int i = 0; i < cnParams; ++ i)
	{
		const ParameterDescriptor& rcParameterDescriptor =
			rSubroutineDeclaration.getType().getParameter(i);
		clang::IdentifierInfo* pIdentifierInfo = getIdentifierInfo(
			m_rGeneratedProgram, rcParameterDescriptor.getName());
		clang::QualType argQualType =
			pcFunctionProtoType->getArgType(static_cast <unsigned> (i));

		clang::VarDecl::StorageClass storageWritten = clang::SC_None;
		if (rcParameterDescriptor.hasAssociatedVariable())
			storageWritten = getClangStorageClass(
				rcParameterDescriptor.getAssociatedVariable().getDeclarators());

		// Not used since clang 3.3
		//
		// clang::VarDecl::StorageClass storageReal =
		// 	(storageWritten == clang::SC_None ?
		// 	clang::SC_Auto : storageWritten);

		clang::ParmVarDecl* pParmVarDecl = clang::ParmVarDecl::Create(
			rASTContext, pDecl,
			clang::SourceLocation(), clang::SourceLocation(),
			pIdentifierInfo, argQualType,
			rASTContext.getTrivialTypeSourceInfo(argQualType),
			storageWritten, 0);

		params.push_back(pParmVarDecl);
	}

	pDecl->setParams(params);

#if 0

	pDecl->printName(llvm::errs());
	llvm::errs() << '\n';
	for (unsigned i = 0; i < cuParams; ++ i)
	{
		string name = pDecl->getParamDecl(i)->getNameAsString();

		llvm::errs() << "  ";
		pDecl->getParamDecl(i)->printName(llvm::errs());
		llvm::errs() << '\n';
	}
	llvm::errs() << '\n';

#endif

	m_DeclContexts.push(pDecl);

	DeepWalker::visit(rSubroutineDeclaration);

	if (rSubroutineDeclaration.hasImplementation())
	{
		BlockStatement& rBlockBody =
			rSubroutineDeclaration.getBodyBlock();

		clang::Stmt* pStmtBody = getClangStmt(
			rASTContext, rBlockBody, m_R2CDeclNodes);

		pDecl->setBody(pStmtBody);
	}

	rASTContext.getTranslationUnitDecl()->addDecl(pDecl);

	m_DeclContexts.pop();
}

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	StructMemberDescriptor& rStructMemberDescriptor)
{
	clang::ASTContext &rASTContext =
		*m_rGeneratedProgram.back();

	// Not:
	// clang::DeclContext* pDeclContext =
	//   getClangParent(rStructMemberDescriptor);
	//
	// Because unlike Reprise where StructType holds all the record
	//   contents information (=> the parent of StructMemberDescriptor
	//   is StructType), in clang RecordDecl holds it, not RecordType

	clang::Decl* pDecl = m_DeclContexts.top();
	clang::DeclContext* pDeclContext =
		clang::Decl::castToDeclContext(pDecl);
	clang::IdentifierInfo* pIdentifierInfo = getIdentifierInfo(
		m_rGeneratedProgram, rStructMemberDescriptor.getName());
	clang::QualType qualType = getClangQualType(
		rStructMemberDescriptor.getType());

	clang::Expr* pExprBitWidth = 0;
	if (rStructMemberDescriptor.hasLimitExpression())
	{
		pExprBitWidth = getClangExpr(
			rASTContext,
			rStructMemberDescriptor.getLimitExpression(),
			m_R2CDeclNodes);
	}
	else
	{
		const int cnNumBits =
			rStructMemberDescriptor.getBitsLimit();

		if (cnNumBits > 0)
		{
			llvm::APInt intVal(
				rASTContext.getIntWidth(rASTContext.IntTy),
				static_cast <uint64_t> (cnNumBits),
				true);
			pExprBitWidth = clang::IntegerLiteral::Create(
				rASTContext,
				intVal,
				rASTContext.IntTy,
				clang::SourceLocation());
		}
	}

	clang::FieldDecl* pFieldDecl = clang::FieldDecl::Create(
		rASTContext,
		pDeclContext,
		clang::SourceLocation(),
		clang::SourceLocation(),
		pIdentifierInfo,
		qualType,
		rASTContext.getTrivialTypeSourceInfo(qualType),
		pExprBitWidth,
		false,
		clang::ICIS_NoInit);

	pDeclContext->addDecl(pFieldDecl);
	putClangParent(rStructMemberDescriptor, pFieldDecl);
	clang::TagDecl* pTagDecl =
		llvm::dyn_cast <clang::TagDecl> (pDeclContext);
	pTagDecl->setCompleteDefinition(true);

#if 0

	clang::RecordDecl* pRecordDecl =
		llvm::dyn_cast <clang::RecordDecl> (pDeclContext);
	llvm::errs() <<
		"\n\n=============\n";
	pRecordDecl->print(llvm::errs());
	llvm::errs() <<
		"\n";
	pFieldDecl->print(llvm::errs());
	llvm::errs() <<
		"\n=============\n\n";

#endif

	DeepWalker::visit(rStructMemberDescriptor);
}

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	BlockStatement& rBlockStatement)
{
	//clang::ASTContext &rASTContext =
	//	*m_rGeneratedProgram.back();

	//clang::CompoundStmt* pCompoundStmt =
	//	new (rASTContext) clang::CompoundStmt(clang::SourceLocation());

	//
	// Clang CompoundStmt does not provide a declaration context
	//   (storage for local declarations)
	//
	// m_DeclContexts.push(pCompoundStmt);

	DeepWalker::visit(rBlockStatement);

	// m_DeclContexts.pop();
}

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	ForStatement& rForStatement)
{

	DeepWalker::visit(rForStatement);
}

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	ExpressionStatement& rExpressionStatement)
{
	//clang::ASTContext &rASTContext =
	//	*m_rGeneratedProgram.back();

	//const ExpressionBase& rcExpressionBase = rExpressionStatement.get();
	//clang::Expr* pExpr = getClangExpr(
	//	rASTContext, rcExpressionBase, m_R2CDeclNodes);

	DeepWalker::visit(rExpressionStatement);
}

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	ParameterDescriptor& rParameterDescriptor)
{

	DeepWalker::visit(rParameterDescriptor);
}

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	BasicType& rBasicType)
{

	DeepWalker::visit(rBasicType);
}

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	SubroutineType& rSubroutineType)
{

	DeepWalker::visit(rSubroutineType);
}

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	BasicLiteralExpression& rBasicLiteralExpression)
{

	DeepWalker::visit(rBasicLiteralExpression);
}

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	StrictLiteralExpression& rStrictLiteralExpression)
{

	DeepWalker::visit(rStrictLiteralExpression);
}

void OPS::Backends::Clang::Internal::ClangWalker::visit(
	RepriseVarDeclWrap& rRepriseVarDeclWrap)
{
	rRepriseVarDeclWrap.get().accept(*this);
}

//////////////////////////////////////////////////////////////////////
// ClangWalker private

clang::DeclContext*
OPS::Backends::Clang::Internal::ClangWalker::getClangParent(
	RepriseBase& rRepriseBase)
{
	RepriseBase* pRepriseBaseParent = rRepriseBase.getParent();
	R2CDeclNodes::const_iterator i = m_R2CDeclNodes.find(pRepriseBaseParent);
	OPS_ASSERT(i != m_R2CDeclNodes.end());

	clang::DeclContext* pDeclContext =
		clang::Decl::castToDeclContext(i->second);

	return pDeclContext;
}

void OPS::Backends::Clang::Internal::ClangWalker::putClangParent(
	OPS::Reprise::RepriseBase& rRepriseBase,
	clang::Decl* pClangDecl)
{
	m_R2CDeclNodes.insert(make_pair(&rRepriseBase, pClangDecl));
}

clang::QualType OPS::Backends::Clang::Internal::ClangWalker::getClangQualType(
	TypeBase& rType)
{
	return OPS::Backends::Clang::Internal::getClangQualType(
		*m_rGeneratedProgram.back(), rType, m_R2CDeclNodes);
}

// End of File
