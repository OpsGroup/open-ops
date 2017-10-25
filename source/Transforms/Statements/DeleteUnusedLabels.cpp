#include "Reprise/Reprise.h"
#include <Reprise/Service/DeepWalker.h>
#include "Transforms/Statements/DeleteUnusedLabels.h"

#include <iostream>
#include <vector>

namespace OPS
{
namespace Transforms
{
	class CollectGotoStatements : public OPS::Reprise::Service::DeepWalker {
	public:
	    using OPS::Reprise::Service::DeepWalker::visit;

        void visit(OPS::Reprise::GotoStatement& gotoSt) {   // РґРѕР±Р°РІР»СЏРµРј РєР°Р¶РґС‹Р№ РѕРїРµСЂР°С‚РѕСЂ goto
            m_listOfGotoStatements.push_back(&gotoSt);
	    }

	    std::vector<OPS::Reprise::GotoStatement*> getList() {
            return m_listOfGotoStatements;
	    }

	private:
	    std::vector<OPS::Reprise::GotoStatement*> m_listOfGotoStatements;
	};


	class CollectLabeledStatements : public OPS::Reprise::Service::DeepWalker { // РґРѕР±Р°РІР»СЏРµРј РєР°Р¶РґС‹Р№ РїРѕРјРµС‡РµРЅРЅС‹Р№ РѕРїРµСЂР°С‚РѕСЂ
	public:
	    using OPS::Reprise::Service::DeepWalker::visit;

	    void visit(OPS::Reprise::BlockStatement& stmnt) {
            if (stmnt.hasLabel()) {
                m_listOfLabeledStatements.push_back(&stmnt);
            }
            OPS::Reprise::Service::DeepWalker::visit(stmnt);
	    }

	    void visit(OPS::Reprise::ForStatement& stmnt) {
            if (stmnt.hasLabel()) {
                 m_listOfLabeledStatements.push_back(&stmnt);
            }
            OPS::Reprise::Service::DeepWalker::visit(stmnt);
	    }

	    void visit(OPS::Reprise::WhileStatement& stmnt) {
            if (stmnt.hasLabel()) {
                m_listOfLabeledStatements.push_back(&stmnt);
            }
            OPS::Reprise::Service::DeepWalker::visit(stmnt);
	    }

	    void visit(OPS::Reprise::IfStatement& stmnt) {
            if (stmnt.hasLabel()) {
                m_listOfLabeledStatements.push_back(&stmnt);
            }
            OPS::Reprise::Service::DeepWalker::visit(stmnt);
	    }

	    void visit(OPS::Reprise::PlainSwitchStatement& stmnt) {
            if (stmnt.hasLabel()) {
                m_listOfLabeledStatements.push_back(&stmnt);
            }
            OPS::Reprise::Service::DeepWalker::visit(stmnt);
	    }

	    void visit(OPS::Reprise::GotoStatement& stmnt) {
            if (stmnt.hasLabel()) {
                m_listOfLabeledStatements.push_back(&stmnt);
            }
	    }

	    void visit(OPS::Reprise::ReturnStatement& stmnt) {
            if (stmnt.hasLabel()) {
                m_listOfLabeledStatements.push_back(&stmnt);
            }
	    }

	    void visit(OPS::Reprise::ExpressionStatement& stmnt) {
            if (stmnt.hasLabel()) {
                m_listOfLabeledStatements.push_back(&stmnt);
            }
            OPS::Reprise::Service::DeepWalker::visit(stmnt);
	    }

	    void visit(OPS::Reprise::ASMStatement& stmnt) {
            if (stmnt.hasLabel()) {
                m_listOfLabeledStatements.push_back(&stmnt);
            }
            OPS::Reprise::Service::DeepWalker::visit(stmnt);
	    }

	    void visit(OPS::Reprise::EmptyStatement& stmnt) {
            if (stmnt.hasLabel()) {
                m_listOfLabeledStatements.push_back(&stmnt);
            }
	    }

