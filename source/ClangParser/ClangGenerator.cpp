#include "ClangParser/ClangGenerator.h"
#include "ClangParser/clangParser.h"
#include "Reprise/Declarations.h"

namespace OPS
{
namespace Stage
{

ClangGenerator::ClangGenerator(void)
{
}

ClangGenerator::~ClangGenerator()
{
}

bool ClangGenerator::run()
{
    using namespace OPS::Reprise;

    for(FileList::iterator fileName = m_fileNames.begin(); fileName != m_fileNames.end(); ++fileName)
    {
        try
        {
            Reprise::CompileResult result;
            ReprisePtr<TranslationUnit> unit;

            clang::parseFile(
							*fileName, unit, result.m_messages, false, m_settings);

            if (unit.get())
                workContext().program().addTranslationUnit(unit);

            for(CompileResult::MessageList::iterator it = result.m_messages.begin()
                ; it != result.m_messages.end(); ++it)
                manager()->addDiagnostics(*it);
        }
        catch(OPS::Exception& e)
        {
            manager()->addDiagnostics(CompilerResultMessage(CompilerResultMessage::CRK_ERROR, e.what(), "", *fileName,
                                      CompilerResultMessage::TLocation(0,0), ""));
            return false;
        }
    }

	if (workContext().program().getUnitCount() != int(m_fileNames.size()))
        throw OPS::RuntimeError("too many errors while parsing input files");
	return true;
}

void ClangGenerator::addFile(const std::string &fileName)
{
    m_fileNames.push_back(fileName);
}

}
}
