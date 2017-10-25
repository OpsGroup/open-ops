/// RepriseClangifier.h
/// Created: 6.05.2016

#ifndef REPRISE_CLANGIFIER_H__
#define REPRISE_CLANGIFIER_H__

#include "Reprise/Service/DeepWalker.h"

namespace OPS
{
	namespace Backends
	{
		namespace Clang
		{
			namespace Internal
			{
				class RepriseClangifier;
				class RepriseVarDeclWrap;

				class RepriseClangifier : public Reprise::Service::DeepWalker
				{
				public:
					//
					RepriseClangifier();
					//
				// Overrides
				public:
					//
					virtual void visit(
						OPS::Reprise::Declarations& rDeclarations);
					//
				private:
					//
				};	// class RepriseClangifier

				// Simple VariableDeclaration to Statement wrapper
				// to insert VarDecls into defined blocks
				class RepriseVarDeclWrap : public Reprise::StatementBase
				{
				public:
					explicit RepriseVarDeclWrap(OPS::Reprise::VariableDeclaration* pVariableDeclaration);

					const OPS::Reprise::VariableDeclaration& get(void) const;
					OPS::Reprise::VariableDeclaration& get(void);

					void set(OPS::Reprise::VariableDeclaration* const pVariableDeclaration);

					virtual void replaceExpression(OPS::Reprise::ExpressionBase& source,
												   OPS::Reprise::ReprisePtr<OPS::Reprise::ExpressionBase> destination);

				//		RepriseBase implementation
					virtual int getChildCount(void) const;
					virtual RepriseBase& getChild(int index);

					virtual std::string dumpState(void) const;

					OPS_DEFINE_VISITABLE()
					OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
					OPS_DEFINE_CLONABLE_INTERFACE(RepriseVarDeclWrap)
				protected:
					RepriseVarDeclWrap(const RepriseVarDeclWrap& other);

				private:
					OPS::Reprise::ReprisePtr<OPS::Reprise::VariableDeclaration> m_varDecl;
				};
			}	// namespace Internal
		}	// namespace Clang
	}	// namespace Backends
}	// namespace OPS

#endif	// REPRISE_CLANGIFIER_H__
