#include "Reprise/Declarations.h"
#include "Reprise/Exceptions.h"
#include "Reprise/Expressions.h"
#include "Reprise/Statements.h"

//	Enter namespace
namespace OPS
{
namespace Reprise
{

//	DeclarationBase class implementation
DeclarationBase::DeclarationBase(const DeclarationKind declKind) : m_kind(declKind)
{
}

DeclarationBase::DeclarationBase(const DeclarationKind declKind, const std::string& name) : m_kind(declKind), m_name(name)
{
}

DeclarationBase::DeclarationBase(const DeclarationBase& other) : RepriseBase(other), m_kind(DK_UNUSED)
{
	m_kind = other.m_kind;
	m_name = other.m_name;
}

const std::string& DeclarationBase::getName(void) const
{
	return m_name;
}

void DeclarationBase::setName(const std::string &name)
{
    m_name = name;
}

DeclarationBase::DeclarationKind DeclarationBase::getKind(void) const
{
	return m_kind;
}

VariableDeclaration& DeclarationBase::asVariable()
{
	if (m_kind != DK_VARIABLE)
		throw StateError("Declaration is not a variable.");
	return static_cast<VariableDeclaration&>(*this);
}

const VariableDeclaration& DeclarationBase::asVariable() const
{
	if (m_kind != DK_VARIABLE)
		throw StateError("Declaration is not a variable.");
	return static_cast<const VariableDeclaration&>(*this);
}

SubroutineDeclaration& DeclarationBase::asSubroutine()
{
	if (m_kind != DK_SUBROUTINE)
		throw StateError("Declaration is not a subroutine.");
	return static_cast<SubroutineDeclaration&>(*this);
}

const SubroutineDeclaration& DeclarationBase::asSubroutine() const
{
	if (m_kind != DK_SUBROUTINE)
		throw StateError("Declaration is not a subroutine.");
	return static_cast<const SubroutineDeclaration&>(*this);
}

TypeDeclaration& DeclarationBase::asType()
{
	if (m_kind != DK_TYPE)
		throw StateError("Declaration is not a type.");
	return static_cast<TypeDeclaration&>(*this);
}

const TypeDeclaration& DeclarationBase::asType() const
{
	if (m_kind != DK_TYPE)
		throw StateError("Declaration is not a type.");
	return static_cast<const TypeDeclaration&>(*this);
}


//	VariableDeclarators class implementation
VariableDeclarators::VariableDeclarators(void)
	: m_declaration(DECL_EMPTY)
{
}

VariableDeclarators::VariableDeclarators(const int declFlags) 
	: m_declaration(declFlags)
{
}

std::string VariableDeclarators::declListOfFlagsToString(int flags)
{
	std::string result = "";
	if (flags & DECL_EXTERN)
	{
		result += "extern ";
	}
	if (flags & DECL_STATIC)
	{
		result += "static ";
	}
	if (flags & DECL_INLINE)
	{
		result += "inline ";
	}
	if (flags & DECL_REGISTER)
	{
		result += "register ";
	}
	if (flags & DECL_ALLOCATABLE)
	{
		result += "allocatable ";
	}
	if (flags & DECL_ASYNCHRONOUS)
	{
		result += "asynchronous ";
	}
	if (flags & DECL_TARGET)
	{
		result += "target ";
	}
	return Strings::trim(result);
}

bool VariableDeclarators::has(DeclFlags flag) const
{
	return (m_declaration & flag) == flag;
}

void VariableDeclarators::set(DeclFlags flag)
{
	m_declaration |= static_cast<unsigned>(flag);
}

void VariableDeclarators::reset(DeclFlags flag)
{
	m_declaration &= ~flag;
}

bool VariableDeclarators::isExtern(void) const
{
	return (m_declaration & DECL_EXTERN) == DECL_EXTERN;
}

bool VariableDeclarators::isStatic(void) const
{
	return (m_declaration & DECL_STATIC) == DECL_STATIC;
}

bool VariableDeclarators::isInline(void) const
{
	return (m_declaration & DECL_INLINE) == DECL_INLINE;
}

bool VariableDeclarators::isRegister(void) const
{
	return (m_declaration & DECL_REGISTER) == DECL_REGISTER;
}

bool VariableDeclarators::isAllocatable(void) const
{
	return (m_declaration & DECL_ALLOCATABLE) == DECL_ALLOCATABLE;
}

bool VariableDeclarators::isAsynchronous(void) const
{
	return (m_declaration & DECL_ASYNCHRONOUS) == DECL_ASYNCHRONOUS;
}

bool VariableDeclarators::isTarget(void) const
{
	return (m_declaration & DECL_TARGET) == DECL_TARGET;
}

VariableDeclarators::DeclFlags VariableDeclarators::getDeclaration(void) const
{
	return static_cast<DeclFlags>(m_declaration);
}

std::string VariableDeclarators::dump(void) const
{
	return declListOfFlagsToString(m_declaration);
}


//	VariableDeclaration class implementation
VariableDeclaration::VariableDeclaration(TypeBase* const typeBase, const std::string& name)
	: DeclarationBase(DK_VARIABLE, name), m_type(typeBase), m_init(new EmptyExpression())
{
	m_type->setParent(this);
	m_init->setParent(this);
}

VariableDeclaration::VariableDeclaration(TypeBase* const typeBase, const std::string& name, const VariableDeclarators& declarators)
	: DeclarationBase(DK_VARIABLE, name), m_type(typeBase), m_declarators(declarators), m_init(new EmptyExpression())
{
	m_type->setParent(this);
	m_init->setParent(this);
}

VariableDeclaration::VariableDeclaration(TypeBase* const typeBase, const std::string& name, 
	const VariableDeclarators& declarators, ExpressionBase* const initExpression)
	: DeclarationBase(DK_VARIABLE, name), m_type(typeBase), m_declarators(declarators), m_init(initExpression)
{
	m_type->setParent(this);
	m_init->setParent(this);
}

VariableDeclaration::VariableDeclaration(TypeBase* const typeBase, const std::string& name, 
	const VariableDeclarators& declarators, ExpressionBase* const initExpression, ParameterDescriptor& parameterReference)
	: DeclarationBase(DK_VARIABLE, name), m_type(typeBase), m_declarators(declarators), m_init(initExpression), 
	m_parameterReference(&parameterReference)
{
	m_type->setParent(this);
	m_init->setParent(this);
}

VariableDeclaration::VariableDeclaration(const VariableDeclaration& other) : DeclarationBase(other)
{
	m_type.reset(other.m_type->clone());
	m_type->setParent(this);
	m_declarators = other.m_declarators;
	m_init.reset(other.m_init->clone());
	m_init->setParent(this);
	// TODO: Proper parameter reference cloning
	m_parameterReference = other.m_parameterReference;
	// TODO: Proper defined block cloning
	m_definedBlock = other.m_definedBlock;
}


const VariableDeclarators& VariableDeclaration::getDeclarators(void) const
{
	return m_declarators;
}

VariableDeclarators& VariableDeclaration::declarators(void)
{
	return m_declarators;
}

const TypeBase& VariableDeclaration::getType(void) const
{
	return *m_type;
}

TypeBase& VariableDeclaration::getType(void)
{
	return *m_type;
}

void VariableDeclaration::setType(TypeBase* typeBase)
{
	m_type.reset(typeBase);
	m_type->setParent(this);
}

bool VariableDeclaration::hasNonEmptyInitExpression(void) const
{
	return !m_init.get()->is_a<EmptyExpression>();
}

const ExpressionBase& VariableDeclaration::getInitExpression(void) const
{
	return *m_init;
}

ExpressionBase& VariableDeclaration::getInitExpression(void)
{
	return *m_init;
}

void VariableDeclaration::setInitExpression(ExpressionBase& initExpression)
{
	m_init.reset(&initExpression);
	m_init->setParent(this);
}

ReprisePtr<ExpressionBase> VariableDeclaration::detachInitExpression(void)
{
	m_init->setParent(0);
	ReprisePtr<ExpressionBase> init(m_init.release());
	m_init.reset(new EmptyExpression());
	m_init->setParent(this);
	return init;
}


bool VariableDeclaration::hasParameterReference(void) const
{
	return m_parameterReference.get() != 0;
}

const ParameterDescriptor& VariableDeclaration::getParameterReference(void) const
{
	return *m_parameterReference;
}

ParameterDescriptor& VariableDeclaration::getParameterReference(void)
{
	return *m_parameterReference;
}

void VariableDeclaration::setParameterReference(ParameterDescriptor& parameterReference)
{
	m_parameterReference.reset(&parameterReference);
}

bool VariableDeclaration::hasDefinedBlock(void) const
{
	return m_definedBlock.get() != 0;
}

const BlockStatement& VariableDeclaration::getDefinedBlock(void) const
{
	return *m_definedBlock;
}

BlockStatement& VariableDeclaration::getDefinedBlock(void)
{
	return *m_definedBlock;
}

void VariableDeclaration::setDefinedBlock(BlockStatement& block)
{
	m_definedBlock.reset(&block);
}

void VariableDeclaration::resetDefinedBlock()
{
    m_definedBlock.reset();
}

bool VariableDeclaration::hasDefinition(void) const
{
    return m_definition.get() != 0;
}

const VariableDeclaration& VariableDeclaration::getDefinition(void) const
{
    return *m_definition;
}

VariableDeclaration& VariableDeclaration::getDefinition(void)
{
    return *m_definition;
}

void VariableDeclaration::setDefinition(VariableDeclaration& definition)
{
    m_definition.reset(&definition);
}

//		VariableDeclaration - RepriseBase implementation
int VariableDeclaration::getChildCount(void) const
{
	return 2;
}

RepriseBase& VariableDeclaration::getChild(const int index)
{
	if (index == 0)
		return *m_type;
	else
	if (index == 1)
		return *m_init;
	else
		throw UnexpectedChildError("VariableDeclaration::getChild");
}

std::string VariableDeclaration::dumpState(void) const
{
//	TODO: Add declarators dump here
	std::string state = DeclarationBase::dumpState();
	state += m_type->dumpState();
	state += " " + m_name;
	if (m_init.get() != 0)
	{
		state += " = " + m_init->dumpState();
	}
	state += ";";
	return state;
}

//	SubroutineDeclaration class implementation
SubroutineDeclaration::SubroutineDeclaration(SubroutineType* subroutineType, const std::string& name) 
	: DeclarationBase(DK_SUBROUTINE, name), m_type(subroutineType)
{
	m_type->setParent(this);
}

SubroutineDeclaration::SubroutineDeclaration(SubroutineType* subroutineType, const std::string& name, const VariableDeclarators& declarators) 
	: DeclarationBase(DK_SUBROUTINE, name), m_type(subroutineType), m_declarators(declarators)
{
	m_type->setParent(this);
}

SubroutineDeclaration::SubroutineDeclaration(const SubroutineDeclaration& other) : DeclarationBase(other)
{
	m_type.reset(other.m_type->clone());
	m_type->setParent(this);
	m_declarators = other.m_declarators;
    m_definition = other.m_definition;
	if (other.m_declarations.get() != 0)
	{
		m_declarations.reset(other.m_declarations->clone());
		m_declarations->setParent(this);
	}
	if (other.m_body.get() != 0)
	{
		m_body.reset(other.m_body->clone());
		m_body->setParent(this);
	}
}


const VariableDeclarators& SubroutineDeclaration::getDeclarators(void) const
{
	return m_declarators;
}

VariableDeclarators& SubroutineDeclaration::declarators(void)
{
	return m_declarators;
}

const SubroutineType& SubroutineDeclaration::getType(void) const
{
	return *m_type;
}

SubroutineType& SubroutineDeclaration::getType(void)
{
	return *m_type;
}

bool SubroutineDeclaration::hasImplementation(void) const
{
	return m_body.get() != 0;
}

const Declarations& SubroutineDeclaration::getDeclarations(void) const
{
	return *m_declarations;
}

Declarations& SubroutineDeclaration::getDeclarations(void)
{
	return *m_declarations;
}

void SubroutineDeclaration::setDeclarations(Declarations* declarations)
{
	m_declarations.reset(declarations);
	m_declarations->setParent(this);
}

const BlockStatement& SubroutineDeclaration::getBodyBlock(void) const
{
	return *m_body;
}

BlockStatement& SubroutineDeclaration::getBodyBlock(void)
{
	return *m_body;
}

void SubroutineDeclaration::setBodyBlock(ReprisePtr<BlockStatement> body)
{
	m_declarations.reset(new Declarations());
	m_declarations->setParent(this);
	m_body.reset(body.release());
	m_body->setParent(this);
}

void SubroutineDeclaration::createEmptyBody(void)
{
	m_declarations.reset(new Declarations());
	m_declarations->setParent(this);
	m_body.reset(new BlockStatement());
	m_body->setParent(this);
}

void SubroutineDeclaration::setPrototype(void)
{
	m_declarations.reset(0);
	m_body.reset(0);
}

bool SubroutineDeclaration::hasDefinition(void) const
{
	return m_definition.get() != 0;
}

const SubroutineDeclaration& SubroutineDeclaration::getDefinition(void) const
{
	return *m_definition;
}

SubroutineDeclaration& SubroutineDeclaration::getDefinition(void)
{
	return *m_definition;
}

void SubroutineDeclaration::setDefinition(SubroutineDeclaration& definition)
{
	m_definition.reset(&definition);
}

//		SubroutineDeclaration - RepriseBase implementation
int SubroutineDeclaration::getChildCount(void) const
{
	return 1 + (hasImplementation() ? 2 : 0);
}

RepriseBase& SubroutineDeclaration::getChild(const int index)
{
	if (index == 0)
		return *m_type;
	if (index == 1)
		return *m_declarations;
	if (index == 2)
		return *m_body;
	else
		throw UnexpectedChildError("SubroutineDeclaration::getChild");
}

std::string SubroutineDeclaration::dumpState(void) const
{
//	TODO: Add declarators dump here
	std::string state = DeclarationBase::dumpState();
	state += m_type->dumpState();
	state += " " + m_name;
	if (hasImplementation())
	{
		state += "\n" + m_declarations->dumpState() + "\n" + m_body->dumpState();
	}
	else
		state += ";";
	return state;
}

//	TypeDeclaration class implementation
TypeDeclaration::TypeDeclaration(TypeBase* typeBase, const std::string& name) 
	: DeclarationBase(DK_TYPE, name), m_type(typeBase), m_extern(false), m_static(false)
{
	m_type->setParent(this);
}

TypeDeclaration::TypeDeclaration(TypeBase* typeBase, const std::string& name, 
	const bool isExtern, const bool isStatic) 
	: DeclarationBase(DK_TYPE, name), m_type(typeBase), m_extern(isExtern), m_static(isStatic)
{
	m_type->setParent(this);
}

TypeDeclaration::TypeDeclaration(const TypeDeclaration& other) : DeclarationBase(other)
{
	m_type.reset(other.m_type->clone());
	m_type->setParent(this);
	m_extern = other.m_extern;
	m_static = other.m_static;
}

const TypeBase& TypeDeclaration::getType(void) const
{
	return *m_type;
}

TypeBase& TypeDeclaration::getType(void)
{
	return *m_type;
}

void TypeDeclaration::setType(TypeBase* typeBase)
{
	m_type.reset(typeBase);
	m_type->setParent(this);
}

bool TypeDeclaration::isExtern(void) const
{
	return m_extern;
}

bool TypeDeclaration::isStatic(void) const
{
	return m_static;
}

//		TypeDeclaration - RepriseBase implementation
int TypeDeclaration::getChildCount(void) const
{
	return 1;
}

RepriseBase& TypeDeclaration::getChild(const int index)
{
	if (index < 0 || index >= getChildCount())
		throw UnexpectedChildError("TypeDeclaration::getChild");
	return *m_type;
}

std::string TypeDeclaration::dumpState(void) const
{
	std::string state = DeclarationBase::dumpState();
	if (m_extern)
		state += "extern ";
	if (m_static)
		state += "static ";
	state += m_type->dumpState();
	return state;
}

/*
//	LabelDeclaration class implementation
LabelDeclaration::LabelDeclaration(StatementBase* labeledStatement, const std::string& name) : m_labeled(labeledStatement), m_name(name)
{
}

const StatementBase& LabelDeclaration::getLabeledStatement(void) const
{
	return *m_labeled;
}

StatementBase& LabelDeclaration::getLabeledStatement(void)
{
	return *m_labeled;
}

std::string LabelDeclaration::getName(void) const
{
	return m_name;
}

//	LabelDeclaration - RepriseBase implementation
int LabelDeclaration::getChildCount(void) const
{
	return 0;
}

RepriseBase& LabelDeclaration::getChild(const int index)
{
	OPS_UNUSED(index)
	throw UnexpectedChildError("LabelDeclaration::getChild");
}

std::string LabelDeclaration::dumpState(void) const
{
	std::string state = DeclarationBase::dumpState();
	state += "label " + m_name;
	return state;
}
*/

//	Declarations class implementation
Declarations::Declarations(void)
{
}

Declarations::Declarations(const Declarations& other) : RepriseBase(other)
{
	for (ConstIterator iter = other.getFirst(); iter.isValid(); ++iter)
	{
		addLast(iter->clone());
	}
}

int Declarations::getTypeCount(void) const
{
	return getCount(DeclarationBase::DK_TYPE);
}

void Declarations::addType(TypeDeclaration* const declarator)
{
	addLast(declarator);
}

//void Declarations::insertType(int index, TypeDeclaration* const declarator)
//{
//	if (index < 0 || index > getTypeCount())
//		throw RepriseError(Strings::format("Getting unexpected type (%i), type count (%i).",
//			index, getTypeCount()));
//	m_types.insert(index, declarator).setParent(this);
//	declarator->private_addRef();
//}

int Declarations::getSubroutineCount(void) const
{
	return getCount(DeclarationBase::DK_SUBROUTINE);
}

void Declarations::addSubroutine(SubroutineDeclaration* declarator)
{
	addLast(declarator);
}

//void Declarations::insertSubroutine(int index, SubroutineDeclaration* declarator)
//{
//	if (index < 0 || index > getSubroutineCount())
//		throw RepriseError(Strings::format("Getting unexpected subroutine (%i), subroutine count (%i).",
//			index, getSubroutineCount()));
//	m_subroutines.insert(index, declarator).setParent(this);
//	declarator->private_addRef();
//}

int Declarations::getVariableCount(void) const
{
	return getCount(DeclarationBase::DK_VARIABLE);
}

void Declarations::addVariable(VariableDeclaration* declarator)
{
	addLast(declarator);
}

//void Declarations::insertVariable(int index, VariableDeclaration* declarator)
//{
//	if (index < 0 || index > getVariableCount())
//		throw RepriseError(Strings::format("Getting unexpected variable (%i), variable count (%i).",
//			index, getTypeCount()));
//	m_variables.insert(index, declarator).setParent(this);
//	declarator->private_addRef();
//}

bool Declarations::isEmpty(void) const
{
	return m_declarations.isEmpty();
}

Declarations::ConstIterator Declarations::getFirst(void) const
{
	return m_declarations.getFirst();
}

Declarations::ConstIterator Declarations::getLast(void) const
{
	return m_declarations.getLast();
}

Declarations::Iterator Declarations::getFirst(void)
{
	return m_declarations.getFirst();
}

Declarations::Iterator Declarations::getLast(void)
{
	return m_declarations.getLast();
}

ConstDeclIterator<VariableDeclaration> Declarations::getFirstVar(void) const
{
	return ConstDeclIterator<VariableDeclaration>(DeclarationBase::DK_VARIABLE, getFirst(), false);
}

ConstDeclIterator<VariableDeclaration> Declarations::getLastVar(void) const
{
	return ConstDeclIterator<VariableDeclaration>(DeclarationBase::DK_VARIABLE, getLast(), true);
}

DeclIterator<VariableDeclaration> Declarations::getFirstVar(void)
{
	return DeclIterator<VariableDeclaration>(DeclarationBase::DK_VARIABLE, getFirst(), false);
}

DeclIterator<VariableDeclaration> Declarations::getLastVar(void)
{
	return DeclIterator<VariableDeclaration>(DeclarationBase::DK_VARIABLE, getLast(), true);
}

Declarations::ConstTypeIterator Declarations::getFirstType(void) const
{
	return ConstDeclIterator<TypeDeclaration>(DeclarationBase::DK_TYPE, getFirst(), false);
}

Declarations::ConstTypeIterator Declarations::getLastType(void) const
{
	return ConstDeclIterator<TypeDeclaration>(DeclarationBase::DK_TYPE, getLast(), true);
}

Declarations::TypeIterator Declarations::getFirstType(void)
{
	return DeclIterator<TypeDeclaration>(DeclarationBase::DK_TYPE, getFirst(), false);
}

Declarations::TypeIterator Declarations::getLastType(void)
{
	return DeclIterator<TypeDeclaration>(DeclarationBase::DK_TYPE, getLast(), true);
}

Declarations::ConstSubrIterator Declarations::getFirstSubr(void) const
{
	return ConstDeclIterator<SubroutineDeclaration>(DeclarationBase::DK_SUBROUTINE, getFirst(), false);
}

Declarations::ConstSubrIterator Declarations::getLastSubr(void) const
{
	return ConstDeclIterator<SubroutineDeclaration>(DeclarationBase::DK_SUBROUTINE, getLast(), true);
}

Declarations::SubrIterator Declarations::getFirstSubr(void)
{
	return DeclIterator<SubroutineDeclaration>(DeclarationBase::DK_SUBROUTINE, getFirst(), false);
}

Declarations::SubrIterator Declarations::getLastSubr(void)
{
	return DeclIterator<SubroutineDeclaration>(DeclarationBase::DK_SUBROUTINE, getLast(), true);
}

Declarations::Iterator Declarations::addFirst(DeclarationBase* const declaration)
{
	declaration->setParent(this);
	m_declHash.insert(make_pair(declaration->getName(), declaration));
	return m_declarations.addFirst(declaration);
}

Declarations::Iterator Declarations::addLast(DeclarationBase* const declaration)
{
	declaration->setParent(this);
	m_declHash.insert(make_pair(declaration->getName(), declaration));
	return m_declarations.addLast(declaration);
}

Declarations::Iterator Declarations::addBefore(const Iterator& iterator, DeclarationBase* const declaration)
{
	declaration->setParent(this);
	m_declHash.insert(make_pair(declaration->getName(), declaration));
	return m_declarations.addBefore(iterator, declaration);
}

Declarations::Iterator Declarations::addAfter(const Iterator& iterator, DeclarationBase* const declaration)
{
	declaration->setParent(this);
	m_declHash.insert(make_pair(declaration->getName(), declaration));
	return m_declarations.addAfter(iterator, declaration);
}

Declarations::Iterator Declarations::convertToIterator(DeclarationBase* const declaration)
{
	return m_declarations.convertToIterator(declaration);
}

void Declarations::erase(Iterator& iterator)
{
	std::pair<THashMap::iterator, THashMap::iterator> toErase = m_declHash.equal_range(iterator->getName());
	m_declHash.erase(toErase.first, toErase.second);
	m_declarations.remove(iterator);
}

void Declarations::replace(Iterator& iterator, DeclarationBase* const declaration)
{
	addBefore(iterator, declaration);
	erase(iterator);
}

void Declarations::rename(DeclarationBase *declaration, const std::string &newName)
{
    if (declaration == 0 ||
        declaration->getParent() != this)
        throw OPS::RuntimeError("Invalid declarations has been passed to Declarations::rename");

    std::pair<THashMap::iterator, THashMap::iterator> toErase = m_declHash.equal_range(declaration->getName());
    m_declHash.erase(toErase.first, toErase.second);
    declaration->setName(newName);
    m_declHash.insert(make_pair(newName, declaration));
}

VariableDeclaration* Declarations::findVariable(const std::string& name)
{
	std::pair<THashMap::iterator, THashMap::iterator> equal = m_declHash.equal_range(name);
	for (THashMap::iterator it = equal.first; it != equal.second; ++it)
	{
		if (it->second->getKind() == DeclarationBase::DK_VARIABLE)
			return static_cast<VariableDeclaration*>(it->second);
	}
	return 0;
}

TypeDeclaration* Declarations::findType(const std::string& name)
{
	std::pair<THashMap::iterator, THashMap::iterator> equal = m_declHash.equal_range(name);
	for (THashMap::iterator it = equal.first; it != equal.second; ++it)
	{
		if (it->second->getKind() == DeclarationBase::DK_TYPE)
			return static_cast<TypeDeclaration*>(it->second);
	}
	return 0;
}

TypeDeclaration* Declarations::findStruct(const std::string& name)
{
	std::pair<THashMap::iterator, THashMap::iterator> equal = m_declHash.equal_range(name);
	for (THashMap::iterator it = equal.first; it != equal.second; ++it)
	{
		if (it->second->getKind() == DeclarationBase::DK_TYPE && static_cast<TypeDeclaration*>(it->second)->getType().is_a<StructType>())
			return static_cast<TypeDeclaration*>(it->second);
	}
	return 0;
}

TypeDeclaration* Declarations::findUnion(const std::string& name)
{
	return 0;
}

TypeDeclaration* Declarations::findEnum(const std::string& name)
{
    return 0;
}

SubroutineDeclaration* Declarations::findSubroutine(const std::string& name)
{
	std::pair<THashMap::iterator, THashMap::iterator> equal = m_declHash.equal_range(name);
	for (THashMap::iterator it = equal.first; it != equal.second; ++it)
	{
		if (it->second->getKind() == DeclarationBase::DK_SUBROUTINE)
			return static_cast<SubroutineDeclaration*>(it->second);
	}
	return 0;
}


//		Declarations - RepriseBase implementation
int Declarations::getChildCount(void) const
{
	return m_declarations.getCount();
}

RepriseBase& Declarations::getChild(const int index)
{
	int count = 0;
	for (Iterator decl = getFirst(); decl.isValid(); ++decl)
	{
		if (index == count)
			return *decl;
		++count;
	}
	throw UnexpectedChildError("Declarations::getChild");
}

std::string Declarations::dumpState(void) const
{
	std::string state = RepriseBase::dumpState();
	for (ConstIterator decl = getFirst(); decl.isValid(); ++decl)
	{
		state += decl->dumpState() + "\n";
	}
	return state;
}


int Declarations::getCount(DeclarationBase::DeclarationKind kind) const
{
	int count = 0;
	for (ConstIterator decl = getFirst(); decl.isValid(); ++decl)
	{
		if (decl->getKind() == kind)
			++count;
	}
	return count;
}

}
}
