#include "Backends/BackendsHub.h"

#include "Backends/RepriseXml/RepriseXml.h"
// OpenOPS comment: #include "Backends/HDL/BackendVHDL.h"
// OpenOPS comment: #include "Backends/ParallelLoopMarker.h"
// OpenOPS comment: #include "Backends/OpenMP/OpenMPProducer.h"
#include "Backends/OutToC/OutToC.h"
#include "Analysis/Profiler/ProfTextBuilder.h"
#include "OPS_Core/Localization.h"
// OpenOPS comment: #include "Backends/GPGPU/gpgpu.h"
#include "Analysis/DeadDeclarations.h"

// OpenOPS comment: #include "Backends/Generators/Filters/Filters.h"

using namespace OPS::Reprise;

namespace OPS
{
namespace TransformationsHub
{

void CodeGeneratorBase::setGeneratedCode(OPS::Reprise::ProgramUnit& program)
{
	for(int unit = 0; unit < program.getUnitCount(); ++unit)
	{
		std::stringstream os;
		Backends::OutToC	printer(os);
		printer.visit(program.getUnit(unit));
		m_generatedCode[OPS::Strings::format("unit%d.c", unit)] = os.str();
	}
}

const CodeGeneratorBase::ProgramCode& CodeGeneratorBase::getGeneratedCode() const
{
	return m_generatedCode;
}

class TransfRepriseXml : public CodeGeneratorBase
{
public:
	TransfRepriseXml()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::RepriseAny));
		m_argumentsInfo.back().mandatory = false;
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, "Write NCID"));
		m_argumentsInfo.back().defaultValue.setBool(true);
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, "Write parent NCID"));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, "Write source code location"));
	}

    virtual void makeTransformImpl(OPS::Reprise::ProgramUnit *program, const ArgumentValues &params)
	{
		m_generatedCode.clear();

		OPS::Reprise::RepriseBase* node = params[0].isValid()
										? params[0].getAsRepriseObject()
										: program;
        std::stringstream ss;
        OPS::XmlBuilder xmlBuilder(ss);
		OPS::Backends::RepriseXml::Options options;
		options.writeNCID = params[1].getAsBool();
		options.writeNCIDofParent = params[2].getAsBool();
		options.writeSourceCodeLocation = params[3].getAsBool();

		OPS::Backends::RepriseXml backend(xmlBuilder, options);
		node->accept(backend);

        m_generatedCode["dump.xml"] = ss.str();
	}
};

class TransfProfiling : public CodeGeneratorBase
{
public:
	TransfProfiling()
	{
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, "Profiling with subroutine calls"));
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, "Show result only for header"));
		m_argumentsInfo.addListArg("Profiling result", "Time", "Size code", "Size data");
	}

    virtual void makeTransformImpl(OPS::Reprise::ProgramUnit *program, const ArgumentValues &params)
	{
		m_generatedCode.clear();

		OPS::Profiling::HardwareIndependentProfiler hardwareIndependentProfiler(params[0].getAsBool());
		if (hardwareIndependentProfiler.Run(program))
		{
			OPS::Profiling::Profiler profiler(L"");
			if (profiler.Run(hardwareIndependentProfiler))
			{
				OPS::Profiling::ProfTextBuilder profTextBuilder(program, &profiler, (Profiling::ShowsProfilingResult)params[2].getAsInt(), params[1].getAsBool());
				m_generatedCode["profiling.inf"] = profTextBuilder.GetResult();
			}
			else
				throw OPS::RuntimeError("Failed profiling");
		}		
		else
			throw OPS::RuntimeError("Failed profiling");
	}
};
/* OpenOPS comment:
class TransfOpenMPProducer : public CodeGeneratorBase
{
public:
	TransfOpenMPProducer()
	{
		m_argumentsInfo.addListArg("OpenMP version", "1.0", "2.0", "2.5", "3.0");
		m_argumentsInfo.back().defaultValue.setInt(3);
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, _TL("Use Lattice graphs","Использовать Решетчатые графы")));
		m_argumentsInfo.back().defaultValue.setBool(true);
		m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, _TL("Use Montego DepGraph","Использовать граф зависимостей Montego")));
		m_argumentsInfo.back().defaultValue.setBool(true);
	}

    virtual void makeTransformImpl(OPS::Reprise::ProgramUnit *program, const ArgumentValues &params)
	{
		OPS_UNUSED(params);
		m_generatedCode.clear();

		using namespace OPS::Backends::OpenMP;

		OpenMPRevision revision = OMP_3_0;
		switch(params[0].getAsInt())
		{
		case 0: revision = OMP_1_0; break;
		case 1: revision = OMP_2_0; break;
		case 2: revision = OMP_2_5; break;
		case 3: revision = OMP_3_0; break;
		OPS_DEFAULT_CASE_LABEL;
		}

		OpenMPConstructs constructs;
		constructs.parallelLoopConstructs = getParallelLoops(*program, revision, params[1].getAsBool(), params[2].getAsBool());
		std::stringstream os;

		if (program->getUnit(0).getSourceLanguage() == TranslationUnit::SL_C)
		{
			OpenMPOutToC out(constructs, os);
            std::set<Reprise::DeclarationBase*> usedDeclarations;
            if (Reprise::SubroutineDeclaration* mainDecl = program->getUnit(0).getGlobals().findSubroutine("main"))
            {
                OPS::Analysis::obtainUsedDeclarations(&mainDecl->getDefinition(), usedDeclarations);
                out.getSettings().usedDeclarations = &usedDeclarations;
            }
			program->getUnit(0).accept(out);
		}
		else
		{
			OpenMPOutToFortran out(constructs, os);
			program->getUnit(0).accept(out);
		}

		m_generatedCode["out.c"] = os.str();
	}
}; */

