#include "Analysis/DeadDeclarations.h"
#include "Reprise/Service/DeepWalker.h"

using namespace OPS::Analysis;
using namespace OPS::Reprise;

namespace
{

class UsedDeclarationsWalker : public OPS::Reprise::Service::DeepWalker
{
public:
	void visit(SubroutineDeclaration& subDecl)
	{
        visitDecl(subDecl);
		if (subDecl.hasDefinition())
			visitDecl(subDecl.getDefinition());
		DeepWalker::visit(subDecl);
	}

	void visit(SubroutineReferenceExpression& subRef)
	{
		visitDecl(subRef.getReference());
	}

	void visit(DeclaredType& typeRef)
	{
		visitDecl(typeRef.getDeclaration());
	}

	void visit(ReferenceExpression& varRef)
	{
        VariableDeclaration& varDecl = varRef.getReference();
        visitDecl(varDecl);
        if (varDecl.hasDefinition())
            visitDecl(varDecl.getDefinition());
	}

	void visit(EnumAccessExpression& enumAccess)
	{
		RepriseBase* parent = enumAccess.getMember().getEnum().getParent();

		while(parent != 0)
		{
			if (TypeDeclaration* typeDecl = parent->cast_ptr<TypeDeclaration>())
			{
				visitDecl(*typeDecl);
				break;
			}
			parent = parent->getParent();
		}
	}

	void visitDecl(DeclarationBase& decl)
	{
		if (usedDeclarations.find(&decl) == usedDeclarations.end())
		{
			usedDeclarations.insert(&decl);
			decl.accept(*this);
		}
	}

	std::set<OPS::Reprise::DeclarationBase*> usedDeclarations;
};

}

void OPS::Analysis::obtainUsedDeclarations(const std::list<OPS::Reprise::DeclarationBase*>& rootDeclarations,
							std::set<OPS::Reprise::DeclarationBase*>& usedDeclarations)
{
	UsedDeclarationsWalker walker;

	std::list<OPS::Reprise::DeclarationBase*>::const_iterator it = rootDeclarations.begin();
	for(; it != rootDeclarations.end(); ++it)
	{
		walker.usedDeclarations.insert(*it);
		(*it)->accept(walker);
	}

	usedDeclarations.swap(walker.usedDeclarations);
}

void OPS::Analysis::obtainUsedDeclarations(DeclarationBase *rootDeclaration, std::set<DeclarationBase *> &usedDeclarations)
{
    std::list<OPS::Reprise::DeclarationBase*> rootDeclarations;
    rootDeclarations.push_back(rootDeclaration);
    obtainUsedDeclarations(rootDeclarations, usedDeclarations);
}
