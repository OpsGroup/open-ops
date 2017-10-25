#include "Shared/ProgramContext.h"
#include "Shared/RepriseClone.h"

namespace OPS
{
namespace Shared
{

using namespace OPS::Reprise;

static const char* const PROGRAM_CONTEXT_NOTE = "ProgramContext";

std::vector<const char*> IMetaInformation::getDependencies() const
{
	return std::vector<const char*>();
}

IMetaInformation::~IMetaInformation()
{
}

class ProgramContextHelper : public OPS::Reprise::RepriseBase
{
public:
	ProgramContextHelper(ProgramContext* context):m_context(context) {}
	~ProgramContextHelper() { delete m_context; }

	ProgramContext* getContext() const { return m_context; }

	ProgramContextHelper* clone() const { return new ProgramContextHelper(m_context); }
	int getChildCount() const { return 0; }
	RepriseBase& getChild(int) { throw RuntimeError("ProgramContextHelper has no childs"); }
	OPS_REPRISE_DEFINE_GET_OBJECT_SIZE()

private:
	ProgramContext* m_context;
};

ProgramContext::ProgramContext(Reprise::RepriseWeakPtr<Reprise::ProgramUnit> program)
	:m_program(program)
{
	if (m_program->hasNote(PROGRAM_CONTEXT_NOTE))
	{
		throw StateError("ProgramContext initialization error: program already has context");
	}

	ReprisePtr<RepriseBase> helper(new ProgramContextHelper(this));
	m_program->setNote(PROGRAM_CONTEXT_NOTE, Note::newReprise(helper));
}

ProgramContext::~ProgramContext()
{
	clearMetaInformation();
	// m_program->clearNote(PROGRAM_CONTEXT_NOTE);
}

ProgramContext* ProgramContext::getFromProgram(Reprise::ProgramUnit &program)
{
	if (program.hasNote(PROGRAM_CONTEXT_NOTE))
	{
		Note note = program.getNote(PROGRAM_CONTEXT_NOTE);
		if (note.getKind() == Note::NK_REPRISE)
		{
			if (ProgramContextHelper* helper = note.getReprise().cast_ptr<ProgramContextHelper>())
				return helper->getContext();
		}

		throw OPS::RuntimeError(std::string(PROGRAM_CONTEXT_NOTE) + " note is not ProgramContextHelper");
	}
	else
	{
		ProgramContext* context = new ProgramContext(RepriseWeakPtr<ProgramUnit>(&program));
		return context;
	}
}

void ProgramContext::removeContextNote(ProgramUnit &program)
{
    program.removeNote(PROGRAM_CONTEXT_NOTE);
}

ProgramUnit& ProgramContext::getProgram()
{
	return *m_program;
}

/*
ProgramContext* ProgramContext::cleanClone()
{
	ReprisePtr<ProgramUnit> program(deepCloneProgramUnit(*m_program));
	return new ProgramContext(RepriseWeakPtr(program);
}
*/

void ProgramContext::programModified()
{
	clearMetaInformation();
}

void ProgramContext::addMetaInformation(IMetaInformation* meta)
{
	OPS_ASSERT(meta != 0);
	m_metaStorage[meta->getUniqueId()] = meta;
}

void ProgramContext::removeMetaInformation(IMetaInformation* meta)
{
	OPS_ASSERT(meta != 0);
	m_metaStorage.erase(meta->getUniqueId());
	delete meta;
}

void ProgramContext::clearMetaInformation()
{
	MetaStorage::iterator it = m_metaStorage.begin();
	for(; it != m_metaStorage.end(); ++it)
		delete it->second;
	m_metaStorage.clear();
}

IMetaInformation* ProgramContext::getMetaInformation(const char *uniqueId)
{
	MetaStorage::const_iterator it = m_metaStorage.find(uniqueId);
	if (it != m_metaStorage.end())
		return it->second;
	else
		return 0;
}

}
}
