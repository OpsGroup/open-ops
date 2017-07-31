#include "OPS_Stage/ClangGenerator.h"

#include "OPS_Stage/GenericWriter.h"

namespace OPS
{
namespace Stage
{

static const char* const CLANG_GENERATOR_NAME = "ClangGenerator";


ClangGenerator::ClangGenerator(void)
{
}

ClangGenerator::~ClangGenerator()
{
}

std::string ClangGenerator::name()
{
	return CLANG_GENERATOR_NAME;
}

void ClangGenerator::run()
{
}

}
}
