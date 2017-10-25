//		STL headers
#include <math.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cassert>

#include "Reprise/Reprise.h"
#include "Shared/SubroutinesShared.h"
#include "Shared/ExpressionHelpers.h"

#include "Reprise/Canto/HirFTypes.h"
#include "Shared/SubroutinesShared.h"
#include "Shared/DataShared.h"
#include "Shared/LinearExpressions.h"
#include "../ReferencesFinder.h"

#include "FrontTransforms/ExpressionSimplifier.h"

#include "Transforms/DataDistribution/Shared/BDParameters.h"
#include "Transforms/DataDistribution/Shared/BDExceptions.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{
using namespace std;
using namespace OPS::Shared;
using namespace OPS::Reprise;
using namespace OPS::Reprise::Canto;
using namespace OPS::Transforms;
using namespace OPS::Shared::ExpressionHelpers;

class PragmasFinder : public Service::DeepWalker
{
public:
	using Service::DeepWalker::visit;
	typedef list<StatementBase*> NodeList;
	NodeList m_nodeList;

#define VISIT_STMT(StmtType) \
	void visit(StmtType& node) \
	{		\
		if (node.hasNote("distribute")) \
			m_nodeList.push_back(&node); \
		Service::DeepWalker::visit(node); \
	}


	VISIT_STMT(BlockStatement)
	VISIT_STMT(ForStatement)
	VISIT_STMT(WhileStatement)
	VISIT_STMT(IfStatement)
	VISIT_STMT(GotoStatement)
	VISIT_STMT(ReturnStatement)
    VISIT_STMT(Canto::HirBreakStatement)
    VISIT_STMT(Canto::HirContinueStatement)
	VISIT_STMT(ExpressionStatement)
	VISIT_STMT(EmptyStatement)
};
//
//BDParameters::IntegerParameter::IntegerParameter(ExpressionBase* expr)
//{
//	m_expr.reset(expr);
//}
//
//int BDParameters::IntegerParameter::calcValue()
//{
//	return -1;
//}
//bool BDParameters::IntegerParameter::isCalculated()
//{
//	return false;
//}
//OPS::Reprise::ReprisePtr<ExpressionBase> BDParameters::IntegerParameter::value()
//{	
//	return m_expr;
//}
//void BDParameters::IntegerParameter::setValue(ExpressionBase* expr)
//{
//	m_expr.reset(expr);
//}
//int BDParameters::size()
//{
//	return -1;
//}
//BDParameters::IntegerParameter BDParameters::blockVolume()
//{
//	return m_blockVolume;
//}
//std::vector<BDParameters::IntegerParameter>& BDParameters::d()
//{
//	return m_d;
//}
//std::vector<BDParameters::IntegerParameter>& BDParameters::dims()
//{
//	return m_dims;
//}

int BDParameters::d_value(int index)
{
	ExpressionBase& resExpr = d[index]->getInitExpression();
	ExpressionBase* simpExpr = OPS::ExpressionSimplifier::Simplifier().simplify(&resExpr);	
	if (simpExpr->cast_ptr<StrictLiteralExpression>() == 0)
		throw OPS::RuntimeError("Couldn't calculate init value for d parameter");
	return simpExpr->cast_ptr<StrictLiteralExpression>()->getInt32();
}
int BDParameters::dim_value(int index)
{
	ExpressionBase& resExpr = dims[index]->getInitExpression();
	ExpressionBase* simpExpr = OPS::ExpressionSimplifier::Simplifier().simplify(&resExpr);	
	if (simpExpr->cast_ptr<StrictLiteralExpression>() == 0)
		throw OPS::RuntimeError("Couldn't calculate init value for d parameter");
	return simpExpr->cast_ptr<StrictLiteralExpression>()->getInt32();
}

int BDParameters::blocks_count(int index)
{
	return ceill(((double)dim_value(index))/d_value(index));
}

int BDParameters::size()
{
	return d.size();
}

long BDParameters::newArraySize()
{
	long result = 1;
	for (int i = 0; i < size(); i++)
		result *= d_value(i) * blocks_count(i);
	return result;
}

std::list<BDParameters> BDParameters::findAllPragmas(OPS::Reprise::SubroutineDeclaration* func)
{	
	list<BDParameters> result;

	PragmasFinder visitor;
	func->accept(visitor);
	for (std::list<StatementBase*>::iterator it = visitor.m_nodeList.begin(); it != visitor.m_nodeList.end(); ++it)
	{
		parsePragma(result, *it);
	}

	return result;
}

bool getBlockIteratorByStatement(StatementBase* stmt, BlockStatement::Iterator& it)
{	
	BlockStatement& parentBlock = stmt->getParentBlock();
	BlockStatement::Iterator blockIt = parentBlock.getFirst();
	while (blockIt.isValid())
	{
		if (&*blockIt == stmt)
		{
			it =  blockIt;
			return true;
		}
	}

	return false;
}

