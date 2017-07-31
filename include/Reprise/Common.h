#ifndef OPS_IR_REPRISE_COMMON_H_INCLUDED__
#define OPS_IR_REPRISE_COMMON_H_INCLUDED__


/*
	\todo

	1. Custom collections: vector, list, map. Iterator, ConstIterator.
	2. XML dump and print utility
	3. 
*/

#include <string>
#include <map>

#include "OPS_Core/Visitor.h"
#include "OPS_Core/Listener.h"
#include "Reprise/Utils.h"

/**
	\brief Dumping full nodes state.

	This option enables dumping full node state. By default its disabled, you can turn it on
	whenever your searching for bug. Do not put this options turned on to source control.
*/
#define OPS_REPRISE_DUMP_FULL_STATE 0
#define OPS_REPRISE_DUMP_NCID 0
#define OPS_REPRISE_COLLECT_CREATION_STATISTIC 0

#define OPS_REPRISE_DEFINE_GET_OBJECT_SIZE() \
	inline virtual size_t getObjectSize(void) const { return sizeof(*this); }


//	Enter namespaces
namespace OPS
{
namespace Reprise
{

class Lifetime;
class ProgramUnit;
class TranslationUnit;
class RepriseBase;
class SourceCodeManager;

typedef int TSourceCodeLocation;

class RepriseContext
{
public:
	RepriseContext(void);
	explicit RepriseContext(const char* workingDirectory);
	~RepriseContext();

	SourceCodeManager& getSourceCodeManager();

	static RepriseContext& defaultContext();

private:
	std::string m_workingDirectory;
	SourceCodeManager* m_sourceCodeManager;
};


///	Class to hold notes information
/**
	Notes in Reprise is like a hash-table which connected with each Reprise node.
	Keys in this hash-table is string values.
	Values in this hash-table -- this class.
*/
class Note
{
public:
	/// Note kinds
	enum NoteKind
	{
		NK_NONE = 0,
		NK_FLAG,
		NK_BOOL,
		NK_INT,
		NK_UINT,
		NK_INT64,
		NK_UINT64,
		NK_DOUBLE,
		NK_STRING,
		NK_WSTRING,
		NK_REPRISE
	};

	/// Create empty (NONE) note
	Note();
	///	Create 
	explicit Note(NoteKind kind);
	/// Copy constructor
	Note(const Note& other);
	///	Operator=
	Note& operator=(const Note& other);


	///	New flag note (kind NK_FLAG). No additional information needed. You can set this type of notes by RepriseBase::setNote(name)
	static Note newFlag();
	///	New boolean note (kind NK_BOOL). 
	static Note newBool(bool);
	///	New integer note (kind NK_BOOL). 
	static Note newInt(int);
	///	New unsigned integer note (kind NK_BOOL). 
	static Note newUInt(unsigned);
	///	New integer 64-bit note (kind NK_BOOL). 
	static Note newInt64(sqword);
	///	New unsigned integer 64-bit note (kind NK_BOOL). 
	static Note newUInt64(qword);
	///	New floating point note (kind NK_BOOL). 
	static Note newDouble(double);
	///	New string note (kind NK_BOOL). 
	static Note newString(const std::string&);
	///	New wide string note (kind NK_BOOL). 
	static Note newWString(const std::wstring&);
	///	New Reprise object holding note (kind NK_BOOL). 
	/**
		Use this kind of notes carefully. Because it stores Reprise node with reference counting. 
		It may be a way to hard-tracking errors. You have to avoid cyclic dependences which may lead to memory leaks.
	*/
	static Note newReprise(ReprisePtr<RepriseBase>& reprise);

	///	Get note kind.
	NoteKind getKind(void) const;
	///	Get boolean note value.
	bool getBool(void) const;
	///	Get integer note value.
	int getInt(void) const;
	///	Get unsigned integer note value.
	unsigned getUInt(void) const;
	///	Get integer 64-bit note value.
	sqword getInt64(void) const;
	///	Get unsigned integer 64-bit note value.
	qword getUInt64(void) const;
	///	Get floating point note value.
	double getDouble(void) const;
	///	Get string note value.
	const std::string getString(void) const;
	///	Get wide string note value.
	const std::wstring getWString(void) const;
	///	Get Reprise object note value (constant version).
	const RepriseBase& getReprise(void) const;
	///	Get Reprise object note value.
	RepriseBase& getReprise(void);
	/// Get Reprise object ReprisePtr
	ReprisePtr<RepriseBase> getReprisePtr(void) const;

