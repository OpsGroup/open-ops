#ifndef OPS_IR_REPRISE_STATEMENTS_H_INCLUDED__
#define OPS_IR_REPRISE_STATEMENTS_H_INCLUDED__

#include "Reprise/Common.h"
#include "Reprise/Collections.h"
#include "Reprise/Utils.h"

/*
	TODO: 
	1. Switch as list of case and blocks intermixed
	2. Goto and labels
	3. "Break from" and "continue what"

*/


namespace OPS
{
namespace Reprise
{

class ExpressionBase;
class BasicLiteralExpression;
class StrictLiteralExpression;
class BlockStatement;
class ConstStatementIterator;
class StatementIterator;
class Declarations;
class VariableDeclaration;

///	Base class for Statements in IR
class StatementBase : public RepriseBase, public IntrusiveNodeBase<StatementBase>
{
public:

	bool hasParentBlock(void) const;
	const BlockStatement& getParentBlock(void) const;
	BlockStatement& getParentBlock(void);
	const BlockStatement& getRootBlock(void) const;
	BlockStatement& getRootBlock(void);

	bool hasLabel() const;
	std::string getLabel() const;
	void setLabel(const std::string& labelName);
	std::string setUniqueLabel(const std::string& partName = "");

	virtual void replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination) = 0;

	virtual StatementBase* clone(void) const = 0;

	virtual std::string dumpState(void) const;

protected:
	StatementBase(void);
	StatementBase(const StatementBase& other);

	std::string m_label;
};

///	Statements block
class BlockStatement : public StatementBase
{
private:
	typedef IntrusiveList<StatementBase> StatementsType;
public: 
	typedef StatementsType::ConstIterator ConstIterator;
	typedef StatementsType::Iterator Iterator;

	BlockStatement(void);

	bool isEmpty(void) const;

	ConstIterator getFirst(void) const;
	ConstIterator getLast(void) const;

	Iterator getFirst(void);
	Iterator getLast(void);

	Iterator addFirst(StatementBase* const statement);
	Iterator addLast(StatementBase* const statement);

	Iterator addBefore(const Iterator& iterator, StatementBase* const statement);
	Iterator addAfter(const Iterator& iterator, StatementBase* const statement);

	bool hasStatement(const StatementBase* const statement) const;

	Iterator convertToIterator(StatementBase* const statement);
	ConstIterator convertToIterator(const StatementBase* statement) const;

//	void moveBefore(Iterator& source, Iterator& destination);

	void erase(const Iterator& iterator);

	void erase(StatementBase* const statement);

	Iterator replace(const Iterator& iterator, StatementBase* destination);

//	void clearPreserve(void);

	const Declarations& getDeclarations(void) const;
	Declarations& getDeclarations(void);

/*	int getStatementCount(void) const;

	const StatementBase& getStatement(int index) const;
	StatementBase& getStatement(int index);

	void add(StatementBase* statement);
	void insert(int index, StatementBase* statement);
*/

	virtual void replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(BlockStatement)

protected:
	BlockStatement(const BlockStatement& other);

private:

	StatementsType m_statements;
};

///	For statement
class ForStatement : public StatementBase
{
public:
	ForStatement(void);
	ForStatement(ExpressionBase* init, ExpressionBase* final, ExpressionBase* step);

	const ExpressionBase& getInitExpression(void) const;
	ExpressionBase& getInitExpression(void);
	void setInitExpression(ExpressionBase* initExpression);

	const ExpressionBase& getFinalExpression(void) const;
	ExpressionBase& getFinalExpression(void);
	void setFinalExpression(ExpressionBase* finalExpression);

	const ExpressionBase& getStepExpression(void) const;
	ExpressionBase& getStepExpression(void);
	void setStepExpression(ExpressionBase* stepExpression);

	const BlockStatement& getBody(void) const;
	BlockStatement& getBody(void);
	void setBody(BlockStatement* body);

