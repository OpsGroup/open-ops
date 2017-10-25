#ifndef MY_BAAD_PARAMETERS_H
#define MY_BAAD_PARAMETERS_H

#include "Reprise/Reprise.h"

namespace OPS
{
namespace Transforms
{
namespace DataDistribution
{
using OPS::Reprise::ExpressionBase;

class BDParameters
{
public:
	//class IntegerParameter
	//{
	//public:		
	//	IntegerParameter(ExpressionBase* expr = 0);

	//	int calcValue();
	//	bool isCalculated();

	//	OPS::Reprise::ReprisePtr<ExpressionBase> value();
	//	void setValue(ExpressionBase* expr);
	//private:
	//	OPS::Reprise::ReprisePtr<ExpressionBase> m_expr;
	//};
	
	OPS::Reprise::StatementBase* firstDistrStmt;
	OPS::Reprise::StatementBase* lastDistrStmt;

	OPS::Reprise::VariableDeclaration* oldArrayDecl;
	OPS::Reprise::VariableDeclaration* newArrayDecl;
	
	std::vector<OPS::Reprise::VariableDeclaration*> d;
	std::vector<OPS::Reprise::VariableDeclaration*> dims;

	int d_value(int index);
	int dim_value(int index);
	int blocks_count(int index);
	// throws exception is pragma params are wrong
	int size();
	long newArraySize();
	static std::list<BDParameters> findAllPragmas(OPS::Reprise::SubroutineDeclaration* func);	
private:	
	static void parsePragma(std::list<BDParameters>& pragmasParsedBefore, OPS::Reprise::StatementBase* stmt);
	static BDParameters processNextOpsDistributeDataPragma(OPS::Reprise::BlockStatement::Iterator& blockIt, std::string dataValue);
	static void parseOpsDistributeDataPragmaParams(OPS::Reprise::BlockStatement& body, std::list<std::string> pragmaParams, BDParameters& bdParameters);
	static OPS::Reprise::VariableDeclaration* findVariableByName(OPS::Reprise::BlockStatement& innerBlock, std::string name);	
	static std::list<std::string> getPragmaParams(std::string pragmaValue);
	static int getArrayDimension(OPS::Reprise::TypeBase* type);
};

bool isDistibutedArray(OPS::Reprise::VariableDeclaration* reference, std::list<BDParameters>& pragmas);
OPS::Reprise::ExpressionBase* getBlockSizeVariableCoefficient(OPS::Reprise::BasicCallExpression* bckCall, int dimIndex, std::list<BDParameters>& pragmas);

}
}
}

#endif
