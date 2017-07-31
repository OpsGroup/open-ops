#include "Shared/RepriseClone.h"
#include "Shared/ReprisePath.h"

#include "Reprise/Service/Service.h"

namespace OPS
{
namespace Shared
{

using namespace OPS::Reprise;

static bool isChildOf(const OPS::Reprise::RepriseBase* child, const OPS::Reprise::RepriseBase* parent)
{
	const OPS::Reprise::RepriseBase* current = child;
	while(current != 0 && current != parent)
		current = current->getParent();

	return current == parent;
}

class LinksClonner : public Service::DeepWalker
{
public:
	using Service::DeepWalker::visit;
	inline  LinksClonner(const OPS::Reprise::RepriseBase* oldRoot, OPS::Reprise::RepriseBase* newRoot) 
		: m_oldRoot(oldRoot), m_newRoot(newRoot)
	{
	}

	inline void visit(VariableDeclaration& varDecl)
	{
		if (varDecl.hasParameterReference() && isChildOf(&varDecl.getParameterReference(), m_oldRoot))
		{
            RepriseBase* newParamRef = findNewNode(varDecl.getParameterReference());
			varDecl.setParameterReference(newParamRef->cast_to<ParameterDescriptor>());
		}
		if (varDecl.hasDefinedBlock() && isChildOf(&varDecl.getDefinedBlock(), m_oldRoot))
		{
            RepriseBase* newDefinedBlock = findNewNode(varDecl.getDefinedBlock());
            varDecl.setDefinedBlock(newDefinedBlock->cast_to<BlockStatement>());
		}
        if (varDecl.hasDefinition() && isChildOf(&varDecl.getDefinition(), m_oldRoot))
        {
            RepriseBase* newDefinition = findNewNode(varDecl.getDefinition());
            varDecl.setDefinition(newDefinition->cast_to<VariableDeclaration>());
        }
		DeepWalker::visit(varDecl);
	}

    inline void visit(SubroutineDeclaration& subroutine)
    {
		if (subroutine.hasDefinition() && isChildOf(&subroutine.getDefinition(), m_oldRoot))
        {
            RepriseBase* newDefinition = findNewNode(subroutine.getDefinition());
            subroutine.setDefinition(newDefinition->cast_to<SubroutineDeclaration>());
        }
		DeepWalker::visit(subroutine);
    }

	inline void visit(DeclaredType& declared)
	{
		if (isChildOf(&declared.getDeclaration(), m_oldRoot))
		{
            RepriseBase* newTypeDecl = findNewNode(declared.getDeclaration());
			declared.setDeclaration(newTypeDecl->cast_ptr<TypeDeclaration>());
		}
	}

	inline void visit(PlainCaseLabel& caseLabel)
	{
		if (isChildOf(&caseLabel.getStatement(), m_oldRoot))
		{
            RepriseBase* newParamRef = findNewNode(caseLabel.getStatement());
			caseLabel.setStatement(newParamRef->cast_ptr<StatementBase>());
		}
	}

	inline void visit(GotoStatement& gotoStmt)
	{
		if (gotoStmt.getPointedStatement() != 0 && isChildOf(gotoStmt.getPointedStatement(), m_oldRoot))
		{
            RepriseBase* newParamRef = findNewNode(*gotoStmt.getPointedStatement());
			gotoStmt.setPointedStatement(newParamRef->cast_ptr<StatementBase>());
		}
	}

	inline void visit(ReferenceExpression& refExpr)
	{
		if (isChildOf(&refExpr.getReference(), m_oldRoot))
		{
            RepriseBase* newParamRef = findNewNode(refExpr.getReference());
			refExpr.setReference(newParamRef->cast_ptr<VariableDeclaration>());
		}
	}

	inline void visit(SubroutineReferenceExpression& refExpr)
	{
		if (isChildOf(&refExpr.getReference(), m_oldRoot))
		{
            RepriseBase* newParamRef = findNewNode(refExpr.getReference());
			refExpr.setReference(newParamRef->cast_ptr<SubroutineDeclaration>());
		}
	}

	inline void visit(StructAccessExpression& structAccess)
	{
		if (isChildOf(&structAccess.getMember(), m_oldRoot))
		{
            RepriseBase* newParamRef = findNewNode(structAccess.getMember());
			structAccess.setMember(newParamRef->cast_ptr<StructMemberDescriptor>());
		}
		DeepWalker::visit(structAccess);
	}