/* OpenOPS comment:
class TransfGPGPUCodeGenerator : public CodeGeneratorBase
{
public:
    TransfGPGPUCodeGenerator()
    {
        m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::StmtFor));
        m_argumentsInfo.back().mandatory = true;
        m_argumentsInfo.addListArg("Technology", "CUDA", "OpenCL");
        m_argumentsInfo.back().defaultValue.setInt(0);
        m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Bool, "check dependency"));
        m_argumentsInfo.back().defaultValue.setBool(false);
        m_argumentsInfo.push_back(ArgumentInfo(ArgumentValue::Int, "loop collapse count (-1 - auto)"));
        m_argumentsInfo.back().defaultValue.setInt(1);
    }

    virtual void makeTransformImpl(OPS::Reprise::ProgramUnit *program, const ArgumentValues &params)
    {
        m_generatedCode.clear();

        OPS::Reprise::ForStatement* fstmt = params[0].getAsFor();
        OPS::Backends::GPGPU::GPUCodeGenerator gpu;
        OPS::Backends::GPGPU::GPUProgramTechnology tech = params[1].getAsInt() == 0 ?
                    OPS::Backends::GPGPU::CUDAProgramTechnology : OPS::Backends::GPGPU::OpenCLProgramTechnology;
        OPS::Backends::GPGPU::ProgramFragment pf(*fstmt, params[2].getAsBool(), params[3].getAsInt());
        gpu.parallizeFragment(pf, tech);
        if (gpu.getCheckResult() == false)
            throw OPS::RuntimeError(gpu.getErrorMessage());
        std::string stech = params[1].getAsInt() == 0 ? "CUDA" : "OpenCL";
        m_generatedCode["Generated " + stech + " code"] = gpu.getProgramCodeString()[0];
    }
}; */


// OpenOPS comment: typedef OPS::Backends::HDL::BackendVHDL TransfVHDLGenerator;
// OpenOPS comment: typedef OPS::Backends::ParallelLoopMarker TransfParallelLoopMarker;
// OpenOPS comment: typedef OPS::Backends::Filters TransfFilters;

#define FACTORY_IMPL(transformation)\
	TransformBase* create##transformation()	{ return new Transf##transformation; }

FACTORY_IMPL(RepriseXml);
// OpenOPS comment: FACTORY_IMPL(VHDLGenerator);
FACTORY_IMPL(Profiling);
// OpenOPS comment: FACTORY_IMPL(ParallelLoopMarker);
// OpenOPS comment: FACTORY_IMPL(OpenMPProducer);
// OpenOPS comment: FACTORY_IMPL(GPGPUCodeGenerator);
// OpenOPS comment: FACTORY_IMPL(Filters)

void registerBackends()
{
	typedef TransformFactory::TransformDescription TransformDescription;
	TransformFactory& g_factory = TransformFactory::instance();
	g_factory.registerTransform("Reprise XML", TransformDescription("Backends", createRepriseXml));
// OpenOPS comment: 	g_factory.registerTransform("VHDL Generator", TransformDescription("Backends", createVHDLGenerator, "Backends/HDL"));
// OpenOPS comment: 	g_factory.registerTransform("Parallel Loop Marker", TransformDescription("Backends", createParallelLoopMarker, "Analysis/ParallelLoops"));
// OpenOPS comment: 	g_factory.registerTransform("OpenMP Generator", TransformDescription("Backends", createOpenMPProducer, "Backends/OpenMP"));
	g_factory.registerTransform("Profiling", TransformDescription("Information", createProfiling));
// OpenOPS comment:     g_factory.registerTransform("GPGPU code generator", TransformDescription("Backends", createGPGPUCodeGenerator));
// OpenOPS comment: 	g_factory.registerTransform("Filters generator", TransformDescription("Backends", createFilters, "Backends/Generators/Filters"));
}

}
}