	virtual void replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(ForStatement)

protected:
	ForStatement(const ForStatement& other);

private:
	ReprisePtr<ExpressionBase> m_init;
	ReprisePtr<ExpressionBase> m_final;
	ReprisePtr<ExpressionBase> m_step;
	ReprisePtr<BlockStatement> m_body;
};

///	while/do while statement
class WhileStatement : public StatementBase
{
public:
	explicit WhileStatement(bool preCondition);
	WhileStatement(bool preCondition, ExpressionBase* condition);

	bool isPreCondition(void) const;
	void setPreCondition(bool preCondition);

	const ExpressionBase& getCondition(void) const;
	ExpressionBase& getCondition(void);
	void setCondition(ExpressionBase* condition);

	const BlockStatement& getBody(void) const;
	BlockStatement& getBody(void);
	void setBody(BlockStatement* body);

	virtual void replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(WhileStatement)

protected:
	WhileStatement(const WhileStatement& other);

private:
	bool m_preCondition;
	ReprisePtr<ExpressionBase> m_condition;
	ReprisePtr<BlockStatement> m_body;
};

///	if statement
class IfStatement : public StatementBase
{
public:
	IfStatement(void);
	IfStatement(ExpressionBase* condition);

	const ExpressionBase& getCondition(void) const;
	ExpressionBase& getCondition(void);
	void setCondition(ExpressionBase* condition);

	const BlockStatement& getThenBody(void) const;
	BlockStatement& getThenBody(void);
	void setThenBody(BlockStatement* body);

	const BlockStatement& getElseBody(void) const;
	BlockStatement& getElseBody(void);
	void setElseBody(BlockStatement* body);

	virtual void replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(IfStatement)

protected:
	IfStatement(const IfStatement& other);

private:
	ReprisePtr<ExpressionBase> m_condition;
	ReprisePtr<BlockStatement> m_thenBody;
	ReprisePtr<BlockStatement> m_elseBody;
};

///	Case variant 
/*
class CaseVariantDescriptor : public RepriseBase
{
public:
	CaseVariantDescriptor(void);
	explicit CaseVariantDescriptor(ExpressionBase* expression);
	CaseVariantDescriptor(ExpressionBase* expression, BlockStatement* body);

	int getVariantCount(void) const;
	const ExpressionBase& getVariant(int index) const;
	ExpressionBase& getVariant(int index);
	void addVariant(ExpressionBase* expression);
	void insertVariant(int index, ExpressionBase* expression);
	void replaceVariant(int index, ReprisePtr<ExpressionBase> expression);

	const BlockStatement& getBody(void) const;
	BlockStatement& getBody(void);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(CaseVariantDescriptor)

protected:
	CaseVariantDescriptor(const CaseVariantDescriptor& other);

private:
	typedef RepriseList<ExpressionBase> VariantsType;

	ReprisePtr<BlockStatement> m_body;
	VariantsType m_variants;
};

///	switch statement
class SwitchStatement : public StatementBase
{
public:
	SwitchStatement(void);
	SwitchStatement(ExpressionBase* condition);

	const ExpressionBase& getCondition(void) const;
	ExpressionBase& getCondition(void);
	void setCondition(ExpressionBase* condition);

	int getCaseCount(void) const;
	const CaseVariantDescriptor& getCase(int index) const;
	CaseVariantDescriptor& getCase(int index);
	void addCase(CaseVariantDescriptor* descriptor);
	void insertCase(int index, CaseVariantDescriptor* descriptor);

	const BlockStatement& getDefaultBody(void) const;
	BlockStatement& getDefaultBody(void);
	void setDefaultBody(BlockStatement* body);

	virtual void replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(SwitchStatement)

protected:
	SwitchStatement(const SwitchStatement& other);

private:
//	Types
	typedef RepriseList<CaseVariantDescriptor> CasesType;

//	Members
	ReprisePtr<ExpressionBase> m_condition;
	ReprisePtr<BlockStatement> m_defaultBody;
	CasesType m_cases;
};*/

/// Plain case label
class PlainCaseLabel : public RepriseBase
{
public:
	PlainCaseLabel();
	explicit PlainCaseLabel(sqword value);
	PlainCaseLabel(sqword value, StatementBase* statement);