	inline void visit(EnumAccessExpression& enumAccess)
	{
		if (isChildOf(&enumAccess.getMember(), m_oldRoot))
		{
            RepriseBase* newParamRef = findNewNode(enumAccess.getMember());
			enumAccess.setMember(newParamRef->cast_ptr<EnumMemberDescriptor>());
		}
	}

private:

    typedef std::map<RepriseBase*, RepriseBase*> OldToNewMap;

    RepriseBase* findNewNode(RepriseBase& node)
    {
        OldToNewMap::const_iterator it = m_oldToNew.find(&node);
        if (it != m_oldToNew.end())
        {
            return it->second;
        }
        else
        {
            RepriseBase* newNode = findByPath(*m_newRoot, makePath(node, m_oldRoot));
            m_oldToNew.insert(std::make_pair(&node, newNode));
            return newNode;
        }
    }

	const OPS::Reprise::RepriseBase* m_oldRoot;
	OPS::Reprise::RepriseBase* m_newRoot;
    OldToNewMap m_oldToNew;
};

ReprisePtr<ProgramUnit> deepCloneProgramUnit(ProgramUnit& unit)
{
	ReprisePtr<ProgramUnit> result(unit.clone());
	doPostCloneLinkFix(unit, *result);
	return result;
}

ReprisePtr<TranslationUnit> deepCloneTranslationUnit(TranslationUnit& unit)
{
	ReprisePtr<TranslationUnit> result(unit.clone());
	doPostCloneLinkFix(unit, *result);
	return result;
}

void doPostCloneLinkFix(const OPS::Reprise::RepriseBase &original, OPS::Reprise::RepriseBase &clone)
{
	LinksClonner clonner(&original, &clone);
	clone.accept(clonner);
}

static void obtainInnerVariables(const OPS::Reprise::StatementBase& stmt,
								 const OPS::Reprise::Declarations& declarations,
								 std::vector<const VariableDeclaration*>& variables)
{
	Declarations::ConstVarIterator it = declarations.getFirstVar();
	for(; it.isValid(); ++it)
	{
		const VariableDeclaration& var = *it;
		if (var.hasDefinedBlock())
		{
			const BlockStatement& definedBlock = var.getDefinedBlock();
			if (isChildOf(&definedBlock, &stmt))
			{
				variables.push_back(&var);
			}
		}
	}
}

class VariableReferenceMapper : public Service::DeepWalker
{
public:
	typedef std::map<const VariableDeclaration*, VariableDeclaration*> DeclarationMap;

	VariableReferenceMapper(const DeclarationMap& declMap)
		:m_declMap(declMap) {}

	void visit(ReferenceExpression& refExpr)
	{
		DeclarationMap::const_iterator it = m_declMap.find(&refExpr.getReference());
		if (it != m_declMap.end())
			refExpr.setReference(it->second);
	}

private:
	const DeclarationMap& m_declMap;
};

static void cloneInnerVariables(const OPS::Reprise::StatementBase& original, 
								OPS::Reprise::StatementBase& clone,
								const OPS::Reprise::Declarations& sourceDeclarations,
								OPS::Reprise::Declarations& destDeclarations)
{
	// Собрать все внутренние переменные из блоков в original
	std::vector<const VariableDeclaration*> originalInnerVars;
	obtainInnerVariables(original, sourceDeclarations, originalInnerVars);

	VariableReferenceMapper::DeclarationMap oldToNewMap;

	// Сгенерировать соответствующие переменные в clone
	for(size_t iVar = 0; iVar < originalInnerVars.size(); ++iVar)
	{
		VariableDeclaration* cloneVar = originalInnerVars[iVar]->clone();
		destDeclarations.addLast(cloneVar);
		oldToNewMap[originalInnerVars[iVar]] = cloneVar;

		RepriseBase* newDefinedBlock = findByPath(clone, makePath(originalInnerVars[iVar]->getDefinedBlock(), &original));
		cloneVar->setDefinedBlock(*newDefinedBlock->cast_ptr<BlockStatement>());
	}

	// Исправить все ссылки в clone
	VariableReferenceMapper refMapper(oldToNewMap);
	clone.accept(refMapper);
}

OPS::Reprise::ReprisePtr<OPS::Reprise::StatementBase> cloneStatement(
			const OPS::Reprise::StatementBase& stmt,
			const OPS::Reprise::Declarations& sourceDeclarations,
			OPS::Reprise::Declarations& destDeclarations
			)
{
	ReprisePtr<StatementBase> result(stmt.clone());
	doPostCloneLinkFix(stmt, *result);
	cloneInnerVariables(stmt, *result, sourceDeclarations, destDeclarations);
	return result;
}

}
}
