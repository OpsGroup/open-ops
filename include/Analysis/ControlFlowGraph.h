#ifndef OPS_IR_REPRISE_CONTROL_FLOW_GRAPH_H_INCLUDED__
#define OPS_IR_REPRISE_CONTROL_FLOW_GRAPH_H_INCLUDED__

#include <map>
#include <list>
#include <vector>
#include <stack>
#include <set>

#include "Reprise/Reprise.h"
#include "Reprise/Service/DeepWalker.h"

using namespace OPS;
using namespace OPS::Reprise;
/************************************************************************/
/* Граф потока управления программы										*/
/************************************************************************/
class ControlFlowGraph: public OPS::Reprise::Service::DeepWalker
{
public:
	typedef std::list<const StatementBase*> StatementList;
	typedef std::map<const StatementBase*, StatementList*> StatementGraph;
	typedef std::vector<const StatementBase*> StatementVector;
	typedef std::list< std::pair<const StatementBase*, const StatementBase*> > EdgeList;
	typedef std::vector< std::pair<const StatementBase*, const StatementBase*> > EdgeArray;
	typedef std::map<const StatementBase*, int> StatementNumbers;
private:
	typedef std::stack<const StatementBase*> StatementStack;
	class SafeStackPopper : public OPS::NonCopyableMix
	{
		StatementStack& m_stackName;
	public:
		SafeStackPopper(StatementStack& stackName, const StatementBase* value): m_stackName(stackName) {m_stackName.push(value);}
		~SafeStackPopper() {m_stackName.pop();}
	};
	/// кореневой оператор, с которого начинается построение графа
	const BlockStatement &m_pBlock;
	/// граф
	StatementGraph m_graph;
	/// минимальное множество дуг, обеспечивающее покрытие всех дуг графа
	EdgeList m_minEdgeSet;
	/// стек управляющих операторов
	StatementStack m_controls;
	/// стек для хранения переходов при досрочном прерывании циклов
	StatementStack m_breaks;
	/// стек для хранения переходов при досрочном переходе на следующую итерацию циклов
	StatementStack m_continues;
private:
	typedef std::map<const Reprise::StatementBase*, int> NodesNumbering;
	typedef std::map<int, const Reprise::StatementBase* > ReverseNodesNumbering;
	typedef std::vector< std::valarray<bool> > Paths;
	const StatementBase& getStatementEntry(const StatementBase& stmt) const;
	const Reprise::StatementBase* preVisit(Reprise::StatementBase& statement, StatementList*& l);
	StatementList& safeRetrieveList(const StatementBase& pStatement);
	const StatementBase* firstStatement(const StatementBase& statement) const;
	const StatementBase* next(const StatementBase& pStatement) const;
	const StatementBase* nextStatement(const StatementBase& statement) const;
    NodesNumbering::const_iterator findVertice(const StatementBase& stmt) const;
	StatementVector m_statementVector;
	// запрещаем копирование графа
	ControlFlowGraph(const ControlFlowGraph&);
	ControlFlowGraph& operator=(const ControlFlowGraph&);
	void BuildMinEdgeSet();
	mutable NodesNumbering m_NodesNumbering;
	mutable ReverseNodesNumbering m_ReverseNodesNumbering;
	mutable Paths m_Paths;
	void buildPaths() const;
	bool pathsBuilt() const { return !(m_NodesNumbering.empty()); }
public:
	/// строит граф управления для заданного блока операторов
	explicit ControlFlowGraph(const BlockStatement &Block);
	virtual ~ControlFlowGraph();
	/// возвращает число вершин-операторов графа
	size_t statementsCount() const {return m_graph.size();} 
	const BlockStatement& rootBlock() const {return m_pBlock;}
	/// Возвращает список вершин, в которые возможна передача управления из данного оператора
	const StatementList& operator[](const StatementBase* pStmt) const
	{
		StatementGraph::const_iterator it(m_graph.find(const_cast<StatementBase*>(pStmt)));
		OPS_ASSERT(it != m_graph.end());
		return *it->second;
	}
	const StatementGraph& getGraph() const {return m_graph;}
	StatementGraph& getGraph(){return m_graph;}
	// возвращает вектор всех операторов фрагмента программы
	const StatementVector& getStatementVector() const {return m_statementVector;}
	bool hasEdge(const Reprise::StatementBase* pSource, const Reprise::StatementBase* pDestination) const;
	// for testing purposes only!
	void PrintToCout() const;
	// Возвращает описание графа на языке DOT
	std::string dumpState() const;
	const EdgeList& GetMinEdgeSet();
	// Отвечает на вопрос, есть ли путь от source к destination на графе потока управления
	// source и destination должны принадлежать блоку, по которому строился граф
	bool hasPath(const Reprise::StatementBase* source, const Reprise::StatementBase* destination) const;

