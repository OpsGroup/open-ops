#ifndef _OPS_REPRISE_SSAFORM_H_INCLUDED_
#define _OPS_REPRISE_SSAFORM_H_INCLUDED_

#include "Analysis/SSAForm/VirtualRegistry.h"
#include "Analysis/DominanceFrontierGraph.h"


namespace OPS 
{
namespace Analysis
{
namespace SSAForms
{

	using namespace ControlFlowGraphs::SSAControlFlowGraph;
	using ControlFlowGraphs::ControlFlowGraphEx;
	using ControlFlowGraphs::Stash;	
	using namespace ControlFlowGraphs::DominanceFrontierGraph;


	//класс, описывающий индекс переменной, для которой построена SSA-форма. 
	//бывает индекс при использовании переменной, индекс при генераторе переменной и индекс при сложном выражении с переменной
	//например, i++ является и генератором и использованием (означает конструкцию типа i2=i1+1), поэтому индекс должен содержать два числа - индекс генератора и индекс использования


	class SSASubscript
	{
	public:
		enum SSASubscriptType
		{
			SSAST_USE=0,
			SSAST_GEN,
			SSAST_COMPLEX
		};
	private:
		SSASubscriptType m_ssast;
		const VariableDeclaration* m_var;
		int m_useSubscript;
		int m_genSubscript;

	public:
		bool isGenSubscript() const;
		bool isUseSubscript() const;

		bool isComplexSubscript() const;

		int getUseSubscript() const;

		int getGenSubscript() const;


		SSASubscriptType getSSASubscriptType() const;
		const VariableDeclaration& getVariable() const;
		bool operator==(const SSASubscript& compared_to) const;
		bool operator<(const SSASubscript& compared_to) const;
		bool equals(const SSASubscript& compared_to) const;

	private:

		SSASubscript(const VariableDeclaration& initvar, int subscript, SSASubscriptType ssast);
		SSASubscript(const VariableDeclaration& initvar, int genSubscript, int useSubscript);
		friend class SSAForm;		
	};


	//класс, описывающий выражение Ai=phi(Bj,Ck,...)
	//Ai доступно с помощью функции getGenSubscript, Bi,Ck,... доступно с помощью getUsesSeubscript/getUsesMap
	class PhiFunction: public OPS::BaseVisitable<>
	{
	public:
		typedef std::map<SSASubscript*, StatementVector*> UsesSubscriptMap;

	private:		
		SSASubscript* m_genSubscript;

		UsesSubscriptMap m_usesSubscript;
		const StatementVector* mDefinedBlock;
	public:

		SSASubscript* getGenSubscript() const; ;
		std::list<SSASubscript*> getUsesSubscript() const;
		const UsesSubscriptMap& getUsesMap() const; ;
		const StatementVector * getDefinedBlock() const
		{
			return mDefinedBlock;
		}

		PhiFunction();
		~PhiFunction();
		OPS_DEFINE_VISITABLE()
	private:		
		void setGen(SSASubscript* gen);
		void addUse(SSASubscript* use, StatementVector* path);

		PhiFunction(SSASubscript* gs, std::list<SSASubscript*>& uses, StatementVector* DefinedBlock);
		friend class SSAForm;
	};

	class SSAVarMap: public IntrusivePointerBase
	{

	private:
		std::map<StatementVector*, std::map <const VariableDeclaration*, int, VarNameLess> > defined;

		std::map<StatementVector*, 
			std::map <const VariableDeclaration*, std::list<StatementVector*>, VarNameLess> > part_defined;

		std::map<Stash*, ReferenceExpression*> gens;
		std::map<Stash*, VariableDeclaration*> gens_declarations;
		std::map<Stash*, std::list<ReferenceExpression*> > uses;
		std::map<StatementVector*, int> visited;
		std::map< StatementVector*, std::set<ReferenceExpression*> > unclear;
		ControlFlowGraphEx* cfgex;
		VirtualRegistry* vreg;
		DominanceFrontier* df;
		BlockStatement* block;
	public:
		SSAVarMap(ControlFlowGraphEx& _cfgex, VirtualRegistry& _vr, DominanceFrontier& _df, BlockStatement* _block);
		~SSAVarMap();
		void index( StatementVector* sv);
		bool reindexUnclear( StatementVector* sv);
		ReferenceExpression* getGen(Stash* stmt);
		VariableDeclaration* getDeclaration(Stash* stmt);
		std::list<ReferenceExpression*>&  getUses(Stash* stmt);
	};

