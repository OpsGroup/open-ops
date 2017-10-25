#ifndef SUBROUTINES_SHARED_DEEP_WALKERS_H
#define SUBROUTINES_SHARED_DEEP_WALKERS_H

#include "Reprise/Service/DeepWalker.h"

#include <set>

namespace OPS
{
	namespace Shared
	{
		class FindSubroutinesByNameDeepWalker: public OPS::Reprise::Service::DeepWalker
		{
		public:
			typedef std::set<OPS::Reprise::SubroutineDeclaration*> SubroutinesDeclarationsContainer;

		public:
			FindSubroutinesByNameDeepWalker(std::string subroutineName, bool findOnlyWithImplementation): OPS::Reprise::Service::DeepWalker(),
				m_subroutineName(subroutineName),
				m_findOnlyWithImplementation(findOnlyWithImplementation),
				m_subroutinesDeclarations()
			{
			}

			SubroutinesDeclarationsContainer getSubroutinesDeclaration()
			{
				return m_subroutinesDeclarations;
			}

			void visit(OPS::Reprise::ProgramUnit& programUnit)
			{
				OPS::Reprise::Service::DeepWalker::visit(programUnit);
			}

			void visit(OPS::Reprise::TranslationUnit& translationUnit)
			{
				OPS::Reprise::Service::DeepWalker::visit(translationUnit);
			}

			void visit(OPS::Reprise::SubroutineDeclaration& subroutineDeclaration)
			{
				if((!m_findOnlyWithImplementation || subroutineDeclaration.hasImplementation()) && subroutineDeclaration.getName() == m_subroutineName)
				{
					m_subroutinesDeclarations.insert(&subroutineDeclaration);
				}
			}

		private:
			std::string                      m_subroutineName;
			bool                             m_findOnlyWithImplementation;
			SubroutinesDeclarationsContainer m_subroutinesDeclarations;
		};

		class FindTypesDeclarationsByNameDeepWalker: public OPS::Reprise::Service::DeepWalker
		{
		public:
			typedef std::set<OPS::Reprise::TypeDeclaration*> TypesDeclarationsContainer;
			typedef std::set<OPS::Reprise::TypeBase*> TypesContainer;

		public:
			FindTypesDeclarationsByNameDeepWalker(std::string typeName): OPS::Reprise::Service::DeepWalker(),
				m_typeName(typeName),
				m_typesDeclarations(),
				m_types()
			{
			}

			TypesDeclarationsContainer getSubroutinesDeclaration()
			{
				return m_typesDeclarations;
			}

			TypesContainer getTypes()
			{
				return m_types;
			}

			void visit(OPS::Reprise::TranslationUnit& translationUnit)
			{
				OPS::Reprise::Service::DeepWalker::visit(translationUnit);
			}

			void visit(OPS::Reprise::DeclaredType& declaredType)
			{
				if(declaredType.getDeclaration().getName() == m_typeName)
				{
					m_types.insert(&declaredType);
				}
			}

			void visit(OPS::Reprise::TypeDeclaration& typeDeclaration)
			{
				if(typeDeclaration.getName() == m_typeName)
				{
					m_typesDeclarations.insert(&typeDeclaration);
				}
			}

		private:
			std::string                m_typeName;
			TypesDeclarationsContainer m_typesDeclarations;
			TypesContainer             m_types;
		};
	}
}

#endif
