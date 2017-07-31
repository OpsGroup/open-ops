#ifndef OPS_IR_REPRISE_DECLARATIONS_H_INCLUDED__
#define OPS_IR_REPRISE_DECLARATIONS_H_INCLUDED__

#include "Reprise/Common.h"
#include "Reprise/Collections.h"
#include "Reprise/Utils.h"

#include <string>

namespace OPS
{
namespace Reprise
{

class TypeBase;
class ExpressionBase;
class SubroutineType;
class StatementBase;
class BlockStatement;
class ParameterDescriptor;
class Declarations;

class VariableDeclaration;
class SubroutineDeclaration;
class TypeDeclaration;

template <class DeclType>
class DeclIterator;

template <class DeclType>
class ConstDeclIterator;

///	Base class for declarations
class DeclarationBase : public RepriseBase, public IntrusiveNodeBase<DeclarationBase>
{
public:
	enum DeclarationKind
	{
		DK_UNUSED = 0,
		DK_VARIABLE,
		DK_TYPE,
		DK_SUBROUTINE
	};

	explicit DeclarationBase(DeclarationKind declKind);
	DeclarationBase(DeclarationKind declKind, const std::string& name);
	DeclarationBase(const DeclarationBase& other);
	inline virtual ~DeclarationBase() { }

	const std::string& getName(void) const;

	DeclarationKind getKind(void) const;

	VariableDeclaration& asVariable();
	const VariableDeclaration& asVariable() const;
	SubroutineDeclaration& asSubroutine();
	const SubroutineDeclaration& asSubroutine() const;
	TypeDeclaration& asType();
	const TypeDeclaration& asType() const;

	virtual DeclarationBase* clone(void) const = 0;
protected:
    friend class Declarations;
    void setName(const std::string& name);

	DeclarationKind m_kind;
	std::string m_name;
};

///	Declarators for variables
class VariableDeclarators
{
public:
	enum DeclFlags
	{
		DECL_EMPTY	      = 0x0000,
		DECL_EXTERN       = 0x0001,
		DECL_STATIC       = 0x0002,
		DECL_REGISTER     = 0x0004,
	// Deprecated:
	//	DECL_CONST        = 0x0008,
	//	DECL_VOLATILE     = 0x0010,
		DECL_ALLOCATABLE  = 0x0020,
		DECL_ASYNCHRONOUS = 0x0040,
		DECL_TARGET       = 0x0080,
		DECL_INLINE		  = 0x0100,
	};

	static std::string declListOfFlagsToString(int flags);

	VariableDeclarators(void);
	VariableDeclarators(int declFlags);

	bool has(DeclFlags flag) const;
	void set(DeclFlags flag);
	void reset(DeclFlags flag);

	bool isExtern(void) const;
	bool isStatic(void) const;
	bool isRegister(void) const;
	bool isInline(void) const;

	bool isAllocatable(void) const;
	bool isAsynchronous(void) const;
	bool isTarget(void) const;

	DeclFlags getDeclaration(void) const;
	
	std::string dump(void) const;

private:
	int m_declaration;
};

///	Class to hold information about variable declaration
class VariableDeclaration : public DeclarationBase
{
public:
	VariableDeclaration(TypeBase* typeBase, const std::string& name);
	VariableDeclaration(TypeBase* typeBase, const std::string& name, const VariableDeclarators& declarators);
	VariableDeclaration(TypeBase* typeBase, const std::string& name, const VariableDeclarators& declarators, 
		ExpressionBase* const initExpression);
	VariableDeclaration(TypeBase* typeBase, const std::string& name, const VariableDeclarators& declarators, 
		ExpressionBase* const initExpression, ParameterDescriptor& parameterReference);

	const VariableDeclarators& getDeclarators(void) const;
	VariableDeclarators& declarators(void);

	const TypeBase& getType(void) const;
	TypeBase& getType(void);

	void setType(TypeBase*);

//	Temp method. Remove?
	bool hasNonEmptyInitExpression(void) const;

	const ExpressionBase& getInitExpression(void) const;
	ExpressionBase& getInitExpression(void);
	void setInitExpression(ExpressionBase& initExpression);
	ReprisePtr<ExpressionBase> detachInitExpression(void);

	bool hasParameterReference(void) const;
	const ParameterDescriptor& getParameterReference(void) const;
	ParameterDescriptor& getParameterReference(void);
	void setParameterReference(ParameterDescriptor& parameterReference);

	bool hasDefinedBlock(void) const;
	const BlockStatement& getDefinedBlock(void) const;
	BlockStatement& getDefinedBlock(void);
	void setDefinedBlock(BlockStatement& block);
	void resetDefinedBlock();