	//контейнер для фи-функции либо генератора, обозначает генератор в SSA-форме
	class SSAGenerator
	{
	public:
		enum SSAGeneratorType
		{
			SSAGT_PHI=0,
			SSAGT_GENERATOR,
			SSAGT_DECLARATION
		};		
		SSAGeneratorType getType() const 
		{
			return m_type;
		}
		//получить переменную, чей это генератор
		const VariableDeclaration& getVariable();
		const PhiFunction* getPhi() const 
		{
			return m_phi;
		}
		const ReferenceExpression* getExpression() const 
		{
			return m_oper;
		}
		const VariableDeclaration* getDeclaration() const
		{
			return m_decl;
		}
		

	private:
		SSAGenerator();
		const PhiFunction* m_phi;
		const ReferenceExpression* m_oper;
		const VariableDeclaration* m_decl;
		SSAGeneratorType m_type;
		SSAGenerator(const PhiFunction* p_phi);
		SSAGenerator(const ReferenceExpression* p_oper);
		SSAGenerator(const VariableDeclaration* p_decl);
		friend class SSAForm;
	};
		

	class SSAForm : public OPS::NonCopyableMix, public IntrusivePointerBase
	{
	public:
		typedef std::list<const PhiFunction*> PhiList;
		typedef std::list<const SSAGenerator*> GeneratorList;
	public:
		//конструктор
		SSAForm(BlockStatement& bs);
	public:
		ControlFlowGraphEx& getCFGex();
		DominanceFrontier& getDominanceFrontier();
	public:
		//если occurrence содержит	содержит переменную, положенную на SSA-форму, то вернётся её индекc для этого использования/генератора.
		//если не содержит, вернется 0
		const SSASubscript* getSubscript(const ExpressionBase* occurrence) const;
		const SSASubscript* getSubscript(const VariableDeclaration* decl) const;		
		//фи-функции, выполняющиеся перед первым оператором линейного блока cfblock
		const PhiList getPhis( StatementVector* cfblock) const;
		//получить генератор по номеру переменной или ноль
		const SSAGenerator*  getSSAGenerator(const SSASubscript* subscript) const;
		//получить генератор по вхождению переменной или ноль
		const SSAGenerator* getSSAGenerator(const ExpressionBase* occurrence) const;

		//список генераторов
		const GeneratorList& getGenerators();



	private:

		typedef std::map<const ExpressionBase*, SSASubscript*> ExpressionsSubscriptsMap;
		typedef std::map<const VariableDeclaration*, SSASubscript*> DeclarationsSubscriptsMap;
		typedef std::map<const SSASubscript*, SSAGenerator*> SSAGraphMap;


		mutable ExpressionsSubscriptsMap m_subscripts;
		mutable DeclarationsSubscriptsMap m_decl_subscripts;

		mutable SSAGraphMap m_subscripts_to_gens;


		BlockStatement* m_bs;

		//to_delete
		ReprisePtr<VirtualRegistry> m_vreg;
		ReprisePtr<DominanceFrontier> m_df;
		ReprisePtr<SSAVarMap> m_varmap;
		
		std::map<const VariableDeclaration*, std::stack<int>, VarNameLess > stacks;
		std::map<const VariableDeclaration*, int, VarNameLess > counters;
		std::map<SSASubscript*,  StatementVector*> phiargssources;
		//какие блоки содержат генераторы данной переменной
		std::map<const VariableDeclaration*,  StatementVectorList*, VarNameLess> gens;

		ControlFlowGraphEx* m_cfgex;
		StatementVectorList* m_blocks;
	
		
	    typedef std::map<const VariableDeclaration*, PhiFunction, VarNameLess> PhisMap;
	    typedef std::map< StatementVector*, PhisMap > PhisAnchorMap;

		mutable PhisAnchorMap m_phisAnchors;


	private:
		void fillGens();
		void placePhis();
		void insertPhi( StatementVector* ins_to, const VariableDeclaration& var);
		void indexVars( StatementVector* blk);
		void insertPhiArg( StatementVector* ins_to, const VariableDeclaration& old,  StatementVector* from , int what);
		

		

	// деструктор
	private:
		void clearSubscripts();
		void clearPhis();
		void clearSubscriptsToGens();
	public:
		~SSAForm();

	};



}
}
}




#endif