	///	Set flag note value. You may use this method to change note kind to flag.
	void setFlag(void);
	///	Set boolean note value.
	void setBool(bool);
	///	Set integer note note value.
	void setInt(int);
	///	Set unsigned integer note value.
	void setUInt(unsigned);
	///	Set integer 64-bit note value.
	void setInt64(sqword);
	///	Set unsigned integer 64-bit note value.
	void setUInt64(qword);
	///	Set floating point note value.
	void setDouble(double);
	///	Set string note value.
	void setString(const std::string&);
	///	Set wide string note value.
	void setWString(const std::wstring&);
	///	Set Reprise object note value.
	void setReprise(const ReprisePtr<RepriseBase>& reprise);


private:
	void aquire(const Note& other);

	NoteKind m_kind;
	union
	{
		bool m_bool;
		int m_int;
		unsigned m_uint;
		sqword m_int64;
		qword m_uint64;
		double m_double;
	};
	ReprisePtr<RepriseBase> m_reprise;
};

///	Notes hash-table template class.
/**
		You can use this class to hold notes separated from Reprise nodes.
*/
template
<
	class TKey = std::string
>
class NotesMap : OPS::NonCopyableMix
{
	typedef std::map<TKey, Note> TNotesMap;

public:
	///	Has note by key
	bool has(const TKey& key)
	{
		return m_notes.find(key) != m_notes.end();
	}

	///	Get note by key (constant version)
	const Note& get(const TKey& key) const
	{
		typename TNotesMap::const_iterator it = m_notes.find(key);
		OPS_ASSERT(it != m_notes.end())
		return it->second;		
	}

	///	Get note by key
	Note& get(const TKey& key)
	{
		typename TNotesMap::iterator it = m_notes.find(key);
		OPS_ASSERT(it != m_notes.end())
		return it->second;		
	}

	///	Set note to key
	void set(const TKey& key, const Note& value)
	{
		m_notes.insert(std::make_pair(key, value));
	}

private:

