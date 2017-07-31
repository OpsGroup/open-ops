#include <iostream>
#include <fstream>

#include "OPS_Stage/Passes.h"
#include "OPS_Core/IO.h"

using namespace std;
using namespace OPS::Reprise;

namespace OPS
{
namespace Stage
{


//	PassBase class implementation
PassBase::PassBase() : m_manager(0)
{
}

PassBase::~PassBase()	
{ 
}

AnalysisUsage PassBase::getAnalysisUsage() const
{
    return AnalysisUsage();
}

void PassBase::setManager(PassManager& manager)
{
	m_manager = &manager;
}

PassManager* PassBase::manager() const
{
	return m_manager;
}

WorkContext& PassBase::workContext()
{
    return m_manager->workContext();
}

SimpleSerializerBase::SimpleSerializerBase(const string &fileName)
    :m_fileName(fileName)
{
}

bool SimpleSerializerBase::run(OPS::Reprise::ProgramUnit& program)
{
    for(int iUnit = 0; iUnit < program.getUnitCount(); ++iUnit)
    {
        OPS::Reprise::TranslationUnit& unit = program.getUnit(iUnit);

        std::string fileName = m_fileName;

        if (fileName.empty())
        {
            fileName = generateFileName(unit);
        }

        ofstream outStream(fileName.c_str());
        serialize(unit, outStream);
    }
    return false;
}

bool SimpleSerializerBase::run()
{
    return run(workContext().program());
}

std::string SimpleSerializerBase::generateFileName(Reprise::TranslationUnit &unit)
{
    return IO::getFileName(unit.getSourceFilename());
}

SatelliteSimpleSerializer::SatelliteSimpleSerializer(SimpleSerializerBase *simpleSerializer)
    :m_serializer(simpleSerializer)
{
}

bool SatelliteSimpleSerializer::run()
{
    RepriseList<ProgramUnit>::Iterator it = workContext().m_satellitePrograms.begin();
    for(; it != workContext().m_satellitePrograms.end(); ++it)
    {
        m_serializer->run(**it);
    }
    return false;
}

/*std::string SatelliteSimpleSerializer::getName() const
{
    return m_serializer->getName();
}*/


//		PassManager class implementation
PassManager::PassManager() : m_context(0), m_generator(0), m_serializer(0)
{
    m_context = new WorkContext();
}

PassManager::~PassManager()
{
    delete m_serializer;
    delete m_generator;
	for (TPasses::iterator it = m_passes.begin(); it != m_passes.end(); ++it)
	{
		delete *it;
	}
	m_passes.clear();
    delete m_context;
}

WorkContext& PassManager::workContext()
{
    return *m_context;
}

void PassManager::addDiagnostics(const Reprise::CompilerResultMessage &message)
{
    std::cerr << message.errorText() << std::endl;
}

void PassManager::setGenerator(GeneratorBase* generator)
{
    OPS_ASSERT(generator);
    m_generator = generator;
	m_generator->setManager(*this);
}

void PassManager::setSerializer(SerializerBase* serializer)
{
    OPS_ASSERT(serializer);
    m_serializer = serializer;
    m_serializer->setManager(*this);
}

void PassManager::addPass(PassBase* pass)
{
    OPS_ASSERT(pass);
    pass->setManager(*this);
    m_passes.push_back(pass);
}

PassManager::TPasses& PassManager::passes()
{
	return m_passes;
}

void PassManager::run()
{
    m_context->reset();

    if (m_generator)
        runPass(*m_generator);

    for(TPasses::iterator itPass = m_passes.begin(); itPass != m_passes.end(); ++itPass)
    {
        runPass(**itPass);
    }

    if (m_serializer)
        runPass(*m_serializer);
}

void PassManager::runPass(PassBase &pass)
{
    // check required analysis
    AnalysisUsage::ServiceList requiredServices = pass.getAnalysisUsage().getRequired();
    for(AnalysisUsage::ServiceList::iterator it = requiredServices.begin();
        it != requiredServices.end(); ++it)
    {
        if (!workContext().serviceExists(*it))
        {
            // run required passes
            if (const PassInfo* pi = PassRegistry::instance().getPassForAnalysis(*it))
            {
                if (PassBase* analysisPass = pi->ctor())
                {
                    analysisPass->setManager(*this);
                    runPass(*analysisPass);
                    delete analysisPass;
                }
            }
        }
        OPS_ASSERT(workContext().serviceExists(*it));
    }
    // run pass itsefl
    pass.run();
}

PassRegistry::PassRegistry()
{
}

PassRegistry& PassRegistry::instance()
{
    static PassRegistry g_intance;
    return g_intance;
}

void PassRegistry::registerPass(const PassInfo &passInfo)
{
    m_passes.push_back(passInfo);
}

void PassRegistry::registerAnalysisPass(Core::ServiceLocatorBase::ServiceId interfaceId, const PassInfo &passInfo)
{
    OPS_ASSERT(m_analysisPasses.find(interfaceId) == m_analysisPasses.end());
    m_passes.push_back(passInfo);
    m_analysisPasses[interfaceId] = &m_passes.back();
}

const PassInfo* PassRegistry::getPassForAnalysis(Core::ServiceLocatorBase::ServiceId interfaceId) const
{
    AnalysisPassesMap::const_iterator it = m_analysisPasses.find(interfaceId);
    if (it != m_analysisPasses.end())
        return it->second;
    else
        return 0;
}

}

}
