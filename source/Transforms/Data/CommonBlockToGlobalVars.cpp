#include "Transforms/Data/CommonBlockToGlobalVars.h"
#include "Reprise/Service/DeepWalker.h"
#include "Reprise/ServiceFunctions.h"
#include <string>

namespace OPS
{
namespace Transforms
{

using namespace OPS::Reprise;

static bool isCommonBlockVariable(VariableDeclaration& variable)
{
	if (!variable.getType().is_a<DeclaredType>())
		return false;

	TypeDeclaration& varTypeDecl = variable.getType().cast_to<DeclaredType>().getDeclaration();

	if (!varTypeDecl.getType().is_a<StructType>())
		return false;

	StructType& commonBlockType = varTypeDecl.getType().cast_to<StructType>();

	if (!commonBlockType.isUnion() ||
		 commonBlockType.getMemberCount() == 0 ||
		 !commonBlockType.getMember(0).getType().is_a<StructType>())
		return false;

	return true;
}

static bool isCommonBlockConvertable(const StructType& block)
{
	// Во всех подпрограммах структура блока должна быть одной и той-же
	OPS_ASSERT(block.getMemberCount() > 0);

	const StructMemberDescriptor& firstMember = block.getMember(0);

	for(int i = 1; i < block.getMemberCount(); ++i)
	{
		if (!block.getMember(i).getType().isEqual(firstMember.getType()))
			return false;
	}
	return true;
}

class MemberAccessReplacer : public OPS::Reprise::Service::DeepWalker
{
public:

	typedef std::map<StructMemberDescriptor*, VariableDeclaration*> MemberToVarMap;

	MemberAccessReplacer(const MemberToVarMap& memberToVar)
		:m_m2v(memberToVar)
	{
	}

	void visit(Reprise::StructAccessExpression& expr)
	{
		MemberToVarMap::const_iterator it = m_m2v.find(&expr.getMember());
		if (it != m_m2v.end())
		{
			ReprisePtr<ExpressionBase> destExpr(new ReferenceExpression(*it->second));
			Editing::replaceExpression(expr, destExpr);
		}
	}

private:
	const MemberToVarMap& m_m2v;
};

bool convertCommonBlockToGlobalVariables(Reprise::VariableDeclaration& commonBlockVariable)
{
	// Проверить, что переменная описывает common блок
	if (!isCommonBlockVariable(commonBlockVariable))
		return false;

	TypeDeclaration& commonBlockTypeDecl = commonBlockVariable.getType().cast_to<DeclaredType>().getDeclaration();
	StructType& commonBlockType = commonBlockTypeDecl.getType().cast_to<StructType>();
	// Проверить, что можно сконвертировать
	if (!isCommonBlockConvertable(commonBlockType))
		return false;

	// Создать глобальные переменные
	Declarations& globals = commonBlockVariable.findTranslationUnit()->getGlobals();

	MemberAccessReplacer::MemberToVarMap m2v;

	std::vector<StructType*> blocks;

	for(int i = 0; i < commonBlockType.getMemberCount(); ++i)
		blocks.push_back(&commonBlockType.getMember(i).getType().cast_to<StructType>());

	for(int i = 0; i < blocks[0]->getMemberCount(); ++i)
	{
		StructMemberDescriptor& member = blocks[0]->getMember(i);
		VariableDeclaration* var = new VariableDeclaration(member.getType().clone(), member.getName());
		globals.addVariable(var);

		for(size_t blk = 0; blk < blocks.size(); ++blk)
		{
			m2v[&blocks[blk]->getMember(i)] = var;
		}
	}

	// Заменить везде вхождения на переменные
	MemberAccessReplacer replacer(m2v);
	globals.accept(replacer);

	// Удалить переменную блока и тип блока
	OPS_ASSERT(commonBlockVariable.getParent() == &globals);
	Declarations::Iterator itCommonBlockVar = globals.convertToIterator(&commonBlockVariable);
	globals.erase(itCommonBlockVar);

	OPS_ASSERT(commonBlockTypeDecl.getParent() == &globals);
	Declarations::Iterator itCommonBlockTypeDecl = globals.convertToIterator(&commonBlockTypeDecl);
	globals.erase(itCommonBlockTypeDecl);

	return false;
}

class CommonBlockCollector : public OPS::Reprise::Service::DeepWalker
{
public:
	std::vector<VariableDeclaration*> commonBlocks;

	void visit(VariableDeclaration& varDecl)
	{
		if (varDecl.getName().find("VARIABLE") != std::string::npos &&
			isCommonBlockVariable(varDecl))
		{
			commonBlocks.push_back(&varDecl);
		}
	}
};

/// Convert all common blocks declared in fragment
void convertAllCommonBlocksToGlobalVariables(Reprise::RepriseBase& node)
{
	CommonBlockCollector collector;
	node.accept(collector);

	for(size_t i = 0; i < collector.commonBlocks.size(); ++i)
	{
		convertCommonBlockToGlobalVariables(*collector.commonBlocks[i]);
	}
}

}
}
