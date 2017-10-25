#ifndef OPS_IR_REPRISE_CALL_GRAPH_H_INCLUDED__
#define OPS_IR_REPRISE_CALL_GRAPH_H_INCLUDED__

#include <string>
#include <vector>
#include <map>
#include <list>
#include <stack>
#include "Reprise/Reprise.h"
#include "Reprise/Service/DeepWalker.h"

/// Граф вызовов подпрограмм
class CallGraph: public OPS::Reprise::Service::DeepWalker
{
public: 
	typedef std::list<const OPS::Reprise::SubroutineCallExpression*> ExprCallList;
	typedef std::map<const OPS::Reprise::SubroutineDeclaration*, ExprCallList*> SubProcCallMap;

	/*** 
	Вершина (узел) графа вызовов, представляющая собой 
	одну подпрограмму, из которой вызываются другие подпрограммы
	*/
	class Node
	{	
		/// подпрограмма, из которой делаются вызовы и в которую делаются вызовы.
		OPS::Reprise::SubroutineDeclaration *m_SubProc;
		///список подпрограмм и их вызовов, находящихся в теле данной подпрограммы
		SubProcCallMap m_calls;
		void addNode(const OPS::Reprise::SubroutineDeclaration *aSubProc,
					 const OPS::Reprise::SubroutineCallExpression* callSite);
        friend class CallGraph;
	public:
		Node(OPS::Reprise::SubroutineDeclaration *aSubProc);
		~Node();

		const SubProcCallMap& getCalls() const;

		const OPS::Reprise::SubroutineDeclaration *getSubProc() const;
		OPS::Reprise::SubroutineDeclaration *getSubProc();
	};
	typedef std::map<std::string, Node*> NodeMap;

public:
	/// Построить граф для всех объявленных в данном модуле функций
	CallGraph(OPS::Reprise::TranslationUnit *aTranslationUnit);

	/// Построить граф для всех функций программы
	CallGraph(OPS::Reprise::ProgramUnit* aProgramUnit);

	/// Построить граф только для функций, которые вызываются из данной
	CallGraph(OPS::Reprise::SubroutineDeclaration* aSubroutine);

	/// Построить граф для фрагмента программы
	CallGraph(OPS::Reprise::StatementBase* aStatement);

	virtual ~CallGraph(void);
	const NodeMap &getGraph() const;

	// выдает описание графа на языке DOT (для Graphviz)
	std::string dumpState() const;
	// удаляет из графа те подпрограммы, которые одновременно:
	// 1) содержатся не в указанной единице трансляции
	// 2) не имеют входящих дуг
	// 3) не имеют исходящих дуг
	void removeExcessNodes(OPS::Reprise::ProgramUnit* aTranslationUnit);
	// TODO: вместо 2) и 3) должны удаляться подпрограммы, в которые нет
	// пути на графе от какой-либо подпрограммы указанной единицы трансляции

public:
	using OPS::Reprise::Service::DeepWalker::visit;

	void visit(OPS::Reprise::Declarations&);
	void visit(OPS::Reprise::SubroutineCallExpression&);

private:
	typedef std::stack<Node*> NodeStack;
	typedef std::set<OPS::Reprise::SubroutineDeclaration*> SubroutineSet;

	class SafeStackPopper : public OPS::NonCopyableMix
	{
		NodeStack& m_stackName;
	public:
		SafeStackPopper(NodeStack& stackName, Node* value): m_stackName(stackName) {m_stackName.push(value);}
		~SafeStackPopper() {m_stackName.pop();}
	};

	void buildByBFS(SubroutineSet notVisitedNodes);

	NodeMap m_Graph;
	NodeStack m_CurrentNodes;
};

#endif //OPS_IR_REPRISE_CALL_GRAPH_H_INCLUDED__
