#include "Reprise/Statements.h"
#include "Reprise/Exceptions.h"
#include "Reprise/Expressions.h"

//	Enter namespace
namespace OPS
{
namespace Reprise
{

//	StatementBase class implementation
StatementBase::StatementBase(void)
{
}

StatementBase::StatementBase(const StatementBase& other) : RepriseBase(other), m_label(other.m_label)
{

}

bool StatementBase::hasParentBlock(void) const
{
	RepriseBase* parent = getParent();
	return dynamic_cast<BlockStatement*>(parent) != 0;
}

const BlockStatement& StatementBase::getParentBlock(void) const
{
	RepriseBase* const parent = getParent();
	BlockStatement* const parentBlock = dynamic_cast<BlockStatement*>(parent);
	if (parentBlock != 0)
		return *parentBlock;
	else
		throw RepriseError("Unexpected getting of parent block.");
}

BlockStatement& StatementBase::getParentBlock(void)
{
	RepriseBase* const parent = getParent();
	BlockStatement* const parentBlock = dynamic_cast<BlockStatement*>(parent);
	if (parentBlock != 0)
		return *parentBlock;
	else
		throw RepriseError("Unexpected getting of parent block.");
}

const BlockStatement& StatementBase::getRootBlock(void) const
{
	return const_cast<StatementBase*>(this)->getRootBlock();
}

BlockStatement& StatementBase::getRootBlock(void)
{
	BlockStatement* rootBlock = dynamic_cast<BlockStatement*>(this);
    RepriseBase* tryNode = this;
	for (; ;)
	{
        RepriseBase* parent = tryNode->getParent();
		BlockStatement* block = dynamic_cast<BlockStatement*>(parent);
		if (block != 0)
		{
			rootBlock = block;
            tryNode = block;
		}
		else
		{
            if (parent != 0 &&
                (parent->is_a<StatementBase>() ||
                 parent->is_a<ExpressionBase>()))
			{
                tryNode = parent;
			}
			else
			{
				if (rootBlock == 0)
					throw RepriseError("Unexpected getting of root block.");
				return *rootBlock;
			}
		}
	}
}

bool StatementBase::hasLabel() const
{
	return !m_label.empty();
}

std::string StatementBase::getLabel() const
{
	return m_label;
}

void StatementBase::setLabel(const std::string& labelName)
{
	m_label = labelName;
}

std::string StatementBase::setUniqueLabel(const std::string& partName)
{
	const std::string& labelName = generateUniqueIndentifier("l" + partName);
	setLabel(labelName);
	return labelName;
}

std::string StatementBase::dumpState(void) const
{
	if (hasLabel())
	{
		return m_label + ": ";
	}
	else
	{
		return "";
	}
}

//	BlockStatement class implementation
BlockStatement::BlockStatement(void) 
{
}

BlockStatement::BlockStatement(const BlockStatement& other) : StatementBase(other)
{
	for (ConstIterator statement = other.getFirst(); statement.isValid(); statement.goNext())
	{
		StatementBase* newStatement = statement->clone();
		addLast(newStatement);
	}
}

bool BlockStatement::isEmpty(void) const
{
	return m_statements.isEmpty();
}

BlockStatement::ConstIterator BlockStatement::getFirst(void) const
{
	return m_statements.getFirst();
}

BlockStatement::ConstIterator BlockStatement::getLast(void) const
{
	return m_statements.getLast();
}

BlockStatement::Iterator BlockStatement::getFirst(void)
{
	return m_statements.getFirst();
}

BlockStatement::Iterator BlockStatement::getLast(void)
{
	return m_statements.getLast();
}

BlockStatement::Iterator BlockStatement::addFirst(StatementBase* const statement)
{
	statement->setParent(this);
	return m_statements.addFirst(statement);
}

BlockStatement::Iterator BlockStatement::addLast(StatementBase* const statement)
{
	statement->setParent(this);
	return m_statements.addLast(statement);
}

BlockStatement::Iterator BlockStatement::addBefore(const BlockStatement::Iterator& iterator, StatementBase* const statement)
{
	statement->setParent(this);
	return m_statements.addBefore(iterator, statement);
}

BlockStatement::Iterator BlockStatement::addAfter(const BlockStatement::Iterator& iterator, StatementBase* const statement)
{
	statement->setParent(this);
	return m_statements.addAfter(iterator, statement);
}

bool BlockStatement::hasStatement(const StatementBase* const statement) const
{
	OPS_ASSERT(statement != 0)
	return statement->getParent() == this;
}

BlockStatement::Iterator BlockStatement::convertToIterator(StatementBase* const statement)
{
	OPS_ASSERT(statement != 0)
	return m_statements.convertToIterator(statement);
}

BlockStatement::ConstIterator BlockStatement::convertToIterator(const StatementBase* statement) const
{
	OPS_ASSERT(statement != 0)
	return m_statements.convertToIterator(statement);
}

/*
void BlockStatement::moveBefore(BlockStatement::Iterator& source, BlockStatement::Iterator& destination)
{
	m_statements.moveBefore(source, destination);
}
*/

void BlockStatement::erase(const Iterator& iterator)
{
	Iterator temp(iterator);
	m_statements.remove(temp);
}

void BlockStatement::erase(StatementBase* const statement)
{
	Iterator tempIterator(convertToIterator(statement));
	if (!tempIterator.isValid())
	{
		throw RepriseError("Unexpected statement passed to erase().");
	}
	m_statements.remove(tempIterator);
}

BlockStatement::Iterator BlockStatement::replace(const Iterator& iterator, StatementBase* destination)
{
	Iterator newStmtIter = addBefore(iterator, destination);
	erase(iterator);
	return newStmtIter;
}

/*
void BlockStatement::clearPreserve(void)
{
	m_statements.clearPreserve();
}
*/

const Declarations& BlockStatement::getDeclarations(void) const
{
	const BlockStatement& rootBlock = getRootBlock();
	
	SubroutineDeclaration* subroutine = dynamic_cast<SubroutineDeclaration*>(rootBlock.getParent());	
	if (subroutine != 0)
	{
		return subroutine->getDeclarations();
	}
	else
		throw RepriseError("Could not get SubroutineBody.");
}

Declarations& BlockStatement::getDeclarations(void)
{
	BlockStatement& rootBlock = getRootBlock();
	SubroutineDeclaration* subroutine = dynamic_cast<SubroutineDeclaration*>(rootBlock.getParent());	
	if (subroutine != 0)
	{
		return subroutine->getDeclarations();
	}
	else
		throw RepriseError("Could not get SubroutineBody.");
}

/*
int BlockStatement::getStatementCount(void) const
{
	return static_cast<int>(m_statements.size());
}

const StatementBase& BlockStatement::getStatement(const int index) const
{
	if (index < 0 || index >= getStatementCount())
		throw RepriseError(Strings::format("Unexpected getting %i statement of %i.", index, getStatementCount()));
	return m_statements[index];
}

StatementBase& BlockStatement::getStatement(const int index)
{
	if (index < 0 || index >= getStatementCount())
		throw RepriseError(Strings::format("Unexpected getting %i statement of %i.", index, getStatementCount()));
	return m_statements[index];
}

void BlockStatement::add(StatementBase* const statement)
{
	m_statements.add(statement).setParent(this);
}

void BlockStatement::insert(const int index, StatementBase* const statement)
{
	if (index < 0 || index >= getStatementCount())
		throw RepriseError(Strings::format("Unexpected getting %i statement of %i.", index, getStatementCount()));
	m_statements.insert(index, statement).setParent(this);
}
*/

void BlockStatement::replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	OPS_UNUSED(source)
	OPS_UNUSED(destination)
	throw RepriseError("Unexpected expression replacement in Block.");
}


//		RepriseBase implementation
int BlockStatement::getChildCount(void) const
{
	int count = 0;
	for (ConstIterator statement = getFirst(); statement.isValid(); statement.goNext())
		++count;
	return count;
}

RepriseBase& BlockStatement::getChild(const int index)
{
	int count = 0;
	for (Iterator statement = getFirst(); statement.isValid(); statement.goNext())
	{
		if (index == count)
			return *statement;
		++count;
	}
	throw UnexpectedChildError("BlockStatement::getChild");
}

std::string BlockStatement::dumpState(void) const
{
	std::string state = StatementBase::dumpState();
	state += "{\n";
	for (ConstIterator statement = getFirst(); statement.isValid(); statement.goNext())
	{
		state += statement->dumpState() + "\n";
	}
	state += "}\n";
	return state;
}

//	ForStatement class implementation
ForStatement::ForStatement(void) : 
	m_init(EmptyExpression::empty()),
	m_final(EmptyExpression::empty()),
	m_step(EmptyExpression::empty()),
	m_body(new BlockStatement())
{
	m_init->setParent(this);
	m_final->setParent(this);
	m_step->setParent(this);
	m_body->setParent(this);
}

ForStatement::ForStatement(ExpressionBase* init, ExpressionBase* final, ExpressionBase* step) :
	m_init(init), m_final(final), m_step(step),
	m_body(new BlockStatement())
{
	m_init->setParent(this);
	m_final->setParent(this);
	m_step->setParent(this);
	m_body->setParent(this);
}

ForStatement::ForStatement(const ForStatement& other) 
	: StatementBase(other), m_init(other.m_init->clone()), m_final(other.m_final->clone()), 
	m_step(other.m_step->clone()),	m_body(other.m_body->clone())
{
	m_init->setParent(this);
	m_final->setParent(this);
	m_step->setParent(this);
	m_body->setParent(this);
}

const ExpressionBase& ForStatement::getInitExpression(void) const
{
	return *m_init;
}

ExpressionBase& ForStatement::getInitExpression(void)
{
	return *m_init;
}

void ForStatement::setInitExpression(ExpressionBase* const initExpression)
{
	m_init.reset(initExpression);
	m_init->setParent(this);
}

const ExpressionBase& ForStatement::getFinalExpression(void) const
{
	return *m_final;
}

ExpressionBase& ForStatement::getFinalExpression(void)
{
	return *m_final;
}

void ForStatement::setFinalExpression(ExpressionBase* const finalExpression)
{
	m_final.reset(finalExpression);
	m_final->setParent(this);
}

const ExpressionBase& ForStatement::getStepExpression(void) const
{
	return *m_step;
}

ExpressionBase& ForStatement::getStepExpression(void)
{
	return *m_step;
}

void ForStatement::setStepExpression(ExpressionBase* const stepExpression)
{
	m_step.reset(stepExpression);
	m_step->setParent(this);
}

const BlockStatement& ForStatement::getBody(void) const
{
	return *m_body;
}

BlockStatement& ForStatement::getBody(void)
{
	return *m_body;
}

void ForStatement::setBody(BlockStatement* const body)
{
	m_body.reset(body);
	m_body->setParent(this);
}

void ForStatement::replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	if (m_init.get() == &source)
	{
		m_init = destination;
		m_init->setParent(this);
	}
	else
	if (m_final.get() == &source)
	{
		m_final = destination;
		m_final->setParent(this);
	}
	else
	if (m_step.get() == &source)
	{
		m_step = destination;
		m_step->setParent(this);
	}
	else
		throw RepriseError(Strings::format("Unexpected replace expression (%p) in ForStatement.", &source));
}

