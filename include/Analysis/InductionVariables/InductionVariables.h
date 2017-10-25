#ifndef _OPS_INDUCTIONVARIABLES_H_INCLUDED_
#define _OPS_INDUCTIONVARIABLES_H_INCLUDED_

#include <memory>
#include "Analysis/SSAForm/SSAForm.h"
#include "Reprise/Service/DeepWalker.h"


namespace OPS 
{
	namespace Analysis
	{
		class InductionDescription;
		class LinearIntegerInductionDescription;
		struct TarjanNode;
		struct TarjanNodeFactory;
		class NodeVisitor;
		class LoopHelper;
		class LoopInductionAnalysis;
		typedef std::map<const OPS::Analysis::SSAForms::SSAGenerator*, InductionDescription*> InductionClassMap;



		class InductionAnalysis: public OPS::NonCopyableMix
		{
		public:
			InductionAnalysis(OPS::Reprise::BlockStatement& blk);
			InductionAnalysis(OPS::Reprise::ReprisePtr<OPS::Analysis::SSAForms::SSAForm> ssa);
			//проверить, индуктивна ли переменная относительно цикла
			bool isInduction(const OPS::Analysis::SSAForms::SSAGenerator* gen);
			bool isInduction(const OPS::Reprise::ExpressionBase* occurrence);
			bool isInduction(const OPS::Analysis::SSAForms::SSASubscript* subscript);

			//получить описание индуктивной переменной
			InductionDescription* getInductionDescription(const OPS::Analysis::SSAForms::SSAGenerator* gen);
			InductionDescription* getInductionDescription(const OPS::Reprise::ExpressionBase* occurrence);
			InductionDescription* getInductionDescription(const OPS::Analysis::SSAForms::SSASubscript* subscript);

			/* проверить, есть ли эталонная переменная:
			// линейная индуктивная переменная, имеющая единственный генератор внутри цикла - в StepExpression цикла for
			*/
			bool hasIdealSequence(LoopHelper loop);			
			const OPS::Analysis::SSAForms::SSAGenerator* getIdealSequence(LoopHelper loop);
		private:
			OPS::Reprise::ReprisePtr<OPS::Analysis::SSAForms::SSAForm> mSSA;
			typedef std::list<OPS::Reprise::ExpressionBase*> LoopList;
			typedef std::map<OPS::Reprise::ExpressionBase*, OPS::Reprise::ReprisePtr<LoopInductionAnalysis> > LoopAnalysisMap;
			LoopList mLoops;
			LoopAnalysisMap mLoopAnalysis;			
			std::unique_ptr<InductionDescription> mNotFound;

			void collectLoops();
			void buildEveryLoopInductionAnalysis();
		};

		//приводит все допустимые типы циклов к необходимому для построения указателю на управляющее выражение
		class LoopHelper
		{
			OPS::Reprise::ExpressionBase* mLoop;
			LoopHelper();
		public:
			LoopHelper(OPS::Reprise::ExpressionBase* loop);
			LoopHelper(OPS::Reprise::ExpressionBase& loop);
			LoopHelper(OPS::Reprise::ForStatement& forStmt);
			LoopHelper(OPS::Reprise::WhileStatement& whileStmt);
			OPS::Reprise::ExpressionBase* getLoop();

		};

		/*базовый класс для описания индуктивности переменных. */
		class InductionDescription
		{
			bool mWrapAround;
			const OPS::Analysis::SSAForms::SSAGenerator* mWrapGenerator;

			//может быть 0
			const OPS::Reprise::ExpressionBase* mLoop;
		public:
			//возвращает цикл, для которого эта переменная индуктивна. может вернуть 0 для неиндуктивных
			const OPS::Reprise::ExpressionBase* getLoop() const;

			/*является ли переменная целочисленно-индуктивной, т.е. линейно зависящей только счетчика цикла,
			  причем все коэффициенты - константы */
			virtual bool isLinearInteger() const;
			
			virtual LinearIntegerInductionDescription* getAsLinearInteger();
			virtual const LinearIntegerInductionDescription* getAsLinearInteger() const;
			
			/*является ли охватывающей*/
			bool isWrapAround() const ;
			/*получить SSA-генератор охватывающей переменной*/
			const OPS::Analysis::SSAForms::SSAGenerator& getWrapGenerator() const;
			/*
			 может ли охватывающий генератор быть описан одним выражением?
			*/
			bool hasWrapExpression() const;
			/* если да, то получить такое выражение */
			const OPS::Reprise::ExpressionBase* getWrapExpression() const;




			/*возвращает true, если переменные равны на любой итерации цикла*/
			virtual bool equals(const InductionDescription* second);
			
			InductionDescription(const OPS::Reprise::ExpressionBase* loop);
			InductionDescription(const OPS::Reprise::ExpressionBase& loop);
			void setWrapGenerator(const OPS::Analysis::SSAForms::SSAGenerator& generator);
		};

