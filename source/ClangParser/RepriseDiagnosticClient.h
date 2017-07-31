#ifndef REPRISE_DIAGNOSTIC_CLIENT_H
#define REPRISE_DIAGNOSTIC_CLIENT_H

#include <memory>
#include <list>
#include <utility>
#include "Reprise/Reprise.h"
#include "Reprise/ParserResult.h"

namespace clang
{
    class DiagnosticConsumer;
}

namespace OPS
{
    namespace Reprise
    {
        std::unique_ptr<clang::DiagnosticConsumer> createRepriseDiagnosticConsumer(std::list<CompilerResultMessage> &messageList);
    }
}
#endif
