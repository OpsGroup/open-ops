#ifndef OPS_STAGE_PASSES_H__
#define OPS_STAGE_PASSES_H__

//  Standard includes
#include <string>
#include <vector>
#include <map>
#include <memory>

//  OPS includes
#include "Reprise/Units.h"
#include "Reprise/ParserResult.h"

//  Local includes
#include "WorkContext.h"

//  Global defines and macros

//  Enter namespace
namespace OPS
{
namespace Stage
{
//  Constants and enums
typedef OPS::Core::ServiceUsage AnalysisUsage;

//  Global classes
class PassManager;

class PassBase : public OPS::NonCopyableMix
{
public:
    PassBase();
    virtual ~PassBase();

    /// Returns true if program has been modified
    virtual bool run() = 0;

    /// Returns list of analysis used by pass
    virtual AnalysisUsage getAnalysisUsage() const;

    void setManager(PassManager& manager);
    PassManager* manager() const;

protected:
    WorkContext& workContext();

    PassManager* m_manager;
};

class GeneratorBase : public PassBase
{
public:

};

class SerializerBase : public PassBase
{
};

class SimpleSerializerBase : public SerializerBase
{
public:
    explicit SimpleSerializerBase(const std::string& fileName);
    bool run();
    bool run(OPS::Reprise::ProgramUnit& program);

protected:
    virtual void serialize(OPS::Reprise::TranslationUnit& unit, std::ostream& os) = 0;
    virtual std::string generateFileName(OPS::Reprise::TranslationUnit& unit);

    std::string m_fileName;
};

class SatelliteSimpleSerializer : public SerializerBase
{
public:
    SatelliteSimpleSerializer(SimpleSerializerBase* simpleSerializer);
    bool run();
    //std::string getName() const;

protected:
    std::unique_ptr<SimpleSerializerBase> m_serializer;
};

class PassManager : public OPS::NonCopyableMix
{
public:
    typedef std::vector<PassBase*> TPasses;

    PassManager();
    ~PassManager();

    WorkContext& workContext();
    void addDiagnostics(const Reprise::CompilerResultMessage& message);

    void setGenerator(GeneratorBase* generator);
    void setSerializer(SerializerBase* serializer);

    void addPass(PassBase* pass);

    TPasses& passes();

    void run();

private:

    void runPass(PassBase& pass);

    WorkContext* m_context;
    GeneratorBase* m_generator;
    SerializerBase* m_serializer;
    TPasses m_passes;
};

struct PassInfo
{
    typedef PassBase*(PassConstructor)(void);

    PassInfo(const char* _name, PassConstructor _ctor)
        :name(_name), ctor(_ctor) {}

    const char* name;
    PassConstructor* ctor;
};

class PassRegistry : public OPS::NonCopyableMix
{
public:
    static PassRegistry& instance();

    void registerPass(const PassInfo& passInfo);
    void registerAnalysisPass(OPS::Core::ServiceLocatorBase::ServiceId interfaceId,
                              const PassInfo& passInfo);

    const PassInfo* getPassForAnalysis(OPS::Core::ServiceLocatorBase::ServiceId interfaceId) const;

private:
    PassRegistry();

    typedef std::list<PassInfo> PassInfoList;
    typedef std::map<OPS::Core::ServiceLocatorBase::ServiceId, PassInfo*> AnalysisPassesMap;

    PassInfoList m_passes;
    AnalysisPassesMap m_analysisPasses;
};

template <typename Pass>
    struct RegisterPass
{
    RegisterPass(const char* passName)
    {
        PassInfo pi(passName, createPass);
        PassRegistry::instance().registerPass(pi);
    }

    static PassBase* createPass()
    {
        return new Pass;
    }
};

template<typename Analysis, typename Pass>
    struct RegisterAnalysisPass
{
    RegisterAnalysisPass(const char* passName)
    {
        PassInfo pi(passName, RegisterPass<Pass>::createPass);
        PassRegistry::instance().registerAnalysisPass(OPS::Core::ServiceLocator::serviceId<Analysis>(), pi);
    }
};

//  Global functions

//  Exit namespace
}
}

#endif 						//	OPS_STAGE_PASSES_H__