		class LinearIntegerInductionDescription: public InductionDescription
		{
		private:
			long_long_t mCoefficient;
			long_long_t mConstantTerm;
		public:
			virtual LinearIntegerInductionDescription* getAsLinearInteger();
			virtual const LinearIntegerInductionDescription* getAsLinearInteger() const;
			LinearIntegerInductionDescription(const OPS::Reprise::ExpressionBase& loop);
			void setCoefficient(long_long_t Coefficient);
			void setConstantTerm(long_long_t ConstantTerm);
			long_long_t getCoefficient() const;
			long_long_t getConstantTerm() const ;


			static InductionDescription* sum(LinearIntegerInductionDescription& first, LinearIntegerInductionDescription& second);
			static InductionDescription* multiply(LinearIntegerInductionDescription& first, long_long_t& second);
		};


		class LoopInductionAnalysis: public OPS::NonCopyableMix, public OPS::Reprise::IntrusivePointerBase
		{

		public:
			LoopInductionAnalysis(OPS::Reprise::BlockStatement& blk, LoopHelper loop);
			LoopInductionAnalysis(OPS::Reprise::ReprisePtr<OPS::Analysis::SSAForms::SSAForm> ssa, LoopHelper loop);
			OPS::Analysis::SSAForms::SSAForm& getSSA();
			
			//проверить, индуктивна ли переменная относительно цикла
			bool isInduction(const OPS::Analysis::SSAForms::SSAGenerator* gen);
			bool isInduction(const OPS::Reprise::ExpressionBase* occurrence);
			bool isInduction(const OPS::Analysis::SSAForms::SSASubscript* subscript);

			//получить описание индуктивной переменной
			InductionDescription* getInductionDescription(const OPS::Analysis::SSAForms::SSAGenerator* gen);
			InductionDescription* getInductionDescription(const OPS::Reprise::ExpressionBase* occurrence);
			InductionDescription* getInductionDescription(const OPS::Analysis::SSAForms::SSASubscript* subscript);
			/* проверить, есть ли эталонная переменная:
			// линейная индуктивная переменная, имеющая единственный генератор внутри цикла - в StepExpression цикла for
			*/
			bool hasIdealSequence();

			



			const OPS::Analysis::SSAForms::SSAGenerator* getIdealSequence()
			{
				return mIdealSequence;
			}

			OPS::Reprise::ExpressionBase* getLoop()
			{
				return mLoop;
			}


		private:

			std::unique_ptr<NodeVisitor> mTarjanWalker;
			std::unique_ptr<TarjanNodeFactory> mTarjanFactory;
			OPS::Reprise::ReprisePtr<OPS::Analysis::SSAForms::SSAForm> mSSA;
			OPS::Reprise::ExpressionBase* mLoop;			
			InductionClassMap mInductions;
			InductionDescription* mNotFound;
			const OPS::Analysis::SSAForms::SSAGenerator* mIdealSequence;

		
			static bool isLoopHeader(OPS::Reprise::ExpressionBase* loop);
			void VisitNodes(OPS::Reprise::ExpressionBase* loop);
			void init();
			void FindComponents(OPS::Reprise::ExpressionBase* loop);
			InductionDescription* deductFromDeclaration(const OPS::Analysis::SSAForms::SSAGenerator* gen);
			const OPS::Analysis::SSAForms::SSAGenerator* deductGenerator(OPS::Reprise::VariableDeclaration*, OPS::Reprise::ExpressionBase* );
			InductionDescription* deductFromExpression(const OPS::Analysis::SSAForms::SSAGenerator* gen);
			InductionDescription* wrapInduction(InductionDescription* to_wrap, const OPS::Analysis::SSAForms::SSASubscript* wrapping);
			InductionDescription* deductFromPhi(const OPS::Analysis::SSAForms::SSAGenerator* gen);
			InductionDescription* deductInductionDescription(const OPS::Analysis::SSAForms::SSAGenerator* gen);

		};

		struct TarjanNode: public OPS::BaseVisitable<>
		{
		public:

			OPS_DEFINE_VISITABLE()
				//static std::set<TarjanNode> NodeSet;

			enum NodeType
			{
				NT_EXPR,
				NT_PHI,
				NT_MU,
				NT_SSALINK
			};

			enum NodeStatus
			{
				NS_NOTYET,
				NS_ONSTACK,
				NS_DONE
			};

			enum NodeClass
			{
				NC_INV,
				NC_WRAP,
				NC_LIN,
				NC_UNKNOWN
			};

			NodeType mType;
			NodeStatus mStatus;
			NodeClass mClass;
			int mLowlink;
			const OPS::Reprise::ExpressionBase* mExpr;
			const OPS::Analysis::SSAForms::PhiFunction* mPhi; 
			typedef std::pair<const OPS::Analysis::SSAForms::PhiFunction*, const OPS::Analysis::SSAForms::SSASubscript*> PhiSSALink;
			PhiSSALink mSSALink;