//		ForStatement - RepriseBase implementation
int ForStatement::getChildCount(void) const
{
	return 4;
}

RepriseBase& ForStatement::getChild(const int index)
{
	switch (index)
	{
	case 0:
		return *m_init;
	case 1:
		return *m_final;
	case 2:
		return *m_step;
	case 3:
		return *m_body;
		OPS_DEFAULT_CASE_LABEL
	}
	throw UnexpectedChildError("ForStatement::getChild");
}

std::string ForStatement::dumpState(void) const
{
	std::string state = StatementBase::dumpState();
	state += Strings::format("for (%s; %s; %s)\n", 
		m_init->dumpState().c_str(), m_final->dumpState().c_str(), m_step->dumpState().c_str());
	state += m_body->dumpState();
	return state;
}

//	WhileStatement class implementation
WhileStatement::WhileStatement(const bool preCondition) :
	m_preCondition(preCondition),
	m_condition(EmptyExpression::empty()),
	m_body(new BlockStatement())
{
	m_body->setParent(this);
}

WhileStatement::WhileStatement(const bool preCondition, ExpressionBase* const condition) :
	m_preCondition(preCondition),
	m_condition(condition),
	m_body(new BlockStatement())
{
	m_condition->setParent(this);
	m_body->setParent(this);
}