	bool hasDefinition(void) const;
	const VariableDeclaration& getDefinition(void) const;
	VariableDeclaration& getDefinition(void);
	void setDefinition(VariableDeclaration&);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(VariableDeclaration)

protected:
	VariableDeclaration(const VariableDeclaration& other);

private:
	ReprisePtr<TypeBase> m_type;
	VariableDeclarators m_declarators;
	ReprisePtr<ExpressionBase> m_init;
	RepriseWeakPtr<ParameterDescriptor> m_parameterReference;
	RepriseWeakPtr<BlockStatement> m_definedBlock;
    RepriseWeakPtr<VariableDeclaration> m_definition;
};

///	Class to hold information about subroutine declaration
class SubroutineDeclaration : public DeclarationBase
{
public:
	SubroutineDeclaration(SubroutineType* subroutineType, const std::string& name);
	SubroutineDeclaration(SubroutineType* subroutineType, const std::string& name, const VariableDeclarators& declarators);

	const VariableDeclarators& getDeclarators(void) const;
	VariableDeclarators& declarators(void);

	const SubroutineType& getType(void) const;
	SubroutineType& getType(void);

	bool hasImplementation(void) const;

	const Declarations& getDeclarations(void) const;
	Declarations& getDeclarations(void);
	void setDeclarations(Declarations* declarations);

	const BlockStatement& getBodyBlock(void) const;
	BlockStatement& getBodyBlock(void);
	void setBodyBlock(ReprisePtr<BlockStatement> body);

	void createEmptyBody(void);
	void setPrototype(void);

	bool hasDefinition(void) const;
	const SubroutineDeclaration& getDefinition(void) const;
	SubroutineDeclaration& getDefinition(void);
	void setDefinition(SubroutineDeclaration&);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(SubroutineDeclaration)

protected:
	SubroutineDeclaration(const SubroutineDeclaration& other);

private:
	ReprisePtr<SubroutineType> m_type;
	VariableDeclarators m_declarators;
	ReprisePtr<Declarations> m_declarations;
	ReprisePtr<BlockStatement> m_body;
	RepriseWeakPtr<SubroutineDeclaration> m_definition;
};

///	Class to hold information about type declaration
class TypeDeclaration : public DeclarationBase
{
public:
	TypeDeclaration(TypeBase* typeBase, const std::string& name);
	TypeDeclaration(TypeBase* typeBase, const std::string& name, bool isExtern, bool isStatic);

	const TypeBase& getType(void) const;
	TypeBase& getType(void);
	void setType(TypeBase*);

	bool isExtern(void) const;

	bool isStatic(void) const;

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(TypeDeclaration)

protected:
	TypeDeclaration(const TypeDeclaration& other);

private:
	ReprisePtr<TypeBase> m_type;
	bool m_extern;
	bool m_static;
};

///	Class to hold information about variables and types
class Declarations : public RepriseBase
{
private:
	typedef IntrusiveList<DeclarationBase> DeclarationsType;
public: 
	typedef DeclarationsType::ConstIterator ConstIterator;
	typedef DeclarationsType::Iterator Iterator;

	typedef ConstDeclIterator<VariableDeclaration> ConstVarIterator;
	typedef DeclIterator<VariableDeclaration> VarIterator;
	typedef ConstDeclIterator<TypeDeclaration> ConstTypeIterator;
	typedef DeclIterator<TypeDeclaration> TypeIterator;
	typedef ConstDeclIterator<SubroutineDeclaration> ConstSubrIterator;
	typedef DeclIterator<SubroutineDeclaration> SubrIterator;

