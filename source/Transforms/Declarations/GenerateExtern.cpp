/// GenerateExtern.cpp
///   Generate necessary extern declarations if any declarations reference
/// another translation unit.
/// Author: Denis Dubrov (dubrov@sfedu.ru)
/// Created: 9.03.2013

#include "Transforms/Declarations/GenerateExtern.h"

#include "Reprise/Service/DeepWalker.h"
#include "Reprise/Expressions.h"

#include <set>
#include <map>

using namespace OPS::Reprise;
using namespace std;

namespace
{
	typedef ReprisePtr <TypeDeclaration>
		TypeDeclarationPtr;

	typedef ReprisePtr <VariableDeclaration>
		VariableDeclarationPtr;

	typedef ReprisePtr <SubroutineDeclaration>
		SubroutineDeclarationPtr;

	bool areOfDifferentTranslationUnits(
		const RepriseBase& rRepriseNode1,
		const RepriseBase& rRepriseNode2)
	{
		const TranslationUnit* pcUnit1 =
			rRepriseNode1.findTranslationUnit();
		const TranslationUnit* pcUnit2 =
			rRepriseNode2.findTranslationUnit();
		const bool cbDifferent =
			(pcUnit1 != pcUnit2);

		return cbDifferent;
	}

	class Walker : public Service::DeepWalker
	{
	// Overrides
	public:
		//
		virtual void visit(
			ProgramUnit& rProgramUnit);
		//
		virtual void visit(
			TranslationUnit& rTranslationUnit);
		//
		virtual void visit(
			VariableDeclaration& rVariableDeclaration);
		//
		virtual void visit(
			TypeDeclaration& rTypeDeclaration);
		//
		virtual void visit(
			SubroutineDeclaration& rSubroutineDeclaration);
		//
		virtual void visit(
			DeclaredType& rDeclaredType);
		//
		virtual void visit(
			ReferenceExpression& rReferenceExpression);
		//
		virtual void visit(
			SubroutineReferenceExpression& rSubroutineReferenceExpression);
		//
	private:
		//
		typedef map <RepriseBase*, RepriseBase*> NodeMap;
		typedef set <DeclarationBase*> DeclSet;
		//
		NodeMap m_NodeMap;
		DeclSet m_SeenDeclarations;
		//
		bool isNotSeen(DeclarationBase& rDeclarationBase) const;
		//
		static Declarations::Iterator findTopDeclaration(
			RepriseBase& rCode, Declarations& rGlobals);
	};
}    // namespace

//////////////////////////////////////////////////////////////////////
// Walker overrides

void Walker::visit(
	ProgramUnit& rProgramUnit)
{
	DeepWalker::visit(rProgramUnit);
}

void Walker::visit(
	TranslationUnit& rTranslationUnit)
{
	m_SeenDeclarations.clear();

	DeepWalker::visit(rTranslationUnit);
}

void Walker::visit(
	VariableDeclaration& rVariableDeclaration)
{
	m_SeenDeclarations.insert(&rVariableDeclaration);

	DeepWalker::visit(rVariableDeclaration);
}

void Walker::visit(
	TypeDeclaration& rTypeDeclaration)
{
	m_SeenDeclarations.insert(&rTypeDeclaration);

	DeepWalker::visit(rTypeDeclaration);
}

void Walker::visit(
	SubroutineDeclaration& rSubroutineDeclaration)
{
	m_SeenDeclarations.insert(&rSubroutineDeclaration);

	DeepWalker::visit(rSubroutineDeclaration);
}

void Walker::visit(
	DeclaredType& rDeclaredType)
{
	TypeDeclaration& rDeclaration =
		rDeclaredType.getDeclaration();
	const bool cbDifferent = areOfDifferentTranslationUnits(
		rDeclaredType, rDeclaration);
	if (cbDifferent || isNotSeen(rDeclaration))
	{
		NodeMap::iterator i = m_NodeMap.find(&rDeclaration);
		if (i != m_NodeMap.end())
		{
			rDeclaredType.setDeclaration(
				&i->second->cast_to <TypeDeclaration> ());
		}
		else
		{
			TypeDeclarationPtr declarationPtr;

			TranslationUnit* pUnit =
				rDeclaredType.findTranslationUnit();
			OPS_ASSERT(0 != pUnit);

			Declarations& rDeclarations = pUnit->getGlobals();

			if (cbDifferent)
			{
				// Cloning type from different translation unit

				declarationPtr.reset(rDeclaration.clone());

				m_NodeMap.insert(make_pair(&rDeclaration, declarationPtr.get()));

				rDeclaredType.setDeclaration(declarationPtr.get());
			}
			else
			{
				// Moving type from the bottom of the current translation unit

				declarationPtr.reset(&rDeclaration);

				Declarations::Iterator i =
					rDeclarations.convertToIterator(declarationPtr.get());
				rDeclarations.erase(i);
			}

			TypeDeclaration* pDeclaration = declarationPtr.release();
			rDeclarations.addBefore(
				findTopDeclaration(rDeclaredType, rDeclarations),
				pDeclaration);

			pDeclaration->accept(*this);
		}
	}

	DeepWalker::visit(rDeclaredType);
}

