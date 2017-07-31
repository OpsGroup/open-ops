#ifndef PRAGMAHANDLERS_H
#define PRAGMAHANDLERS_H

#include <vector>
#include "clang/Lex/Pragma.h"
#include "clang/Basic/SourceLocation.h"

namespace OPS
{
namespace ClangParser
{

typedef std::vector< std::pair< clang::SourceLocation, std::pair<std::string, std::string> > > Pragmas;

class PragmaHandlerBase : public clang::PragmaHandler
{
    Pragmas& pragmas;
protected:

    void registerPragma(const clang::SourceLocation& location,
                        const std::string& name,
                        const std::string& value);

public:
    PragmaHandlerBase(llvm::StringRef name, Pragmas& p);
};

clang::PragmaHandler* createOpsPragmaHandlers(Pragmas& p);

void registerPragmaHandler(clang::PragmaHandler*(*factory)(Pragmas& p));

}
}

#endif // PRAGMAHANDLERS_H