WhileStatement::WhileStatement(const WhileStatement& other) 
	: StatementBase(other), 
	m_preCondition(other.m_preCondition), 
	m_condition(other.m_condition->clone()), 
	m_body(other.m_body->clone())
{
	m_condition->setParent(this);
	m_body->setParent(this);
}

bool WhileStatement::isPreCondition(void) const
{
	return m_preCondition;
}

void WhileStatement::setPreCondition(const bool preCondition)
{
	m_preCondition = preCondition;
}

const ExpressionBase& WhileStatement::getCondition(void) const
{
	return *m_condition;
}

ExpressionBase& WhileStatement::getCondition(void)
{
	return *m_condition;
}

void WhileStatement::setCondition(ExpressionBase* const condition)
{
	m_condition.reset(condition);
	m_condition->setParent(this);
}

const BlockStatement& WhileStatement::getBody(void) const
{
	return *m_body;
}

BlockStatement& WhileStatement::getBody(void)
{
	return *m_body;
}

void WhileStatement::setBody(BlockStatement* const body)
{
	m_body.reset(body);
	m_body->setParent(this);
}

void WhileStatement::replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	if (m_condition.get() == &source)
	{
		m_condition = destination;
		m_condition->setParent(this);
	}
	else
		throw RepriseError(Strings::format("Unexpected replace expression (%p) in WhileStatement.", &source));
}

//		WhileStatement - RepriseBase implementation
int WhileStatement::getChildCount(void) const
{
	return 2;
}

