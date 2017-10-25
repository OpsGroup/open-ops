#include "Shared/LabelsShared.h"
#include "Reprise/Service/DeepWalker.h"
#include <string>
#include <map>

namespace OPS
{
namespace Shared
{

using OPS::Reprise::Service::DeepWalker;
using OPS::Reprise::StatementBase;
using OPS::Reprise::BlockStatement;
using OPS::Reprise::ForStatement;
using OPS::Reprise::WhileStatement;
using OPS::Reprise::IfStatement;
using OPS::Reprise::GotoStatement;
using OPS::Reprise::ReturnStatement;
using OPS::Reprise::ExpressionStatement;
using OPS::Reprise::EmptyStatement;


class GenerateNewLabelsWalker: public DeepWalker
{
public:
    GenerateNewLabelsWalker()
        : m_firstVisitCall(true) {}

    void visit(BlockStatement& repriseStatement)
    {
        generateNewLabel(repriseStatement);
        DeepWalker::visit(repriseStatement);
    }
    void visit(ForStatement& repriseStatement)
    {
        generateNewLabel(repriseStatement);
        DeepWalker::visit(repriseStatement);
    }

    void visit(WhileStatement& repriseStatement)
    {
        generateNewLabel(repriseStatement);
        DeepWalker::visit(repriseStatement);
    }

    void visit(IfStatement& repriseStatement)
    {
        generateNewLabel(repriseStatement);
        DeepWalker::visit(repriseStatement);
    }
    void visit(GotoStatement& repriseStatement)
    {
        generateNewLabel(repriseStatement);
    }

    void visit(ReturnStatement& repriseStatement)
    {
        generateNewLabel(repriseStatement);
    }

    void visit(ExpressionStatement& repriseStatement)
    {
        generateNewLabel(repriseStatement);
    }
    void visit(EmptyStatement& repriseStatement)
    {
        generateNewLabel(repriseStatement);
    }

    std::map<std::string, StatementBase*> oldLabels2NewStatementsMap() const
    {
        return m_oldLabels2NewStatementsMap;
    }


private:
    void generateNewLabel(StatementBase& repriseStatement)
    {
        if (!m_firstVisitCall)
        {
            const std::string oldLabel = repriseStatement.getLabel();
            if (!oldLabel.empty())
            {
                repriseStatement.setUniqueLabel(oldLabel);
                m_oldLabels2NewStatementsMap[oldLabel] = &repriseStatement;
            }
        }
        else
        {
            m_firstVisitCall = false;
        }
    }


private:
    std::map<std::string, StatementBase*> m_oldLabels2NewStatementsMap;
    bool m_firstVisitCall;
};

class UpdateGotoWalker: public DeepWalker
{
public:
    explicit UpdateGotoWalker(const std::map<std::string, StatementBase*>& oldLabels2NewStatementsMap)
    : m_oldLabels2NewStatementsMap(oldLabels2NewStatementsMap)
    {
    }

    void visit(GotoStatement& gotoStatement)
    {
        StatementBase* pointedStatement(gotoStatement.getPointedStatement());
        if (pointedStatement != NULL)
        {
            const std::string label = pointedStatement->getLabel();
            std::map<std::string, StatementBase*>::const_iterator statementWithSameLabel =
                m_oldLabels2NewStatementsMap.find(label);
            if (statementWithSameLabel != m_oldLabels2NewStatementsMap.end())
            {
                OPS_ASSERT(statementWithSameLabel->second != NULL);
                gotoStatement.setPointedStatement(statementWithSameLabel->second);
            }
        }
    }

private:
    const std::map<std::string, StatementBase*> m_oldLabels2NewStatementsMap;
};


ReprisePtr<StatementBase> updateLabel(ReprisePtr<StatementBase> sourceStmt, StatementBase& destinationStmt)
{
	const std::string label(sourceStmt->getLabel());
	if (!label.empty())
	{
		std::map<std::string, StatementBase*> oldLabels2NewStatementsMap;
		destinationStmt.setLabel(label);
		oldLabels2NewStatementsMap[label] = &destinationStmt;
		UpdateGotoWalker updateGotoWalker(oldLabels2NewStatementsMap);
		destinationStmt.getRootBlock().accept(updateGotoWalker);
	}

	return sourceStmt;
}

bool isPossibleToGenerateNewLabels(StatementBase& statementBase)
{
	// TODO: Implement
	return true;
}

void generateNewLabels(StatementBase& statementBase)
{
	OPS_ASSERT(isPossibleToGenerateNewLabels(statementBase));

	GenerateNewLabelsWalker generateNewLabelsWalker;
	statementBase.accept(generateNewLabelsWalker);
	UpdateGotoWalker updateGotoWalker(generateNewLabelsWalker.oldLabels2NewStatementsMap());
	statementBase.accept(updateGotoWalker);
}

}	// Shared
}	// OPS