	Declarations(void);

///		Gets type count [deprecated] O(n)
	int getTypeCount(void) const;

/**
		\brief Add type to declarations block. Declarations take ownership of passing pointer.
*/
	void addType(TypeDeclaration* declarator);

///		Gets subroutine count [deprecated] O(n)
	int getSubroutineCount(void) const;

/**
		\brief Add subroutine to declarations block. Declarations take ownership of passing pointer.
*/
	void addSubroutine(SubroutineDeclaration* declarator);

///		Gets variable count [deprecated] O(n)
	int getVariableCount(void) const;

/**
		\brief Add variable to declarations block. Declarations take ownership of passing pointer.
*/
	void addVariable(VariableDeclaration* declarator);

///	Checks that declarations block is empty.
	bool isEmpty(void) const;

///	Gets const iterator to first declaration.
	ConstIterator getFirst(void) const;
///	Gets const iterator to last declaration.
	ConstIterator getLast(void) const;

///	Gets iterator to first declaration.
	Iterator getFirst(void);
///	Gets iterator to last declaration.
	Iterator getLast(void);

///	Gets iterator to first variable declaration.
	ConstVarIterator getFirstVar(void) const;
///	Gets iterator to last variable declaration.
	ConstVarIterator getLastVar(void) const;

///	Gets iterator to first variable declaration.
	VarIterator getFirstVar(void);
///	Gets iterator to last variable declaration.
	VarIterator getLastVar(void);

///	Gets iterator to first type declaration.
	ConstTypeIterator getFirstType(void) const;
///	Gets iterator to last type declaration.
	ConstTypeIterator getLastType(void) const;

///	Gets iterator to first type declaration.
	TypeIterator getFirstType(void);
///	Gets iterator to last type declaration.
	TypeIterator getLastType(void);

///	Gets iterator to first subroutine declaration.
	ConstSubrIterator getFirstSubr(void) const;
///	Gets iterator to last subroutine declaration.
	ConstSubrIterator getLastSubr(void) const;

///	Gets iterator to first subroutine declaration.
	SubrIterator getFirstSubr(void);
///	Gets iterator to last subroutine declaration.
	SubrIterator getLastSubr(void);

///	Adds declaration at first place. Declarations take ownership of passing pointer.
	Iterator addFirst(DeclarationBase* const declaration);
///	Adds declaration at last place. Declarations take ownership of passing pointer.
	Iterator addLast(DeclarationBase* const declaration);

/**
	\brief Adds declaration before iterator. Declarations take ownership of passing pointer.

	\arg \c iterator - iterator of declaration to insert your declaration before.
	\arg \c declaration - your declaration.
*/
	Iterator addBefore(const Iterator& iterator, DeclarationBase* const declaration);
/**
	\brief Adds declaration after iterator. Declarations take ownership of passing pointer.

	\arg \c iterator - iterator of declaration to insert your declaration after.
	\arg \c declaration - your declaration.
*/
	Iterator addAfter(const Iterator& iterator, DeclarationBase* const declaration);

///	Converts declaration to iterator. 
	Iterator convertToIterator(DeclarationBase* const declaration);

///	Erase declaration form declarations block by iterator. Declaration will be deleted if no other ReprisePtrs points to it.
	void erase(Iterator& iterator);

/**
	\brief Replace one declaration with another. 
	Declarations take ownership of passing pointer. Declaration pointed with iterator will be deleted if no other ReprisePtrs points to it.

	\arg \c iterator - iterator of declaration to replace it by your declaration.
	\arg \c declaration - your declaration
*/
	void replace(Iterator& iterator, DeclarationBase* const declaration);

    void rename(DeclarationBase* declaration, const std::string& newName);

/// C declarations finds
	VariableDeclaration* findVariable(const std::string& name);
	TypeDeclaration* findType(const std::string& name);
	TypeDeclaration* findStruct(const std::string& name);
	TypeDeclaration* findUnion(const std::string& name);
	TypeDeclaration* findEnum(const std::string& name);
	SubroutineDeclaration* findSubroutine(const std::string& name);

//		RepriseBase implementation
	virtual int getChildCount(void) const;
	virtual RepriseBase& getChild(int index);

	virtual std::string dumpState(void) const;

	OPS_DEFINE_VISITABLE()
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()
	OPS_DEFINE_CLONABLE_INTERFACE(Declarations)

protected:
	Declarations(const Declarations& other);

private:
	typedef std::multimap<std::string, DeclarationBase*> THashMap;

	int getCount(DeclarationBase::DeclarationKind kind) const;

	DeclarationsType m_declarations;
	THashMap m_declHash;
};

template <class DeclType, class BaseIterator>
class DeclIteratorBase
{
public:
	typedef DeclType DeclarationType;

	DeclIteratorBase(DeclarationBase::DeclarationKind kind, const BaseIterator& iter, bool isLast)
		: m_kind(kind), m_iter(iter)
	{
		OPS_ASSERT(m_kind != DeclarationBase::DK_UNUSED);
		if (isLast)
			gotoPrevMatch();
		else
			gotoNextMatch();
	}

	DeclIteratorBase(const DeclIteratorBase<DeclType, BaseIterator>& iter)
		: m_kind(iter.m_kind), m_iter(iter.m_iter)
	{
	}

	template <class OtherBaseIterator>
	DeclIteratorBase(const DeclIteratorBase<DeclType, OtherBaseIterator>& iter)
		: m_kind(iter.getDeclKind()), m_iter(iter.getDeclIter())
	{
	}

	BaseIterator base() const
	{
		return m_iter;
	}

	bool isValid(void) const
	{
		return m_iter.isValid();
	}

