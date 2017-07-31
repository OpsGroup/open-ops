#pragma once

#include "OPS_Stage/Passes.h"
#include "clangParserSettings.h"

namespace OPS
{
namespace Stage
{

class ClangGenerator : public GeneratorBase
{
public:
	ClangGenerator();
	virtual ~ClangGenerator();

    virtual bool run();

    clang::ClangParserSettings& settings() { return m_settings; }

    void addFile(const std::string& fileName);

private:
    typedef std::list<std::string> FileList;

    clang::ClangParserSettings m_settings;
    FileList m_fileNames;
};

static RegisterPass<ClangGenerator> clangGenerator("Clang Generator");

}
}