RepriseBase& WhileStatement::getChild(const int index)
{
	switch (index)
	{
	case 0:
		return *m_condition;
	case 1:
		return *m_body;
	}
	throw UnexpectedChildError("WhileStatement::getChild");
}

std::string WhileStatement::dumpState(void) const
{
	std::string state = StatementBase::dumpState();
	if (m_preCondition)
	{
		state += "while (" + m_condition->dumpState() + ")\n";
		state += m_body->dumpState();
	}
	else
	{
		state += "do\n";
		state += m_body->dumpState();
		state += "while (" + m_condition->dumpState() + ");\n";
	}
	return state;
}

//	IfStatement class implementation
IfStatement::IfStatement(void) :
	m_condition(EmptyExpression::empty()),
	m_thenBody(new BlockStatement()),
	m_elseBody(new BlockStatement())
{
	m_condition->setParent(this);
	m_thenBody->setParent(this);
	m_elseBody->setParent(this);
}

IfStatement::IfStatement(ExpressionBase* const condition) : 
	m_condition(condition),
	m_thenBody(new BlockStatement()),
	m_elseBody(new BlockStatement())
{
	m_condition->setParent(this);
	m_thenBody->setParent(this);
	m_elseBody->setParent(this);
}

IfStatement::IfStatement(const IfStatement& other) : 
	StatementBase(other), 
	m_condition(other.m_condition->clone()),
	m_thenBody(other.m_thenBody->clone()),
	m_elseBody(other.m_elseBody->clone())
{
	m_condition->setParent(this);
	m_thenBody->setParent(this);
	m_elseBody->setParent(this);
}

const ExpressionBase& IfStatement::getCondition(void) const
{
	return *m_condition;
}

ExpressionBase& IfStatement::getCondition(void)
{
	return *m_condition;
}

void IfStatement::setCondition(ExpressionBase* const condition)
{
	m_condition.reset(condition);
	m_condition->setParent(this);
}

const BlockStatement& IfStatement::getThenBody(void) const
{
	return *m_thenBody;
}

BlockStatement& IfStatement::getThenBody(void)
{
	return *m_thenBody;
}

void IfStatement::setThenBody(BlockStatement* const body)
{
	m_thenBody.reset(body);
	m_thenBody->setParent(this);
}

const BlockStatement& IfStatement::getElseBody(void) const
{
	return *m_elseBody;
}

BlockStatement& IfStatement::getElseBody(void)
{
	return *m_elseBody;
}

void IfStatement::setElseBody(BlockStatement* const body)
{
	m_elseBody.reset(body);
	m_elseBody->setParent(this);
}

void IfStatement::replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	if (m_condition.get() == &source)
	{
		m_condition = destination;
		m_condition->setParent(this);
	}
	else
		throw RepriseError(Strings::format("Unexpected replace expression (%p) in IfStatement.", &source));
}

//		IfStatement - RepriseBase implementation
int IfStatement::getChildCount(void) const
{
	return 3;
}

RepriseBase& IfStatement::getChild(const int index)
{
	switch (index)
	{
	case 0:
		return *m_condition;
	case 1:
		return *m_thenBody;
	case 2:
		return *m_elseBody;
	}
	throw UnexpectedChildError("IfStatement::getChild");
}

std::string IfStatement::dumpState(void) const
{
	std::string state = StatementBase::dumpState();
	state += "if (" + m_condition->dumpState() + ")\n";
	state += m_thenBody->dumpState();
	state += "else\n";
	state += m_elseBody->dumpState();
	return state;
}