void BDParameters::parsePragma(std::list<BDParameters>& pragmasParsedBefore, StatementBase* stmt)
{
	assert(stmt->hasParentBlock() == true);

	BlockStatement& parentBlock = stmt->getParentBlock();
	BlockStatement::Iterator blockIt = parentBlock.getFirst();
	while (blockIt.isValid())
	{
		if (&*blockIt == stmt)
		{
			BDParameters nextItem;			
			if (blockIt->hasNote("distribute") == false) 
				throw BDException("Stmt hasn't note 'distribute'");
			string pragmaValue = blockIt->getNote("distribute").getString();

			if (pragmaValue.find("data") != string::npos)
			{
				pragmaValue = pragmaValue + ";";
				int nextDataValueIndex = pragmaValue.find(";");
				while (nextDataValueIndex >= 0)
				{		
					nextItem = processNextOpsDistributeDataPragma(blockIt, pragmaValue.substr(0, nextDataValueIndex));
					nextItem.firstDistrStmt = stmt;				
					pragmasParsedBefore.push_back(nextItem);

					pragmaValue.erase(0, nextDataValueIndex + 1);	
					nextDataValueIndex = pragmaValue.find(";");
				}
			}

			if (pragmaValue.find("revert") != string::npos)
			{
				pragmaValue = pragmaValue + ";";
				int nextDataValueIndex = pragmaValue.find(";");
				while (nextDataValueIndex >= 0)
				{
					std::list<string> params = getPragmaParams(pragmaValue.substr(0, nextDataValueIndex));
					if (params.size() == 1)
					{
						string arrayName = *params.begin();
						std::list<BDParameters>::iterator it = pragmasParsedBefore.begin();
						while (it != pragmasParsedBefore.end())
						{
							if (it->oldArrayDecl->getName() == arrayName)
							{
								it->lastDistrStmt = stmt;
								break;
							}
							++it;
						}
					}

					pragmaValue.erase(0, nextDataValueIndex + 1);	
					nextDataValueIndex = pragmaValue.find(";");
				}
			}

			blockIt->removeNote("distribute");						
		}
		blockIt.goNext();
	}
}

BDParameters BDParameters::processNextOpsDistributeDataPragma(BlockStatement::Iterator& blockIt, string dataValue)
{
	BDParameters result;	
	parseOpsDistributeDataPragmaParams(blockIt->getParentBlock(),  getPragmaParams(dataValue), result);
	return result;
}

void BDParameters::parseOpsDistributeDataPragmaParams(BlockStatement& body, list<string> pragmaParams, BDParameters& bdParameters)
{
	int arrayDim;
	int expectedParamsCount;
	int nextParamIndex = 0;
	for (list<string>::iterator it = pragmaParams.begin(); it != pragmaParams.end(); ++it)
	{
		if (it == pragmaParams.begin())
		{
			//		1-й параметр - имя размещаемого массива
			bdParameters.oldArrayDecl = findVariableByName(body, *it);
			
			PtrType* newArrayType = new PtrType(OPS::Shared::getArrayElementBasicType(&bdParameters.oldArrayDecl->getType()));
			bdParameters.newArrayDecl = &Editing::createNewVariable(*newArrayType, body, *it);
            arrayDim = getTypeDimension(bdParameters.oldArrayDecl->getType());
			expectedParamsCount = 1 + 2*arrayDim;
			
			if (int(pragmaParams.size()) != expectedParamsCount)
				throw OPS::RuntimeError("Parameters count is wrong");
		}
		else
		{
			VariableDeclaration* nextVarDecl = findVariableByName(body, *it);		
			if (nextParamIndex < arrayDim + 1)
				bdParameters.dims.push_back(nextVarDecl);			
			else if (nextParamIndex < 2*arrayDim + 1)
				bdParameters.d.push_back(nextVarDecl);
		}
		nextParamIndex++;
	}	
}

VariableDeclaration* BDParameters::findVariableByName(BlockStatement& innerBlock, string name)
{		
	VariableDeclaration* variableDecl = innerBlock.getDeclarations().findVariable(name);
	if (variableDecl != 0)
		return variableDecl;
	if (innerBlock.hasParentBlock() == true)
		return findVariableByName(innerBlock.getParentBlock(), name);
	
	return innerBlock.findTranslationUnit()->getGlobals().findVariable(name);
}

std::list<string> BDParameters::getPragmaParams(string pragmaValue)
{
	int leftBracketPos = pragmaValue.find("(");
	int rightBracketPos = pragmaValue.find(")");
	std::list<string> pragmaParamsList;
	pragmaValue = pragmaValue.substr(leftBracketPos+1, rightBracketPos - leftBracketPos - 1) + ",";
	int nextCommaPos = pragmaValue.find(",");
	while (nextCommaPos != string::npos)
	{		
		pragmaParamsList.push_back(pragmaValue.substr(0, nextCommaPos));
		pragmaValue = pragmaValue.erase(0, nextCommaPos + 1);		
		nextCommaPos = pragmaValue.find(",");
	}

	return pragmaParamsList;
}

int BDParameters::getArrayDimension(TypeBase* type)
{	
	if (type != 0)
	{
		if (type->is_a<PtrType>())
			return getArrayDimension(&type->cast_to<PtrType>().getPointedType()) + 1;
		if (type->is_a<HirFArrayType>())
			return type->cast_to<HirFArrayType>().getShape().getRank();
		if (type->is_a<ArrayType>())
            return getTypeDimension(*type);
	}
	return 0;
}

bool isDistibutedArray(VariableDeclaration* reference, std::list<BDParameters>& pragmas)
{
	for (std::list<BDParameters>::iterator it = pragmas.begin(); it != pragmas.end(); ++it)
	{
		if (it->newArrayDecl == reference)
			return true;
	}
	return false;
}

}
}
}
