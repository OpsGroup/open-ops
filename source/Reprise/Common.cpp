#include "Reprise/Common.h"
#include "Reprise/Expressions.h"
#include "Reprise/Layouts.h"
#include "Reprise/Units.h"


namespace OPS
{
namespace Reprise
{

static unsigned g_nextNCID = 0;
static RepriseContext g_defaultContext;

//	RepriseContext class implementation
#if OPS_COMPILER_MSC_VER >= 1310
#pragma warning(push)
#pragma warning(disable: 4355)
#endif
RepriseContext::RepriseContext(void) : m_sourceCodeManager(new SourceCodeManager(*this))
#if OPS_COMPILER_MSC_VER >= 1310
#pragma warning(pop)
#endif
{
}

#if OPS_COMPILER_MSC_VER >= 1310
#pragma warning(push)
#pragma warning(disable: 4355)
#endif
RepriseContext::RepriseContext(const char* workingDirectory) : m_workingDirectory(workingDirectory), m_sourceCodeManager(new SourceCodeManager(*this))
#if OPS_COMPILER_MSC_VER >= 1310
#pragma warning(pop)
#endif
{
}

RepriseContext::~RepriseContext(void)
{
	delete m_sourceCodeManager;
}

SourceCodeManager& RepriseContext::getSourceCodeManager()
{
	return *m_sourceCodeManager;
}

RepriseContext& RepriseContext::defaultContext()
{
	return g_defaultContext;
}


//	Note class implementation
Note::Note() 
	: m_kind(NK_NONE)
{
}

Note::Note(NoteKind kind)
	: m_kind(kind)
{
}

Note::Note(const Note& other)
	: m_kind(NK_NONE)
{
	aquire(other);
}

Note& Note::operator=(const Note& other)
{
	aquire(other);
	return *this;
}

Note Note::newBool(bool value)
{
	Note result(NK_BOOL);
	result.m_bool = value;
	return result;
}

Note Note::newFlag()
{
	return Note(NK_FLAG);
}

Note Note::newInt(int value)
{
	Note result(NK_INT);
	result.m_int = value;
	return result;
}

Note Note::newUInt(unsigned value)
{
	Note result(NK_UINT);
	result.m_uint = value;
	return result;
}

Note Note::newInt64(sqword value)
{
	Note result(NK_INT64);
	result.m_int64 = value;
	return result;
}

Note Note::newUInt64(qword value)
{
	Note result(NK_UINT64);
	result.m_uint64 = value;
	return result;
}

Note Note::newDouble(double value)
{
	Note result(NK_DOUBLE);
	result.m_double = value;
	return result;
}

Note Note::newString(const std::string& value)
{
	Note result(NK_STRING);
	result.m_reprise.reset(BasicLiteralExpression::createString(value));
	return result;
}

Note Note::newWString(const std::wstring& value)
{
	Note result(NK_WSTRING);
	result.m_reprise.reset(BasicLiteralExpression::createWideString(value));
	return result;
}

Note Note::newReprise(ReprisePtr<RepriseBase>& reprise)
{
	Note result(NK_REPRISE);
	result.m_reprise = reprise;
	return result;
}

Note::NoteKind Note::getKind(void) const
{
	return m_kind;
}

bool Note::getBool(void) const
{
	OPS_ASSERT(m_kind == NK_BOOL)
	return m_bool;
}

int Note::getInt(void) const
{
	OPS_ASSERT(m_kind == NK_INT)
	return m_int;
}

unsigned Note::getUInt(void) const
{
	OPS_ASSERT(m_kind == NK_UINT)
	return m_uint;
}

sqword Note::getInt64(void) const
{
	OPS_ASSERT(m_kind == NK_INT64)
	return m_int64;
}

qword Note::getUInt64(void) const
{
	OPS_ASSERT(m_kind == NK_UINT64)
	return m_uint64;
}

double Note::getDouble(void) const
{
	OPS_ASSERT(m_kind == NK_DOUBLE)
	return m_double;
}

const std::string Note::getString(void) const
{
	OPS_ASSERT(m_kind == NK_STRING)
	OPS_ASSERT(m_reprise.get() != 0)
	return static_cast<BasicLiteralExpression*>(m_reprise.get())->getString();
}

const std::wstring Note::getWString(void) const
{
	OPS_ASSERT(m_kind == NK_WSTRING)
	OPS_ASSERT(m_reprise.get() != 0)
	return static_cast<BasicLiteralExpression*>(m_reprise.get())->getWideString();
}

const RepriseBase& Note::getReprise(void) const
{
	OPS_ASSERT(m_kind == NK_REPRISE)
	OPS_ASSERT(m_reprise.get() != 0)
	return *m_reprise;
}

RepriseBase& Note::getReprise(void)
{
	OPS_ASSERT(m_kind == NK_REPRISE)
	OPS_ASSERT(m_reprise.get() != 0)
	return *m_reprise;
}

ReprisePtr<RepriseBase> Note::getReprisePtr(void) const
{
	OPS_ASSERT(m_kind == NK_REPRISE)
	OPS_ASSERT(m_reprise.get() != 0)
	return m_reprise;
}


void Note::setFlag(void)
{
	m_kind = NK_FLAG;
}

void Note::setBool(bool value)
{
	m_kind = NK_BOOL;
	m_bool = value;
}

void Note::setInt(int value)
{
	m_kind = NK_INT;
	m_int = value;
}

void Note::setUInt(unsigned value)
{
	m_kind = NK_UINT;
	m_uint = value;
}

void Note::setInt64(sqword value)
{
	m_kind = NK_INT64;
	m_int64 = value;

}

void Note::setUInt64(qword value)
{
	m_kind = NK_UINT64;
	m_uint64 = value;
}

void Note::setDouble(double value)
{
	m_kind = NK_DOUBLE;
	m_double = value;
}

void Note::setString(const std::string& value)
{
	m_kind = NK_STRING;
	m_reprise.reset(BasicLiteralExpression::createString(value));
}

void Note::setWString(const std::wstring& value)
{
	m_kind = NK_WSTRING;
	m_reprise.reset(BasicLiteralExpression::createWideString(value));
}

void Note::setReprise(const ReprisePtr<RepriseBase>& reprise)
{
	m_kind = NK_REPRISE;
	m_reprise = reprise;
}

void Note::aquire(const Note& other)
{
	m_kind = other.m_kind;
	switch (m_kind)
	{
	case NK_NONE:
		break;

	case NK_FLAG:
		setFlag();
		break;

	case NK_BOOL:
		setBool(other.getBool());
		break;

	case NK_INT:
		setInt(other.getInt());
		break;

	case NK_UINT:
		setUInt(other.getUInt());
		break;

	case NK_INT64:
		setInt64(other.getInt64());
		break;

	case NK_UINT64:
		setUInt64(other.getUInt64());
		break;

	case NK_DOUBLE:
		setDouble(other.getDouble());
		break;

	case NK_STRING:
		setString(other.getString());
		break;

	case NK_WSTRING:
		setWString(other.getWString());
		break;

	case NK_REPRISE:
		{
			setReprise(other.getReprisePtr());
		}
		break;
		OPS_DEFAULT_CASE_LABEL
	}
}


//	RepriseBase class implementation
RepriseBase::RepriseBase() 
	: m_parent(0), m_location(-1), m_ncid(++g_nextNCID)
{
#if OPS_REPRISE_COLLECT_CREATION_STATISTIC
	Coordinator::instance().addNode(this);
#endif
}

RepriseBase::RepriseBase(const RepriseBase& other)
	:IntrusivePointerBase(), m_parent(0), m_location(other.m_location), m_ncid(++g_nextNCID)
{
	m_notes = other.m_notes;
}

RepriseBase::~RepriseBase(void)
{
#if OPS_REPRISE_COLLECT_CREATION_STATISTIC
	Coordinator::instance().removeNode(this);
#endif
}

std::string RepriseBase::dumpState(void) const
{
#if OPS_REPRISE_DUMP_FULL_STATE
//	TODO:(Top) Add location dump here
	return Strings::format("(parent: %X, this: %X) ", m_parent, this);
#elif OPS_REPRISE_DUMP_NCID
	return Strings::format("(NCID: %u)", m_ncid);
#else
	return "";
#endif
}

RepriseBase* RepriseBase::getParent(void) const
{
	return m_parent;
}

void RepriseBase::setParent(RepriseBase* parent)
{
	m_parent = parent;
}

TSourceCodeLocation RepriseBase::getLocationId(void) const
{
	return m_location;
}

void RepriseBase::setLocationId(TSourceCodeLocation location)
{
	m_location = location;
}

bool RepriseBase::hasNote(const std::string& name) const
{
	return m_notes.find(name) != m_notes.end();
}

const Note& RepriseBase::getNote(const std::string& name) const
{
	TNotes::const_iterator iter = m_notes.find(name);
	if (iter != m_notes.end())
		return iter->second;
	else
		throw RepriseError(Strings::format("Unexpected getting note '%s'.", name.c_str()));
}

Note& RepriseBase::getNote(const std::string& name)
{
	TNotes::iterator iter = m_notes.find(name);
	if (iter != m_notes.end())
		return iter->second;
	else
		throw RepriseError(Strings::format("Unexpected getting note '%s'.", name.c_str()));
}

void RepriseBase::setNote(const std::string& name, const Note& value)
{
	if (!m_notes.insert(std::make_pair(name, value)).second)
		throw RepriseError(Strings::format("Note '%s' already set.", name.c_str()));
}

void RepriseBase::setNote(const std::string& name)
{
	if (!m_notes.insert(std::make_pair(name, Note::newFlag())).second)
		throw RepriseError(Strings::format("Note '%s' already set.", name.c_str()));
}

void RepriseBase::removeNote(const std::string &name)
{
	TNotes::iterator iter = m_notes.find(name);
	if (iter != m_notes.end())
		m_notes.erase(iter);
	else
		throw RepriseError(Strings::format("Unexpected removing note '%s'.", name.c_str()));
}

void RepriseBase::acquireNotes(const RepriseBase& node)
{
	m_notes = node.m_notes;
}

ProgramUnit* RepriseBase::findProgramUnit() const
{
	ProgramUnit* program = 0;
	RepriseBase* parent = const_cast<RepriseBase*>(this);
	do 
	{
		program = dynamic_cast<ProgramUnit*>(parent);
		if (program != 0)
		{
			return program;
		}
		else
			parent = parent->getParent();
	} 
	while (parent != 0);
	return 0;
}

TranslationUnit* RepriseBase::findTranslationUnit() const
{
	TranslationUnit* unit = 0;
	RepriseBase* parent = const_cast<RepriseBase*>(this);
	do
	{
		unit = dynamic_cast<TranslationUnit*>(parent);
		if (unit != 0)
		{
			return unit;
		}
		else
			parent = parent->getParent();
	}
	while (parent != 0);
	return 0;
}

unsigned RepriseBase::getNCID() const
{
	return m_ncid;
}

}
}