void Walker::visit(
	ReferenceExpression& rReferenceExpression)
{
	VariableDeclaration& rDeclaration =
		rReferenceExpression.getReference();
	const bool cbDifferent = areOfDifferentTranslationUnits(
		rReferenceExpression, rDeclaration);
	if (cbDifferent || isNotSeen(rDeclaration))
	{
		NodeMap::iterator i = m_NodeMap.find(&rDeclaration);
		if (i != m_NodeMap.end())
		{
			rReferenceExpression.setReference(
				&i->second->cast_to <VariableDeclaration> ());
		}
		else
		{
			VariableDeclarationPtr declarationPtr;
			TranslationUnit* pUnit =
				rReferenceExpression.findTranslationUnit();
			OPS_ASSERT(0 != pUnit);

			Declarations& rDeclarations = pUnit->getGlobals();

			if (cbDifferent)
			{
				// Generating extern declaration for the variable from
				//   another translation unit

				rDeclaration.declarators().set(
					VariableDeclarators::DECL_EXTERN);

				declarationPtr.reset(
					rDeclaration.clone());
				declarationPtr->detachInitExpression();

				m_NodeMap.insert(make_pair(&rDeclaration, declarationPtr.get()));
			}
			else
			{
				// Moving variable declaration from the bottom of the current
				//   translation unit

				declarationPtr.reset(&rDeclaration);

				Declarations::Iterator i =
					rDeclarations.convertToIterator(declarationPtr.get());
				rDeclarations.erase(i);
			}

			VariableDeclaration* pDeclaration = declarationPtr.release();
			rDeclarations.addBefore(
				findTopDeclaration(rReferenceExpression, rDeclarations),
				pDeclaration);

			pDeclaration->accept(*this);
		}
	}

	DeepWalker::visit(rReferenceExpression);
}

void Walker::visit(
	SubroutineReferenceExpression& rSubroutineReferenceExpression)
{
	SubroutineDeclaration& rDeclaration =
		rSubroutineReferenceExpression.getReference();
	const bool cbDifferent = areOfDifferentTranslationUnits(
		rSubroutineReferenceExpression, rDeclaration);
	if (cbDifferent || isNotSeen(rDeclaration))
	{
		NodeMap::iterator i = m_NodeMap.find(&rDeclaration);
		if (i != m_NodeMap.end())
		{
			rSubroutineReferenceExpression.setReference(
				&i->second->cast_to <SubroutineDeclaration> ());
		}
		else
		{
			if (cbDifferent)
				rDeclaration.declarators().set(
					VariableDeclarators::DECL_EXTERN);

			SubroutineDeclarationPtr declarationPtr(
				rDeclaration.clone());
			declarationPtr->setPrototype();

			m_NodeMap.insert(make_pair(&rDeclaration, declarationPtr.get()));

			TranslationUnit* pUnit =
				rSubroutineReferenceExpression.findTranslationUnit();
			OPS_ASSERT(0 != pUnit);

			SubroutineDeclaration* pDeclaration = declarationPtr.release();
			Declarations::Iterator topDeclaration = findTopDeclaration(
				rSubroutineReferenceExpression, pUnit->getGlobals());
			pUnit->getGlobals().addBefore(topDeclaration, pDeclaration);

			pDeclaration->accept(*this);
		}
	}    // if (cbDifferent)

	DeepWalker::visit(rSubroutineReferenceExpression);
}

//////////////////////////////////////////////////////////////////////
// Walker private

bool Walker::isNotSeen(DeclarationBase& rDeclarationBase) const
{
	// Filtering local declarations

	const TranslationUnit* pcTranslationUnit =
		rDeclarationBase.findTranslationUnit();
	const RepriseBase* pcParent = rDeclarationBase.getParent();

	if (0 == pcTranslationUnit ||
		pcParent != &pcTranslationUnit->getGlobals())
		return false;

	return
		(m_SeenDeclarations.find(&rDeclarationBase) ==
		m_SeenDeclarations.end());
}

Declarations::Iterator Walker::findTopDeclaration(
	RepriseBase &rCode, Declarations &rGlobals)
{
	RepriseBase* pCurrent = &rCode;
	while (pCurrent != 0 && pCurrent->getParent() != &rGlobals)
		pCurrent = pCurrent->getParent();

		OPS_ASSERT(pCurrent != 0);

		return rGlobals.convertToIterator(
			&pCurrent->cast_to <DeclarationBase> ());
}

//////////////////////////////////////////////////////////////////////
// generateExtern()

void OPS::Transforms::Declarations::generateExtern(
	ProgramUnit& rProgramUnit)
{
	Walker walker;
	walker.visit(rProgramUnit);
}

// End of File