	TNotesMap m_notes;
};


///	Base class for all Reprise nodes.
/**
	This class is base for all Reprise nodes. It contains useful general methods such us
	dumping node state, getting location for the node in source code, etc.
*/
class RepriseBase : public OPS::BaseVisitable<>, public BaseListenable, 
	public OPS::TypeConvertibleMix, 
	public IntrusivePointerBase,
	public ClonableMix<RepriseBase>
{
public:
	enum RepKind
	{
		RK_UNKNOWN = 0,

		//	Reprise types
		RK_BASIC_TYPE = 1000,
		RK_PTR_TYPE,
		RK_TYPEDEF_TYPE,
		RK_ARRAY_TYPE,
		RK_STRUCT_MEMBER,
		RK_STRUCT_TYPE,
		RK_ENUM_MEMBER,
		RK_ENUM_TYPE,
		RK_PARAMETER,
		RK_SUBROUTINE_TYPE,
		RK_DECLARED_TYPE,

		//	Canto types
		RK_CANTO_C_BASIC_TYPE = 1500,
		RK_CANTO_F_ARRAY_TYPE,

		//	Reprise declarations
		RK_VARIABLE_DECLARATION = 2000,
		RK_SUBROUTINE_DECLARATION,
		RK_TYPE_DECLARATION,
		RK_DECLARATIONS,

		//	Reprise expressions
		RK_BASIC_LITERAL_EXPRESSION = 3000,
		RK_STRICT_LITERAL_EXPRESSION,
		RK_COMPOUND_LITERAL_EXPRESSION,
		RK_REFERENCE_EXPRESSION,
		RK_SUBROUTINE_REFERENCE_EXPRESSION,
		RK_STRUCT_ACCESS_EXPRESSION,
		RK_ENUM_ACCESS_EXPRESSION,
		RK_TYPE_CAST_EXPRESSION,
		RK_BASIC_CALL_EXPRESSION,
		RK_SUBROUTINE_CALL_EXPRESSION,
		RK_EMPTY_EXPRESSION,

		//	Canto expressions
		RK_CANTO_C_CALL_EXPRESSION = 3500,
		RK_CANTO_F_DEFERRED_SHAPE_EXPRESSION,
		RK_CANTO_F_DIMENSION_EXPRESSION,
		RK_CANTO_F_SUBSCRIPT_PRIPLET_EXPRESSION,
		RK_CANTO_F_SECTION_SUBSCRIPT_LIST_EXPRESION,
		RK_CANTO_F_ARRAY_CONSTRUCTOR_EXPRESSION,
		RK_CANTO_F_IMPLIED_DO_EXPRESSION,
		RK_CANTO_F_INTRINSIC_CALL,

		//	Reprise statement
		RK_BLOCK_STATEMENT = 4000,
		RK_FOR_STATEMENT,
		RK_WHILE_STATEMENT,
		RK_IF_STATEMENT,
		RK_CASE_VARIANT,
		RK_SWITCH_STATEMENT,
		RK_PLAIN_CASE_LABEL,
		RK_PLAIN_SWITCH_STATEMENT,
		RK_GOTO_STATEMENT,
		RK_RETURN_STATEMENT,
		RK_BREAK_STATEMENT,
		RK_CONTINUE_STATEMENT,
		RK_EXPRESSION_STATEMENT,
		RK_EMPTY_STATEMENT,



	


	};

//	Constructors/destructor
	virtual ~RepriseBase(void);

/**
	\brief Getting child count.
	\return child node count
*/
	virtual int getChildCount(void) const = 0;

/**
	\brief Getting child.
	\arg \c index - child index. Call getChildCount() to determine child count.
*/
	virtual RepriseBase& getChild(int index) = 0;

/**
	\brief Getting link count.
	\return link node count
*/
	inline virtual int getLinkCount(void) const
	{
		return 0;
	}

/**
	\brief Getting link.
	\arg \c index - link index. Call getLinkCount() to determine link count.
*/
	inline virtual RepriseBase& getLink(int index)
	{
		OPS_UNUSED(index)
		throw RepriseError("Unexpected call to RepriseBase::getLink().");
	}

/**
	\brief Getting node (and all child nodes) state as string.
	\return string with node state.
*/
	virtual std::string dumpState(void) const;

/**
	\brief Gets parent node.

	Every node except ProgramUnit must have valid parent. Exclusion from this rule is a situation 
	when you have part of Reprise, for instance, the clone of some expression.
	\return parent node or 0 if no parent
*/
	RepriseBase* getParent(void) const;

/**
	\brief Sets parent node.

	You should use this function carefully. Usually, Reprise use this function internally.
*/
	void setParent(RepriseBase* parent);

///	Source location getter.
	TSourceCodeLocation getLocationId(void) const;
///	Source location setter.
	void setLocationId(TSourceCodeLocation location);

///	Note presence checker.
    bool hasNote(const std::string& name) const;
///	Gets note by name (constant version).
	const Note& getNote(const std::string& name) const;
///	Gets note by name.
	Note& getNote(const std::string& name);
///	Sets note by name.
	void setNote(const std::string& name, const Note& value);
///	Gets note by name to flag value.
	void setNote(const std::string& name);
/// Removes note by name
	void removeNote(const std::string& name);
///	Takes notes form another reprise node.
	void acquireNotes(const RepriseBase& node);

/**
	\brief Find and returns Program Unit to this node belong to.
	\return program unit or 0 if no program unit could be found.
*/
	ProgramUnit* findProgramUnit() const;

/**
	\brief Find and returns Translation Unit to this node belong to.
	\return translation unit or 0 if no translation unit could be found.
*/
	TranslationUnit* findTranslationUnit() const;

///	Gets object size in memory (useful for memory consume tracking)
	virtual size_t getObjectSize(void) const = 0;

/**
	\brief Gets the Node Creation ID (NCID).

	This is an experimental debug-purpose function. It may be removed in future releases. Anyway you should 
	use it only for debugging purposes, do not preserve.
	Every Reprise node have NCID. It's globally unique in program run. Every node take unique id, even node 
	clones have unique ids. Now IDs always incremented but it may hit multi-threading performance in the 
	future and may be removed.

	Recommended use: 
	  - View in debugger instead of pointers of nodes. May be extremely useful because this IDs are stable from run to run.
	  - After some Reprise modifications determine new nodes (they numbers greater than olders).

	\return node NCID.
*/
	unsigned getNCID() const;

	OPS_DEFINE_VISITABLE()
protected:
//	explicit RepriseBase(RepriseBase* parent);
	RepriseBase();
	RepriseBase(const RepriseBase&);

private:
	typedef std::map<std::string, Note> TNotes;

	RepriseBase* m_parent;
	TSourceCodeLocation m_location;
	TNotes m_notes;
	unsigned m_ncid;
	
	friend class Lifetime;
};

//	Exit namespaces
}
}


#endif                      // OPS_IR_REPRISE_COMMON_H_INCLUDED__