//	CaseVariantDescriptor class implementation
/*
CaseVariantDescriptor::CaseVariantDescriptor(void) 
	: m_body(new BlockStatement())
{
	m_body->setParent(this);
}

CaseVariantDescriptor::CaseVariantDescriptor(ExpressionBase* const expression) 
	: m_body(new BlockStatement())
{
	OPS_ASSERT(expression != 0)
	m_body->setParent(this);
	addVariant(expression);
}

CaseVariantDescriptor::CaseVariantDescriptor(ExpressionBase* const expression, BlockStatement* const body)
{
	OPS_ASSERT(expression != 0)
	OPS_ASSERT(body != 0)
	m_body.reset(body);
	m_body->setParent(this);
	addVariant(expression);
}

CaseVariantDescriptor::CaseVariantDescriptor(const CaseVariantDescriptor& other) : RepriseBase(other), m_body(other.m_body->clone())
{
	m_body->setParent(this);
	for (VariantsType::ConstIterator iter = other.m_variants.begin(); iter != other.m_variants.end(); ++iter)
	{
		addVariant((*iter)->clone());
	}
}

int CaseVariantDescriptor::getVariantCount(void) const
{
	return static_cast<int>(m_variants.size());
}

const ExpressionBase& CaseVariantDescriptor::getVariant(const int index) const
{
	if (index < 0 || index >= getVariantCount())
		throw RepriseError(Strings::format("Getting unexpected variant (%i), variant count (%i).",
			index, getVariantCount()));
	return m_variants[index];
}

ExpressionBase& CaseVariantDescriptor::getVariant(const int index)
{
	if (index < 0 || index >= getVariantCount())
		throw RepriseError(Strings::format("Getting unexpected variant (%i), variant count (%i).",
			index, getVariantCount()));
	return m_variants[index];
}

void CaseVariantDescriptor::addVariant(ExpressionBase* expression)
{
	m_variants.add(expression).setParent(this);
}

void CaseVariantDescriptor::insertVariant(int index, ExpressionBase* expression)
{
	if (index < 0 || index > getVariantCount())
		throw RepriseError(Strings::format("Getting unexpected variant (%i), variant count (%i).",
			index, getVariantCount()));
	m_variants.insert(index, expression).setParent(this);
}

void CaseVariantDescriptor::replaceVariant(int index, ReprisePtr<ExpressionBase> expression)
{
	if (index < 0 || index > getVariantCount())
		throw RepriseError(Strings::format("Getting unexpected variant (%i), variant count (%i).",
			index, getVariantCount()));
	m_variants[index] = *expression;	
}

const BlockStatement& CaseVariantDescriptor::getBody(void) const
{
	return *m_body;
}

BlockStatement& CaseVariantDescriptor::getBody(void)
{
	return *m_body;
}

//		CaseVariantDescriptor - RepriseBase implementation
int CaseVariantDescriptor::getChildCount(void) const
{
	return 1 + getVariantCount();
}

RepriseBase& CaseVariantDescriptor::getChild(const int index)
{
	if (index == 0)
		return *m_body;
	if (index > 0 && index - 1 < getVariantCount())
		return m_variants[index - 1];
	throw UnexpectedChildError("CaseVariantDescriptor::getChild");
}

std::string CaseVariantDescriptor::dumpState(void) const
{
	std::string state = RepriseBase::dumpState();
	for (int index = 0; index < getVariantCount(); ++index)
	{
		state += "case (" + getVariant(index).dumpState() + "):\n";
	}
	state += m_body->dumpState();
	return state;
}

//	Switch statement
SwitchStatement::SwitchStatement(void) : m_condition(new EmptyExpression()), m_defaultBody(new BlockStatement()) 
{
	m_condition->setParent(this);
	m_defaultBody->setParent(this);
}

SwitchStatement::SwitchStatement(ExpressionBase* const condition) : m_condition(condition), m_defaultBody(new BlockStatement())
{
	m_condition->setParent(this);
	m_defaultBody->setParent(this);
}

SwitchStatement::SwitchStatement(const SwitchStatement& other) : StatementBase(other), m_condition(other.m_condition->clone()), 
	m_defaultBody(other.m_defaultBody->clone())
{
	m_condition->setParent(this);
	m_defaultBody->setParent(this);
	for (CasesType::ConstIterator iter = other.m_cases.begin(); iter != other.m_cases.end(); ++iter)
	{
		addCase((*iter)->clone());
	}
}

const ExpressionBase& SwitchStatement::getCondition(void) const
{
	return *m_condition;
}

ExpressionBase& SwitchStatement::getCondition(void)
{
	return *m_condition;
}

void SwitchStatement::setCondition(ExpressionBase* condition)
{
	m_condition.reset(condition);
	m_condition->setParent(this);
}

int SwitchStatement::getCaseCount(void) const
{
	return static_cast<int>(m_cases.size());
}

const CaseVariantDescriptor& SwitchStatement::getCase(const int index) const
{
	if (index < 0 || index >= getCaseCount())
		throw RepriseError(Strings::format("Getting unexpected case (%i), case count (%i).",
			index, getCaseCount()));
	return m_cases[index];
}

CaseVariantDescriptor& SwitchStatement::getCase(const int index)
{
	if (index < 0 || index >= getCaseCount())
		throw RepriseError(Strings::format("Getting unexpected case (%i), case count (%i).",
			index, getCaseCount()));
	return m_cases[index];
}

void SwitchStatement::addCase(CaseVariantDescriptor* const descriptor)
{
	m_cases.add(descriptor).setParent(this);
}

void SwitchStatement::insertCase(const int index, CaseVariantDescriptor* const descriptor)
{
	if (index < 0 || index > getCaseCount())
		throw RepriseError(Strings::format("Getting unexpected case (%i), case count (%i).",
			index, getCaseCount()));
	m_cases.insert(index, descriptor).setParent(this);
}


const BlockStatement& SwitchStatement::getDefaultBody(void) const
{
	return *m_defaultBody;
}

BlockStatement& SwitchStatement::getDefaultBody(void)
{
	return *m_defaultBody;
}

void SwitchStatement::setDefaultBody(BlockStatement* body)
{
	m_defaultBody.reset(body);
	m_defaultBody->setParent(this);
}

void SwitchStatement::replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	if (m_condition.get() == &source)
	{
		m_condition = destination;
		m_condition->setParent(this);
	}
	else
	{
		for (CasesType::Iterator iter = m_cases.begin(); iter != m_cases.end(); ++iter)
		{
			CaseVariantDescriptor& descriptor = **iter;
			for (int index = 0; index < descriptor.getVariantCount(); ++index)
			{
				if (&descriptor.getVariant(index) == &source)
				{
					descriptor.replaceVariant(index, destination);
					return;
				}
			}
		}
		throw RepriseError(Strings::format("Unexpected replace expression (%p) in SwitchStatement.", &source));
	}
}

//		SwitchStatement - RepriseBase implementation
int SwitchStatement::getChildCount(void) const
{
	return 2 + getCaseCount();
}

RepriseBase& SwitchStatement::getChild(const int index)
{
	if (index == 0)
		return *m_condition;
	if (index == 1)
		return *m_defaultBody;
	if (index > 1 && index < getChildCount())
		return getCase(index - 2);
	throw UnexpectedChildError("SwitchStatement::getChild");
}

std::string SwitchStatement::dumpState(void) const
{
	std::string state = StatementBase::dumpState();
	state += "switch (" + m_condition->dumpState() + ")\n";
	state += "{";
	for (int index = 0; index < getCaseCount(); ++index)
	{
		state += getCase(index).dumpState();
	}
	state += "default:\n";
	state += m_defaultBody->dumpState();
	return state;
}
*/

