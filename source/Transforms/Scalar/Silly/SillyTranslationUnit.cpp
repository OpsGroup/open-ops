/// SillyTranslationUnit.cpp
///   Test transformation applied to the translation unit.
/// Author: Denis Dubrov (dubrov@sfedu.ru)
/// Created: 20.11.2013

#include "Transforms/Scalar/Silly/SillyTranslationUnit.h"

#include "Reprise/Service/DeepWalker.h"
#include "Reprise/Expressions.h"

using namespace OPS::Reprise;
using namespace std;

namespace
{
	class Walker : public Service::DeepWalker
	{
	// Overrides
	public:
		//
		virtual void visit(
			TranslationUnit& rTranslationUnit);
		//
		virtual void visit(
			VariableDeclaration& rVariableDeclaration);
		//
		virtual void visit(
			BasicCallExpression& rBasicCallExpression);
	};
}    // namespace

//////////////////////////////////////////////////////////////////////
// Walker overrides

void Walker::visit(
	TranslationUnit& rTranslationUnit)
{
	DeepWalker::visit(rTranslationUnit);
}

void Walker::visit(
	VariableDeclaration& rVariableDeclaration)
{

	DeepWalker::visit(rVariableDeclaration);
}

void Walker::visit(
	BasicCallExpression& rBasicCallExpression)
{
	switch (rBasicCallExpression.getKind())
	{
		case BasicCallExpression::BCK_BINARY_PLUS:
			//
			rBasicCallExpression.setKind(BasicCallExpression::BCK_BINARY_MINUS);
			//
			break;
			//
		case BasicCallExpression::BCK_BINARY_MINUS:
			//
			rBasicCallExpression.setKind(BasicCallExpression::BCK_BINARY_PLUS);
			//
			break;
			//
		case BasicCallExpression::BCK_MULTIPLY:
			//
			rBasicCallExpression.setKind(BasicCallExpression::BCK_DIVISION);
			//
			break;
			//
		case BasicCallExpression::BCK_DIVISION:
			//
			rBasicCallExpression.setKind(BasicCallExpression::BCK_MULTIPLY);
			//
			break;
		OPS_DEFAULT_CASE_LABEL
	}

	DeepWalker::visit(rBasicCallExpression);
}

//////////////////////////////////////////////////////////////////////
// applySillyToTranslationUnit()

void OPS::Transforms::Scalar::applySillyToTranslationUnit(
	TranslationUnit& rTranslationUnit)
{
	Walker walker;
	walker.visit(rTranslationUnit);
}

// End of File
