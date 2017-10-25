#include "Transforms/Helpers/IsolateFrame/IsolateFrame.h"
#include "Reprise/Reprise.h"

namespace OPS
{
namespace Transforms
{
namespace Helpers
{
using namespace Reprise;


ReprisePtr<ProgramUnit> isolateFrame(StatementBase& frame, ReplaceParams& to_replace, VariableList &localVariables)
{
    StatementList one_statement_list;
    one_statement_list.add(&frame);
    return isolateFrame(one_statement_list, to_replace, localVariables);
}

ReprisePtr<ProgramUnit> isolateFrame(StatementList& frame, ReplaceParams& to_replace, VariableList &localVariables)
{
    ReprisePtr<BlockStatement> main_body(new BlockStatement());
    for(StatementList::Iterator i = frame.begin();
        i!=frame.end();
        ++i)
    {
        main_body->addLast(*i);
    }


    SubroutineType* main_type = new SubroutineType(BasicType::int32Type());
    SubroutineDeclaration* main_decl = new SubroutineDeclaration(main_type, "main");
    main_decl->setBodyBlock(main_body);

    for(VariableList::Iterator i = localVariables.begin(); i != localVariables.end(); ++i)
    {
        main_decl->getDeclarations().addLast(*i);
    }

    ReprisePtr<TranslationUnit> inner_unit (new TranslationUnit(TranslationUnit::SL_C));
    Declarations& global_decl=inner_unit->getGlobals();
    global_decl.addLast(main_decl);

    ReprisePtr<ProgramUnit> to_return (new ProgramUnit());
    to_return->addTranslationUnit(inner_unit);

    ReplaceMap replace_map = prepareCadreDeclaraions(to_replace);

    replaceCadreVariables(*main_body, replace_map);

    return to_return;
}



}
}
}