//	PlainCaseLabel class implementation
PlainCaseLabel::PlainCaseLabel() 
	: m_value(0), m_default(true)
{
}

PlainCaseLabel::PlainCaseLabel(const sqword value)
	: m_value(value), m_default(false)
{
}

PlainCaseLabel::PlainCaseLabel(const sqword value, StatementBase* statement)
	: m_value(value), m_default(false), m_statement(statement)
{
}

sqword PlainCaseLabel::getValue() const
{
	return m_value;
}

void PlainCaseLabel::setValue(sqword value)
{
	m_value = value;
}

bool PlainCaseLabel::isDefault() const
{
	return m_default;
}

void PlainCaseLabel::setDefault(bool defaultState)
{
	m_default = defaultState;
}

const StatementBase& PlainCaseLabel::getStatement(void) const
{
	return *m_statement;
}

StatementBase& PlainCaseLabel::getStatement(void)
{
	return *m_statement;
}

void PlainCaseLabel::setStatement(StatementBase* statement)
{
	m_statement.reset(statement);
}

//		PlainCaseLabel - RepriseBase implementation
int PlainCaseLabel::getChildCount(void) const
{
	return 0;
}

RepriseBase& PlainCaseLabel::getChild(int index)
{
	OPS_UNUSED(index);
	throw UnexpectedChildError("PlainCaseLabel::getChild");
}

int PlainCaseLabel::getLinkCount(void) const
{
	return 1;
}

RepriseBase& PlainCaseLabel::getLink(int index)
{
	if (index < 0 || index >= getLinkCount())
		throw RepriseError("PlainCaseLabel::getLink()");
	OPS_ASSERT(m_statement.get() != 0)
	return *m_statement;
}

std::string PlainCaseLabel::dumpState(void) const
{
	return RepriseBase::dumpState();
}


//	PlainSwitchStatement class implementation
PlainSwitchStatement::PlainSwitchStatement(void) : m_condition(new EmptyExpression()), m_block(new BlockStatement())
{
	m_condition->setParent(this);
	m_block->setParent(this);
}

PlainSwitchStatement::PlainSwitchStatement(ExpressionBase* condition) : m_condition(condition), m_block(new BlockStatement())
{
	m_condition->setParent(this);
	m_block->setParent(this);
}

PlainSwitchStatement::PlainSwitchStatement(const PlainSwitchStatement& other) : 
	StatementBase(other),
	m_condition(other.m_condition->clone()),
	m_block(other.m_block->clone())
{
	m_condition->setParent(this);
	m_block->setParent(this);
	for (CaseLabels::ConstIterator iter = other.m_labels.begin(); iter != other.m_labels.end(); ++iter)
	{
		addLabel((*iter)->clone());
	}
}


const ExpressionBase& PlainSwitchStatement::getCondition(void) const
{
	return *m_condition;
}

ExpressionBase& PlainSwitchStatement::getCondition(void)
{
	return *m_condition;
}

