#ifndef OPS_ALIASANALYSIS_CALLGRAPHEX_H_INCLUDED__
#define OPS_ALIASANALYSIS_CALLGRAPHEX_H_INCLUDED__

#include "Analysis/AliasAnalysis/PointersTable.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace AliasAnalysis
{

/// Расширенный граф вызовов подпрограмм
class CallGraphEx
{
public: 
	typedef std::list<const ExprCallEx*> ExprCallExList;
	typedef std::map<const SubroutineDeclaration*, ExprCallExList*> SubProcCallMap;
	/*** 
	Вершина (узел) графа вызовов, представляющая собой 
	одну подпрограмму, из которой вызываются другие подпрограммы
	*/
	class Node
	{	
		///подпрограмма, из которой делаются вызовы и в которую делаются вызовы.
		SubroutineDeclaration* m_SubProc;
		///список подпрограмм и их вызовов, находящихся в теле данной подпрограммы
		SubProcCallMap m_calls;
		void addNode(const SubroutineDeclaration* aSubProc, const SubroutineCallExpression* callSite);
		friend class CallGraphEx;
	public:
		Node(SubroutineDeclaration* aSubProc);
		virtual ~Node(void);
		const SubProcCallMap& getCalls() const;
		const SubroutineDeclaration* getSubProc() const;
		//non-const getSubProc
		SubroutineDeclaration* getSubProc();
	};
	typedef std::map<std::string, Node*> NodeMap;
private:
	TranslationUnit* m_TranslationUnit;
	ExprCallEx* m_ExprCallExMain;
	NodeMap m_Graph;
	void parseExpression(Node* pNode, const ExpressionBase* expression);
	void parseBlock(Node* procNode, const BlockStatement& aBlock);
	void buildSubProc(SubroutineDeclaration& pSubProcDeclaration);
	void build();
public:
	CallGraphEx(TranslationUnit* aTranslationUnit);
	virtual ~CallGraphEx(void);
	const NodeMap &getGraph() const;
	ExprCallEx* GetExprCallExMain() { return m_ExprCallExMain; };
	ExprCallEx* GetSubProcExprCallEx(const SubroutineCallExpression* exprCall);
	ExprCallEx* GetSubProcExprCallEx(const ExprName& procName, const SubroutineCallExpression* exprCall);
};

} // end namespace AliasAnalysis
} // end namespace OPS


#endif