	/// Возвращает список дуг по которым возможна передача управления снаружи stmt в stmt или его потомок
	EdgeList getInEdges(const StatementBase& stmt) const;

	/// Возвращает список дуг по которым возможна передача управления от оператора stmt или
	/// одного из его потомков наружу stmt
	EdgeList getOutEdges(const StatementBase& stmt) const;
public:
	void checkStatementVector();
	using OPS::Reprise::Service::DeepWalker::visit;

	void visit(OPS::Reprise::ExpressionStatement&);
	void visit(OPS::Reprise::BlockStatement&);
	void visit(OPS::Reprise::ForStatement&);
	void visit(OPS::Reprise::WhileStatement&);
	void visit(OPS::Reprise::IfStatement&);
	void visit(OPS::Reprise::PlainSwitchStatement&);
    void visit(OPS::Reprise::Canto::HirBreakStatement&);
    void visit(OPS::Reprise::Canto::HirContinueStatement&);
	void visit(OPS::Reprise::GotoStatement&);
	void visit(OPS::Reprise::ReturnStatement&);
	void visit(OPS::Reprise::EmptyStatement&);
};

class ControlFlowGraphExpr
{
public:
    class BasicBlock;
    typedef std::vector<BasicBlock*> BasicBlockList;

    class BasicBlock : public std::vector<const ExpressionBase*>
    {
    public:
        BasicBlockList& getInBlocks() { return m_inBlocks; }
        const BasicBlockList& getInBlocks() const { return m_inBlocks; }
        BasicBlockList& getOutBlocks() { return m_outBlocks; }
        const BasicBlockList& getOutBlocks() const { return m_outBlocks; }

        bool isEntry() const { return m_inBlocks.empty(); }
        bool isExit() const { return m_outBlocks.empty(); }
    private:
        BasicBlockList m_inBlocks;
        BasicBlockList m_outBlocks;
    };

    ControlFlowGraphExpr();
    ~ControlFlowGraphExpr();

    void clear();
    void addBasicBlock(BasicBlock* block);

    BasicBlockList& getBlocks() { return m_allBlocks; }

    BasicBlock& getEntry() { return *m_allBlocks.front(); }
    BasicBlock& getExit() { return *m_allBlocks.back(); }

	BasicBlock* findBlockOfExpr(const ExpressionBase*);

private:
    /// All basic blocks
    BasicBlockList m_allBlocks;
};

class ControlFlowGraphExprBuilder : public OPS::Reprise::Service::DeepWalker
{
public:
	explicit ControlFlowGraphExprBuilder(bool mergeBlocks = true);
	void build(StatementBase& stmt, ControlFlowGraphExpr& graph);
private:
    typedef ControlFlowGraphExpr::BasicBlock BasicBlock;

    ControlFlowGraphExpr* m_graph;
    typedef std::map<ExpressionBase*, ControlFlowGraphExpr::BasicBlock*> ExprToBlockMap;

    ExprToBlockMap m_exprToBlock;

    struct CurrentPos
    {
        explicit CurrentPos(StatementBase* s = 0, bool outside = true)
            :stmt(s),fromOutside(outside) {}
        StatementBase* stmt;
        bool fromOutside;
    };

    BasicBlock* m_entryBlock;
    BasicBlock* m_exitBlock;
    BasicBlock* m_currentBlock;
    CurrentPos m_current;
    std::list<std::pair<BasicBlock*,CurrentPos> > m_altStmts;
	bool m_mergeBlocks;

    void buildBaseGraph(StatementBase& firstStmt);
    void mergeBlocks();
    void buildGraph();

    void visit(OPS::Reprise::ExpressionStatement&);
    void visit(OPS::Reprise::BlockStatement&);
    void visit(OPS::Reprise::ForStatement&);
    void visit(OPS::Reprise::WhileStatement&);
    void visit(OPS::Reprise::IfStatement&);
    void visit(OPS::Reprise::PlainSwitchStatement&);
    void visit(OPS::Reprise::Canto::HirBreakStatement&);
    void visit(OPS::Reprise::Canto::HirContinueStatement&);
    void visit(OPS::Reprise::GotoStatement&);
    void visit(OPS::Reprise::ReturnStatement&);
    void visit(OPS::Reprise::EmptyStatement&);

    void saveAlternative(const CurrentPos& pos);
    bool visitExpr(ExpressionBase& expr);
    void visitExit();

    static CurrentPos nextInBlock(StatementBase& stmt);
};

#endif