void PlainSwitchStatement::setCondition(ExpressionBase* condition)
{
	m_condition.reset(condition);
	m_condition->setParent(this);
}

const BlockStatement& PlainSwitchStatement::getBody(void) const
{
	return *m_block;
}

BlockStatement& PlainSwitchStatement::getBody(void)
{
	return *m_block;
}

void PlainSwitchStatement::setBody(BlockStatement* body)
{
	m_block.reset(body);
	m_block->setParent(this);
}

int PlainSwitchStatement::getLabelCount(void) const
{
	return m_labels.size();
}

const PlainCaseLabel& PlainSwitchStatement::getLabel(int index) const
{
	return m_labels[index];
}

PlainCaseLabel& PlainSwitchStatement::getLabel(int index)
{
	return m_labels[index];
}

void PlainSwitchStatement::addLabel(PlainCaseLabel* caseLabel)
{
	m_labels.add(caseLabel).setParent(this);
}

void PlainSwitchStatement::insertLabel(int index, PlainCaseLabel* caseLabel)
{
	m_labels.insert(index, caseLabel).setParent(this);
}


void PlainSwitchStatement::replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	if (m_condition.get() == &source)
	{
		m_condition = destination;
		m_condition->setParent(this);
	}
	else
		throw RepriseError(Strings::format("Unexpected replace expression (%p) in ReturnStatement.", &source));
}

//		PlainSwitchStatement - RepriseBase implementation
int PlainSwitchStatement::getChildCount(void) const
{
	return 2 + getLabelCount();
}

RepriseBase& PlainSwitchStatement::getChild(int index)
{
	if (index == 0)
		return *m_condition;
	else if (index == 1)
		return *m_block;
	else if (index >= 2 && index < getLabelCount() + 2)
		return getLabel(index-2);
	else
		throw UnexpectedChildError("PlainSwitchStatement::getChild");
}

std::string PlainSwitchStatement::dumpState(void) const
{
	std::string state = StatementBase::dumpState();
	state += "[Switch](" + this->getCondition().dumpState() + ")\n{\n";
	for (int i = 0; i < this->getLabelCount(); i++)
	{
		PlainCaseLabel rCaseLabel = this->getLabel(i);
		state += rCaseLabel.isDefault() ? "default:\n" :
			Strings::format("case %" OPS_CRT_FORMAT_LONG_LONG_PREFIX "i:\n", rCaseLabel.getValue());
		state += rCaseLabel.getStatement().dumpState();
	}
	state += "}\n";
	return state;
}


//	GotoStatement class implementation
GotoStatement::GotoStatement()
{
}

GotoStatement::GotoStatement(StatementBase* const pointedStatement) :
	m_pointed(pointedStatement)
{
}

StatementBase* GotoStatement::getPointedStatement(void) const
{
	return m_pointed.get();
}

void GotoStatement::setPointedStatement(StatementBase* const pointedStatement)
{
	m_pointed.reset(pointedStatement);
}

void GotoStatement::replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	OPS_UNUSED(destination)
	throw RepriseError(Strings::format("Unexpected replace expression (%p) in GotoStatement.", &source));
}

//		GotoStatement - RepriseBase implementation
int GotoStatement::getChildCount(void) const
{
	return 0;
}

RepriseBase& GotoStatement::getChild(const int index)
{
	OPS_UNUSED(index)
	throw UnexpectedChildError("GotoStatement::getChild");
}

int GotoStatement::getLinkCount(void) const
{
	return 1;
}

RepriseBase& GotoStatement::getLink(const int index)
{
	if (index < 0 || index >= getLinkCount())
		throw RepriseError("GotoStatement::getLink()");
	OPS_ASSERT(m_pointed.get() != 0)
	return *m_pointed;
}

std::string GotoStatement::dumpState(void) const
{
	std::string state = StatementBase::dumpState();
	state += "goto " + m_pointed->dumpState() + ";\n";
	return state;
}


//	ReturnStatement class implementation
ReturnStatement::ReturnStatement(void) :
	m_return(EmptyExpression::empty())
{
	m_return->setParent(this);
}

ReturnStatement::ReturnStatement(ExpressionBase* const returnExpression) :
	m_return(returnExpression)
{
	m_return->setParent(this);
}

ReturnStatement::ReturnStatement(const ReturnStatement& other) : StatementBase(other), m_return(other.m_return->clone())
{
	m_return->setParent(this);
}


const ExpressionBase& ReturnStatement::getReturnExpression(void) const
{
	return *m_return;
}

ExpressionBase& ReturnStatement::getReturnExpression(void)
{
	return *m_return;
}

void ReturnStatement::setReturnExpression(ExpressionBase* returnExpression)
{
	m_return.reset(returnExpression);
	m_return->setParent(this);
}

