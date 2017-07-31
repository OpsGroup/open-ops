#pragma once

#include "OPS_Stage/Passes.h"

namespace OPS
{
namespace Stage
{

class ClangGenerator : public GeneratorBase
{
public:
	ClangGenerator();
	virtual ~ClangGenerator();

	virtual std::string name();
	virtual void run();

private:
};

}
}