			static OPS::Reprise::ExpressionBase* defaultLoop;
			const OPS::Reprise::ExpressionBase* mLoop;


			void init();
			const OPS::Reprise::ExpressionBase* determineExprLoop();
			const OPS::Reprise::ExpressionBase* determinePhiLoop();
			const OPS::Reprise::ExpressionBase* determineSSALinkLoop();

		public:
			//bool operator<(ExpressionBase& expressionBase1, ExpressionBase& expressionBase2);
			NodeType getType() const
			{
				return mType;
			}
			NodeStatus getStatus();
			OPS::Reprise::ExpressionBase* getLoop();
			TarjanNode(const OPS::Reprise::ExpressionBase* Expr);			
			TarjanNode(const OPS::Analysis::SSAForms::PhiFunction* Phi, bool isMu );
			TarjanNode(const OPS::Analysis::SSAForms::PhiFunction* Phi, const OPS::Analysis::SSAForms::SSASubscript* Subscript);

		};
		struct TarjanNodeFactory
		{


			TarjanNode& getNode(const OPS::Reprise::ExpressionBase* Expr);
			TarjanNode& getNode(const OPS::Analysis::SSAForms::PhiFunction* Phi);

			TarjanNode& getNode(const OPS::Analysis::SSAForms::PhiFunction* Phi, const OPS::Analysis::SSAForms::SSASubscript* PhiArg);
			TarjanNodeFactory(OPS::Reprise::ExpressionBase* loop, OPS::Analysis::SSAForms::SSAForm* SSA);
		private:
			TarjanNodeFactory() 
			{

			}
			OPS::Reprise::ExpressionBase* mLoop;
			OPS::Analysis::SSAForms::SSAForm* mSSA;
			typedef std::map<const OPS::Reprise::ExpressionBase*, TarjanNode*> ExpressionMap;
			typedef std::map<const OPS::Analysis::SSAForms::PhiFunction*, TarjanNode*> PhiMap;
			typedef std::pair<const OPS::Analysis::SSAForms::PhiFunction*, const OPS::Analysis::SSAForms::SSASubscript*> PhiSSALink;
			typedef std::map<PhiSSALink, TarjanNode*> SSALinkMap;
			ExpressionMap mExpressionMap;
			PhiMap mPhiMap;
			SSALinkMap mSSALinkMap;

		};

		typedef std::stack<TarjanNode*> SCCStack;
		typedef std::list<TarjanNode*> SCC;
		typedef std::list<SCC*> SCCList;


		class NodeVisitor: public OPS::Reprise::Service::DeepWalker, public OPS::Visitor<TarjanNode>
		{


			OPS::Reprise::ExpressionBase* mLoop;
			int mNumber;
			SCCStack mSCCStack;
			SCCList mComponents;
			OPS::Analysis::SSAForms::SSAForm* mSSA;
			TarjanNodeFactory* mTarjanFactory;




			NodeVisitor()
			{
			}
		public:	
			const OPS::Analysis::SSAForms::SSAGenerator* mIdealSequence;
			InductionClassMap mClassMap;
			NodeVisitor(OPS::Reprise::ExpressionBase* loop, TarjanNodeFactory* TarjanFactory, OPS::Analysis::SSAForms::SSAForm* SSA);

			static int min(int m1, int m2);
			static bool contains(const OPS::Reprise::ExpressionBase* outer_loop, const OPS::Reprise::ExpressionBase* inner_loop);

			void processDescendent(TarjanNode& child, int& mLow);

			void visitPhiDescendents( TarjanNode& visited_node, int& mLow);

			void visitSSAGenerator(const OPS::Analysis::SSAForms::SSAGenerator* ssalink, int& mLow);

			void visitExprDescendents( TarjanNode& visited_node, int& mLow);

			void visitSSALinkDescendent( TarjanNode& visited_node, int& mLow);



			void visitDescendents(TarjanNode& visited_node, int& mLow);

			void classifyTrivial(TarjanNode& classified);

			void unclassifySequence(const OPS::Analysis::SSAForms::SSAGenerator* classified);

			static bool isInteger(const OPS::Reprise::TypeBase* intType);

			static long_long_t getLiteralAsInteger(const OPS::Reprise::LiteralExpression& expr);

			static OPS::Analysis::SSAForms::SSASubscript* getOuterLink(const OPS::Analysis::SSAForms::PhiFunction& Phi, OPS::Analysis::SSAForms::SSAForm* mSSA);

			bool classifyMu(TarjanNode& first_elem, LinearIntegerInductionDescription& known_class);

			bool classifyExpression(TarjanNode& exprElem, long_long_t& addition);


			void classifySequence(SCC* classified);
			bool isIdealSequence(const OPS::Analysis::SSAForms::PhiFunction* phi, const OPS::Reprise::ExpressionBase* expr);



			virtual void visit( TarjanNode& visited_node);




		};


		


	}
}





#endif