	sqword getValue() const;
	void setValue(sqword value);

	bool isDefault() const;
	void setDefault(bool defaultState);

	const StatementBase& getStatement(void) const;
	StatementBase& getStatement(void);
	void setStatement(StatementBase* statement);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual int getLinkCount(void) const;
	virtual RepriseBase& getLink(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(PlainCaseLabel)

private:
	sqword m_value;
	bool m_default;
	RepriseWeakPtr<StatementBase> m_statement;
};

///	plain switch statement
class PlainSwitchStatement : public StatementBase
{
public:
	PlainSwitchStatement(void);
	explicit PlainSwitchStatement(ExpressionBase* condition);

	const ExpressionBase& getCondition(void) const;
	ExpressionBase& getCondition(void);
	void setCondition(ExpressionBase* condition);

	const BlockStatement& getBody(void) const;
	BlockStatement& getBody(void);
	void setBody(BlockStatement* body);

	int getLabelCount(void) const;
	const PlainCaseLabel& getLabel(int index) const;
	PlainCaseLabel& getLabel(int index);
	void addLabel(PlainCaseLabel* caseLabel);
	void insertLabel(int index, PlainCaseLabel* caseLabel);

	virtual void replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(PlainSwitchStatement)

protected:
	PlainSwitchStatement(const PlainSwitchStatement& other);

private:
//	Types
	typedef RepriseList<PlainCaseLabel> CaseLabels;

//	Members
	ReprisePtr<ExpressionBase> m_condition;
	ReprisePtr<BlockStatement> m_block;
	CaseLabels m_labels;
};

///	goto statement
class GotoStatement : public StatementBase
{
public:
	GotoStatement();
	GotoStatement(StatementBase* pointedStatement);

	StatementBase* getPointedStatement(void) const;
	void setPointedStatement(StatementBase* pointedStatement);

	virtual void replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);
	virtual int getLinkCount(void) const;
	virtual RepriseBase& getLink(int index);
	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(GotoStatement)

private:
	RepriseWeakPtr<StatementBase> m_pointed;
};

///	return statement
class ReturnStatement : public StatementBase
{
public:
	ReturnStatement(void);
	explicit ReturnStatement(ExpressionBase* returnExpression);

	const ExpressionBase& getReturnExpression(void) const;
	ExpressionBase& getReturnExpression(void);
	void setReturnExpression(ExpressionBase* returnExpression);

	virtual void replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(ReturnStatement)

protected:
	ReturnStatement(const ReturnStatement& other);

private:
	ReprisePtr<ExpressionBase> m_return;
};

///	expression statement
class ExpressionStatement : public StatementBase
{
public:
	explicit ExpressionStatement(ExpressionBase* const expression);

	const ExpressionBase& get(void) const;
	ExpressionBase& get(void);

	void set(ExpressionBase* const expression);

	virtual void replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(ExpressionStatement)

protected:
	ExpressionStatement(const ExpressionStatement& other);

private:
	ReprisePtr<ExpressionBase> m_expression;
};

///	inline assembler statement
class ASMStatement : public StatementBase
{
public:
	enum InlineASMType {
		ASMTP_MS,
		ASMTP_GCC
	};

	explicit ASMStatement(std::string inlineASMString, InlineASMType type);

	std::string getASMString(void) const;
	
	InlineASMType getASMType() const;
	void setASMType(InlineASMType t);

	void setASMString(std::string inlineASMString);

	virtual void replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(ASMStatement)

protected:
	ASMStatement(const ASMStatement& other);

private:
	std::string m_ASMString;
	InlineASMType m_ASMType;
};


///	empty statement
class EmptyStatement : public StatementBase
{
public:
	EmptyStatement(void);
	explicit EmptyStatement(const std::string& note);

	virtual void replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(EmptyStatement)

private:
	std::string m_note;
};

}
}

#endif                      // OPS_IR_REPRISE_STATEMENTS_H_INCLUDED__