	void goFirst(void)
	{
		m_iter.goFirst();
		gotoNextMatch();
	}

	void goLast(void)
	{
		m_iter.goLast();
		gotoPrevMatch();
	}


	DeclIteratorBase& operator=(const DeclIteratorBase<DeclType, BaseIterator>& iter)
	{
		m_kind = iter.m_kind;
		m_iter = iter.m_iter;
		return *this;
	}

	bool operator==(const DeclIteratorBase<DeclType, BaseIterator>& iter) const
	{
		return m_kind == iter.m_kind && m_iter == iter.m_iter;
	}
	
	bool operator!=(const DeclIteratorBase<DeclType, BaseIterator>& iter) const
	{
		return !(*this == iter);
	}

	DeclarationBase::DeclarationKind getDeclKind() const
	{
		return m_kind;
	}

	BaseIterator getDeclIter() const
	{
		return m_iter;
	}

protected:
	void goNext()
	{
		++m_iter;
		gotoNextMatch();
	}

	void goPrev()
	{
		--m_iter;
		gotoPrevMatch();
	}

	void gotoNextMatch()
	{
		while (m_iter.isValid() && m_iter->getKind() != m_kind)
		{
			++m_iter;
		}
	}
	void gotoPrevMatch()
	{
		while (m_iter.isValid() && m_iter->getKind() != m_kind)
		{
			--m_iter;
		}
	}

	DeclarationBase::DeclarationKind m_kind;
	BaseIterator m_iter;
};

template <class DeclType>
class DeclIterator : public DeclIteratorBase<DeclType, Declarations::Iterator>
{
	typedef DeclIteratorBase<DeclType, Declarations::Iterator> BaseDeclIterator;
public:
	typedef DeclType DeclarationType;

	DeclIterator(DeclarationBase::DeclarationKind kind, const Declarations::Iterator& iter, bool isLast)
		: BaseDeclIterator(kind, iter, isLast)
	{
	}
	
	DeclIterator(const DeclIterator<DeclType>& iter)
		: BaseDeclIterator(iter)
	{
	}

	DeclarationType& operator*()
	{
		return static_cast<DeclarationType&>(*BaseDeclIterator::m_iter);
	}
	
	DeclarationType* operator->()
	{
		return static_cast<DeclarationType*>(&*BaseDeclIterator::m_iter);
	}

	//	Prefix
	DeclIterator& operator++()
	{
		BaseDeclIterator::goNext();
		return *this;
	}

	DeclIterator& operator--()
	{
		BaseDeclIterator::goPrev();
		return *this;
	}

	//	Postfix
	DeclIterator operator++(int)
	{
		DeclIterator<DeclType> temp(*this);
		BaseDeclIterator::goNext();
		return temp;
	}

	DeclIterator operator--(int)
	{
		DeclIterator<DeclType> temp(*this);
		BaseDeclIterator::goPrev();
		return temp;
	}


};

template <class DeclType>
class ConstDeclIterator : public DeclIteratorBase<DeclType, Declarations::ConstIterator>
{
	typedef DeclIteratorBase<DeclType, Declarations::ConstIterator> BaseDeclIterator;
public:
	typedef DeclType DeclarationType;

	ConstDeclIterator(DeclarationBase::DeclarationKind kind, const Declarations::ConstIterator& iter, bool isLast)
		: BaseDeclIterator(kind, iter, isLast)
	{
	}
	
	ConstDeclIterator(const ConstDeclIterator<DeclType>& iter)
		: BaseDeclIterator(iter)
	{
	}

	ConstDeclIterator(const DeclIterator<DeclType>& iter)
		: BaseDeclIterator(iter)
	{
	}

	const DeclarationType& operator*() const
	{
		return static_cast<const DeclarationType&>(*BaseDeclIterator::m_iter);
	}
	
	const DeclarationType* operator->() const
	{
		return static_cast<const DeclarationType*>(&*BaseDeclIterator::m_iter);
	}

	//	Prefix
	ConstDeclIterator& operator++()
	{
		BaseDeclIterator::goNext();
		return *this;
	}

	ConstDeclIterator& operator--()
	{
		BaseDeclIterator::goPrev();
		return *this;
	}

	//	Postfix
	ConstDeclIterator operator++(int)
	{
		ConstDeclIterator<DeclType> temp(*this);
		BaseDeclIterator::goNext();
		return temp;
	}

	ConstDeclIterator operator--(int)
	{
		ConstDeclIterator<DeclType> temp(*this);
		BaseDeclIterator::goPrev();
		return temp;
	}


};


}
}

#endif                      // OPS_IR_REPRISE_DECLARATIONS_H_INCLUDED__
