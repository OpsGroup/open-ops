#ifndef RENAME_DISTRIBUTION_ARRAY_VISITOR_H
#define RENAME_DISTRIBUTION_ARRAY_VISITOR_H

#include <math.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <stack>
#include <cassert>

#include "Reprise/Reprise.h"
#include "Reprise/Service/DeepWalker.h"
#include "Transforms/DataDistribution/Shared/BDParameters.h"

class RenameDistributionArrayVisitor : public OPS::Reprise::Service::DeepWalker
{		
public:
	using OPS::Reprise::Service::DeepWalker::visit;

	OPS::Reprise::StatementBase *firstStmt;
	OPS::Reprise::StatementBase *lastStmt;

	OPS::Reprise::VariableDeclaration *oldArrayDecl;
	OPS::Reprise::VariableDeclaration *newArrayDecl;

	RenameDistributionArrayVisitor();

	void visit(OPS::Reprise::BlockStatement&);
	void visit(OPS::Reprise::ForStatement&);
	void visit(OPS::Reprise::WhileStatement&);
	void visit(OPS::Reprise::IfStatement&);
	void visit(OPS::Reprise::PlainSwitchStatement&);
	void visit(OPS::Reprise::GotoStatement&);
	void visit(OPS::Reprise::ReturnStatement&);
    void visit(OPS::Reprise::Canto::HirBreakStatement&);
    void visit(OPS::Reprise::Canto::HirContinueStatement&);
	void visit(OPS::Reprise::ExpressionStatement&);
	void visit(OPS::Reprise::EmptyStatement&);

	void visit(OPS::Reprise::ReferenceExpression& bckCall);
private:
	int processStmt(OPS::Reprise::StatementBase& stmt);
	bool m_startRenameNames;
};

#endif
