#ifndef OPS_SHARED_PROGRAMCONTEXT_H
#define OPS_SHARED_PROGRAMCONTEXT_H

#include <Reprise/Units.h>

namespace OPS
{
namespace Shared
{

/// Interface for any kind of meta-information about program
class IMetaInformation
{
public:
	virtual const char* getUniqueId() const = 0;
	virtual std::vector<const char*> getDependencies() const;

	//virtual IMetaInformation* clone(Reprise::ProgramUnit& cloneProgram) = 0;

	virtual bool programModified() { return false; }

	virtual ~IMetaInformation();
};

class ProgramContext
{
public:
    explicit ProgramContext(Reprise::RepriseWeakPtr<Reprise::ProgramUnit> program);
	~ProgramContext();

    static ProgramContext* getFromProgram(Reprise::ProgramUnit& program);
    static void removeContextNote(Reprise::ProgramUnit& program);

	Reprise::ProgramUnit& getProgram();

	/// Return copy of state with all stored metainformation
	ProgramContext* clone();

	/// Return copy of state without any metainformation
	//ProgramContext* cleanClone();

	void programModified();

	void addMetaInformation(IMetaInformation*);
	void removeMetaInformation(IMetaInformation*);

	void clearMetaInformation();

	IMetaInformation* getMetaInformation(const char* uniqueId);

	template<typename MetaClass>
		MetaClass* getMetaInformation() const
	{
		MetaStorage::const_iterator it = m_metaStorage.find(MetaClass::UNIQUE_ID);
		if (it != m_metaStorage.end())
			return dynamic_cast<MetaClass*>(it->second);
		else
			return 0;
	}

	template<typename MetaClass>
		MetaClass& getMetaInformationSafe()
	{
		MetaStorage::iterator it = m_metaStorage.find(MetaClass::UNIQUE_ID);
		if (it != m_metaStorage.end())
		{
			return *dynamic_cast<MetaClass*>(it->second);
		}
		else
		{
			MetaClass* meta = new MetaClass(*this);
			addMetaInformation(meta);
			return *meta;
		}
	}

private:
	typedef std::map<const char*, IMetaInformation*> MetaStorage;

	Reprise::RepriseWeakPtr<Reprise::ProgramUnit> m_program;
	MetaStorage m_metaStorage;
};

}
}

#endif // OPS_SHARED_PROGRAMCONTEXT_H