	    std::vector<OPS::Reprise::StatementBase*> getList() {
            return m_listOfLabeledStatements;
        }

	private:
	    std::vector<OPS::Reprise::StatementBase*> m_listOfLabeledStatements;
	};

	void deleteLabel(OPS::Reprise::StatementBase* labledStatement) {
	    labledStatement->setLabel("");  // СѓСЃС‚Р°РЅРѕРІРєР° РїСѓСЃС‚РѕР№ РјРµС‚РєРё ~ СѓРґР°Р»РµРЅРёРµ РјРµС‚РєРё

	    if (labledStatement->is_a<OPS::Reprise::BlockStatement>()) {

            if (labledStatement->cast_ptr<OPS::Reprise::BlockStatement>()->isEmpty()) {  // СѓРґР°Р»РµРЅРёРµ РїРѕРјРµС‡РµРЅРЅРѕРіРѕ Р±Р»РѕРєР°, РµСЃР»Рё РѕРЅ РїСѓСЃС‚РѕР№
                OPS::Reprise::BlockStatement& parentBlock = labledStatement->getParentBlock();
                parentBlock.erase(labledStatement);
            }
	    }
        else if (labledStatement->is_a<OPS::Reprise::EmptyStatement>()) { // СѓРґР°Р»РµРЅРёРµ РїСѓСЃС‚РѕРіРѕ РѕРїРµСЂР°С‚РѕСЂР°
                OPS::Reprise::BlockStatement& parentBlock = labledStatement->getParentBlock();
                parentBlock.erase(labledStatement);
	    }
	}

    void deleteUnusedLabels(OPS::Reprise::StatementBase& statement) // СѓРґР°Р»РµРЅРёРµ РјРµС‚РѕРє, РЅРµРёСЃРїРѕР»СЊР·СѓРµРјС‹С… РІ С„СЂР°РіРјРµРЅС‚Рµ РєРѕРґР°, Р° С‚Р°РєР¶Рµ РїРѕРјРµС‡РµРЅРЅС‹С… С‚Р°РєРёРјРё РјРµС‚РєР°РјРё
    {                                                               // РїСѓСЃС‚С‹С… РѕРїРµСЂР°С‚РѕСЂРѕРІ Рё Р±Р»РѕРєРѕРІ
	    CollectGotoStatements collectGotoStatements;
	    CollectLabeledStatements collectLabeledStatements;
	    OPS::Reprise::BlockStatement& rootBlock = statement.getRootBlock();

        rootBlock.accept(collectGotoStatements);    //
        rootBlock.accept(collectLabeledStatements); // СЃРїРёСЃРєРё РјРµС‚РѕРє СЃРѕСЃС‚Р°РІР»РµРЅС‹

	    std::vector<OPS::Reprise::StatementBase*> labledStatements;
	    std::vector<OPS::Reprise::GotoStatement*> gotoStatements;
	    labledStatements = collectLabeledStatements.getList();
	    gotoStatements = collectGotoStatements.getList();

		for(size_t i = 0; i < labledStatements.size(); i++) {

            int counter = 0;  //  РѕС‚СЃС‡РёС‚С‹РІР°РµС‚ РєРѕР»РёС‡РµСЃС‚РІРѕ СЃРѕРІРїР°РґРµРЅРёР№ РЅР°Р·РІР°РЅРёР№ РјРµС‚РѕРє
			for (size_t j = 0; j < gotoStatements.size(); j++)
                if (labledStatements[i]->getLabel() == gotoStatements[j]->getPointedStatement()->getLabel())
                    counter++;
            if(counter == 0)  // С‚Р°Рє Рё РЅРµ РІСЃС‚СЂРµС‚РёР» РјРµС‚РєСѓ СЃСЂРµРґРё РёСЃРїРѕР»СЊР·СѓРµРјС‹С…
                deleteLabel(labledStatements[i]);
        }
    }
}
}
