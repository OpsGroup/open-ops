#ifndef _OPS_REPRISE_SYMBOLPREDICATEFINDER_H_INCLUDED_
#define _OPS_REPRISE_SYMBOLPREDICATEFINDER_H_INCLUDED_

#include "Reprise/Reprise.h"
#include <sstream>

namespace OPS 
{
	OPS_DEFINE_EXCEPTION_CLASS(NotImplementedError, OPS::Exception)


	class SymbolPredicate;
	class SymbolPredicateExpression;
	class SymbolPredicateLogic;
	class SymbolPredicateBool;

	///Base class for predicates
	class SymbolPredicate: public OPS::NonCopyableMix
	{
	public:
		enum SymbolPredicateType 
		{
			SPT_EXPRESSION = 1,
			SPT_LOGIC,
			SPT_BOOL,
			SPT_UNDEF
		};

	protected: 
		SymbolPredicateType m_spt;

	public:
		SymbolPredicate() { m_spt=SPT_UNDEF; }
		virtual ~SymbolPredicate() {}
		SymbolPredicateType getType() { return m_spt; }
//		void Normalize(SSAForm* ssa) { throw new NotImplementedError("Приведение в нормальную форму ещё не реализовано"); }
		virtual SymbolPredicateExpression* getAsExpression() {return 0;} 
		virtual SymbolPredicateLogic* getAsLogic() { return 0;} 
		virtual SymbolPredicateBool* getAsBool() { return 0;} 

		virtual std::string outToString() { return std::string(""); }
		virtual Reprise::ExpressionBase* outToExpression() const = 0;

		static SymbolPredicateExpression* createExpression(const Reprise::ExpressionBase* expr);
		static SymbolPredicateLogic* createAnd(SymbolPredicate* first, SymbolPredicate* second);
		static SymbolPredicateLogic* createOr(SymbolPredicate* first, SymbolPredicate* second);
		static SymbolPredicateLogic* createNot(SymbolPredicate* first);
		static SymbolPredicateBool* createBool(bool m_value);
	};

	class SymbolPredicateExpression: public SymbolPredicate
	{
	private:
		const Reprise::ExpressionBase* m_expression;
	public:
		SymbolPredicateExpression(const Reprise::ExpressionBase* exp): m_expression(exp) { m_spt=SPT_EXPRESSION;}
		const Reprise::ExpressionBase* getExpression() { return m_expression; }
		SymbolPredicateExpression* getAsExpression() {return this;}
		virtual std::string outToString() { 
			return std::string("EXPR{") + m_expression->dumpState() + "}"; 
		}
		virtual Reprise::ExpressionBase* outToExpression() const {
			return m_expression->clone();
		}
	};


	class SymbolPredicateLogic: public SymbolPredicate
	{
	public:
		enum PredicateLogicType { 
			PLT_AND = 1,
			PLT_OR,
			PLT_NOT
		};
	private:
		PredicateLogicType m_plt;
		SymbolPredicate* m_first;
		SymbolPredicate* m_second;
	public:
		SymbolPredicateLogic(PredicateLogicType plt, SymbolPredicate* first, SymbolPredicate* second = 0): m_plt(plt), m_first(first), m_second(second)
		{
			if( !first ) throw ArgumentError("SymbolPredicateLogic first argements is 0");
			if( !second && (m_plt != PLT_NOT) ) throw ArgumentError("SymbolPredicateLogic is binary and second argements is 0");
			m_spt = SPT_LOGIC;

		}
		~SymbolPredicateLogic()
		{
			delete m_first;
			if(m_second) delete m_second;
		}
		PredicateLogicType getLogicType() {return m_plt;}
		SymbolPredicate* getFirst() { return m_first; }
		SymbolPredicate* getSecond() { return m_second; }
		SymbolPredicateLogic* getAsLogic() {return this;}
		virtual std::string outToString() { 
			std::string oper;
			switch(m_plt)
			{
			case PLT_AND: oper = "AND"; break;
			case PLT_OR: oper = "OR"; break;
			case PLT_NOT: oper = "NOT"; break;
			default: OPS_ASSERT(0);
			}
			
			if(m_plt == PLT_NOT)
				return "(" + oper + " " + m_first->outToString() + ")";
			else 
				return "(" + m_first->outToString() + " " + oper + " " + m_second->outToString() + ")"; 			

		}
		virtual Reprise::ExpressionBase* outToExpression() const;
	};

	class SymbolPredicateBool: public SymbolPredicate
	{	
	private:
		bool m_value;
	public:
		SymbolPredicateBool(bool value): m_value(value) { m_spt = SPT_BOOL; }
		bool getValue() { return m_value; }
		SymbolPredicateBool* getAsBool() {return this;}
		virtual std::string outToString() { 
			std::string to_return;
			if(m_value)to_return = "TRUE";
			else to_return = "FALSE";
			return to_return;
		}
		virtual Reprise::ExpressionBase* outToExpression() const;
	};


	SymbolPredicate* getPathPredicates(const OPS::Reprise::StatementBase& end, const OPS::Reprise::BlockStatement& start_block);
	SymbolPredicate* getPathPredicates(const OPS::Reprise::StatementBase& begin, const OPS::Reprise::StatementBase& end, const OPS::Reprise::BlockStatement& enclosing_block);


}




#endif