void ReturnStatement::replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	if (m_return.get() == &source)
	{
		m_return = destination;
		m_return->setParent(this);
	}
	else
		throw RepriseError(Strings::format("Unexpected replace expression (%p) in ReturnStatement.", &source));
}

//		ReturnStatement - RepriseBase implementation
int ReturnStatement::getChildCount(void) const
{
	return 1;
}

RepriseBase& ReturnStatement::getChild(const int index)
{
	switch (index)
	{
	case 0:
		return *m_return;
	}
	throw UnexpectedChildError("ReturnStatement::getChild");
}

std::string ReturnStatement::dumpState(void) const
{
	std::string state = StatementBase::dumpState();
	state += "return " + m_return->dumpState() + ";\n";
	return state;
}

//	ExpressionStatement class implementation
ExpressionStatement::ExpressionStatement(ExpressionBase* const expression) 
	: m_expression(expression)
{
	OPS_ASSERT(expression != 0)
	m_expression->setParent(this);
}

ExpressionStatement::ExpressionStatement(const ExpressionStatement& other) : StatementBase(other), 
	m_expression(other.m_expression->clone())
{
	m_expression->setParent(this);
}

const ExpressionBase& ExpressionStatement::get(void) const
{
	return *m_expression;
}

ExpressionBase& ExpressionStatement::get(void)
{
	return *m_expression;
}

void ExpressionStatement::set(ExpressionBase* const expression)
{
	m_expression.reset(expression);
	m_expression->setParent(this);
}

void ExpressionStatement::replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	if (m_expression.get() == &source)
	{
		m_expression = destination;
		m_expression->setParent(this);
	}
	else
		throw RepriseError(Strings::format("Unexpected replace expression (%p) in ExpressionStatement.", &source));
}

//		ExpressionStatement - RepriseBase implementation
int ExpressionStatement::getChildCount(void) const
{
	return 1;
}

RepriseBase& ExpressionStatement::getChild(const int index)
{
	switch (index)
	{
	case 0:
		return *m_expression;
	default:
		throw UnexpectedChildError("ExpressionStatement::getChild");
	}
}

std::string ExpressionStatement::dumpState(void) const
{
	std::string state = StatementBase::dumpState();
	state += m_expression->dumpState();
	state += ";\n";
	return state;
}

//	ASMStatement class implementation
ASMStatement::ASMStatement(std::string inlineASMString, ASMStatement::InlineASMType type) 
	: m_ASMString(inlineASMString),
	  m_ASMType(type)
{
}

ASMStatement::ASMStatement(const ASMStatement& other) : StatementBase(other), 
	m_ASMString(other.m_ASMString),
	m_ASMType(other.m_ASMType)
{
}

std::string ASMStatement::getASMString(void) const
{
	return m_ASMString;
}

void ASMStatement::setASMString(std::string inlineASMString)
{
	m_ASMString = inlineASMString;
}

ASMStatement::InlineASMType ASMStatement::getASMType() const
{
	return m_ASMType;
}

void ASMStatement::setASMType(ASMStatement::InlineASMType t)
{
	m_ASMType = t;
}

void ASMStatement::replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	OPS_UNUSED(destination)
	throw RepriseError(Strings::format("Unexpected replace expression (%p) in ASMStatement.", &source));
}

//		ASMStatement - RepriseBase implementation
int ASMStatement::getChildCount(void) const
{
	return 0;
}

RepriseBase& ASMStatement::getChild(const int index)
{
	throw UnexpectedChildError("ASMStatement::getChild");
}

std::string ASMStatement::dumpState(void) const
{
	std::string state = StatementBase::dumpState();
	state += "assember { ";
	state += m_ASMString;
	state += "};\n";
	return state;
}

//	EmptyStatement class implementation
EmptyStatement::EmptyStatement(void)
{
}

EmptyStatement::EmptyStatement(const std::string& note) : m_note(note)
{
}

void EmptyStatement::replaceExpression(ExpressionBase& source, ReprisePtr<ExpressionBase> destination)
{
	OPS_UNUSED(destination)
	throw RepriseError(Strings::format("Unexpected replace expression (%p) in EmptyStatement.", &source));
}


//		EmptyStatement - RepriseBase implementation
int EmptyStatement::getChildCount(void) const
{
	return 0;
}

RepriseBase& EmptyStatement::getChild(const int index)
{
	OPS_UNUSED(index)
	throw UnexpectedChildError("EmptyStatement::getChild");
}

std::string EmptyStatement::dumpState(void) const
{
	std::string state = StatementBase::dumpState();
	if (m_note.empty())
		state += "EMPTY_STATEMENT;\n";
	else
		state += "EMPTY_STATEMENT(" + m_note + ");\n";
	return state;
}



}
}
