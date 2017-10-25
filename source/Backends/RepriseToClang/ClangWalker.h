/// ClangWalker.h
/// Created: 19.02.2013

#ifndef CLANG_WALKER_H__
#define CLANG_WALKER_H__

#include "Backends/RepriseToClang/ClangGenerator.h"

#include "Reprise/Service/DeepWalker.h"
#include "R2CDeclNodes.h"
#include "RepriseClangifier.h"

#include <stack>

namespace OPS
{
	namespace Backends
	{
		namespace Clang
		{
			namespace Internal
			{
				class ClangWalker : public Reprise::Service::DeepWalker
				{
				public:
					//
					ClangWalker(
						ASTContexts& rGeneratedProgram,
						ASTContextParams& rASTContextParams);
					//
				// Overrides
				public:
					//
					virtual void visit(
						OPS::Reprise::ProgramUnit& rProgramUnit);
					//
					virtual void visit(
						OPS::Reprise::TranslationUnit& rTranslationUnit);
					//
					virtual void visit(
						OPS::Reprise::Declarations& rDeclarations);
					//
					virtual void visit(
						OPS::Reprise::VariableDeclaration& rVariableDeclaration);
					//
					virtual void visit(
						OPS::Reprise::TypeDeclaration& rTypeDeclaration);
					//
					virtual void visit(
						OPS::Reprise::SubroutineDeclaration& rSubroutineDeclaration);
					//
					virtual void visit(
						OPS::Reprise::StructMemberDescriptor& rStructMemberDescriptor);
					//
					virtual void visit(
						OPS::Reprise::BlockStatement& rBlockStatement);
					//
					virtual void visit(
						OPS::Reprise::ForStatement& rForStatement);
					//
					virtual void visit(
						OPS::Reprise::ExpressionStatement& rExpressionStatement);
					//
					virtual void visit(
						OPS::Reprise::ParameterDescriptor& rParameterDescriptor);
					//
					virtual void visit(
						OPS::Reprise::BasicType& rBasicType);
					//
					virtual void visit(
						OPS::Reprise::SubroutineType& rSubroutineType);
					//
					virtual void visit(
						OPS::Reprise::BasicLiteralExpression& rBasicLiteralExpression);
					//
					virtual void visit(
						OPS::Reprise::StrictLiteralExpression& rStrictLiteralExpression);
					//
					virtual void visit(
						OPS::Backends::Clang::Internal::RepriseVarDeclWrap& rRepriseVarDeclWrap);
					//
				private:
					//
					typedef std::stack <clang::Decl*>
						DeclContexts;
					//
					ASTContexts& m_rGeneratedProgram;
					ASTContextParams& m_rASTContextParams;
					R2CDeclNodes m_R2CDeclNodes;
					DeclContexts m_DeclContexts;
					//
					clang::DeclContext* getClangParent(
						OPS::Reprise::RepriseBase& rRepriseBase);
					//
					void putClangParent(
						OPS::Reprise::RepriseBase& rRepriseBase,
						clang::Decl* pClangDecl);
					//
					clang::QualType getClangQualType(
						OPS::Reprise::TypeBase& rType);
					//
				};    // class ClangWalker
			}    // namespace Internal
		}    // namespace Clang
	}    // namespace Backends
}    // namespace OPS

#endif    // CLANG_WALKER_H__

// End of File
