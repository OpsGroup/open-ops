/*
	FrontTransforms/Resolver.cpp - FrontTransforms module, resolver implementation

*/

//  Standard includes

//  OPS includes

//  Local includes
#include "FrontTransforms/Resolver.h"

//  Namespaces using
using namespace std;
using namespace OPS::Reprise;

//  Defines and macros

//  Enter namespace
namespace OPS
{
namespace Transforms
{

//  Constants and enums

//  Classes

//  Functions declaration

//  Variables

//  Classes implementation

//  Global classes implementation
//		Resolver class implementation
Resolver::Resolver(void) : m_program(0)
{
}

void Resolver::setProgram(ProgramUnit& program)
{
	m_errorList.clear();
	m_program = &program;
}

int Resolver::getErrorCount(void) const
{
	return static_cast<int>(m_errorList.size());
}

string Resolver::getError(int index) const
{
	return m_errorList[index];
}

void Resolver::resolve()
{
	m_errorList.clear();
    TSubroutinesMap externalSubroutines;
    TVariablesMap externalVariables;
	for (int unitIndex = 0; unitIndex < m_program->getUnitCount(); ++unitIndex)
	{
        resolveUnit(m_program->getUnit(unitIndex), externalSubroutines, externalVariables);
	}

	for (int unitIndex = 0; unitIndex < m_program->getUnitCount(); ++unitIndex)
	{
        setupDefinitionsUnit(m_program->getUnit(unitIndex), externalSubroutines, externalVariables);
	}
}


void Resolver::resolveUnit(TranslationUnit &unit, TSubroutinesMap &externalSubs, TVariablesMap &externalVars)
{
    TSubroutinesMap localSubs;
    TVariablesMap localVars;
    for (Declarations::Iterator iter = unit.getGlobals().getFirst(); iter.isValid(); ++iter)
	{
        if (SubroutineDeclaration* subRoutine = iter->cast_ptr<SubroutineDeclaration>())
        {
            const bool isStatic = subRoutine->getDeclarators().isStatic();
            const bool isInline = subRoutine->getDeclarators().isInline();
            if (subRoutine->hasImplementation())
            {
                if (isStatic || isInline)
                {
                    if (!localSubs.insert(make_pair(subRoutine->getName(), subRoutine)).second)
                    {
                        reportError(Strings::format("Duplicate subroutine found: '%hs'", subRoutine->getName().c_str()));
                    }
                }
                else
                {
                    if (!externalSubs.insert(make_pair(subRoutine->getName(), subRoutine)).second)
                    {
                        reportError(Strings::format("Duplicate subroutine found: '%hs'", subRoutine->getName().c_str()));
                    }
                }
            }
        }
        else if (VariableDeclaration* varDecl = iter->cast_ptr<VariableDeclaration>())
        {
            if (varDecl->hasNonEmptyInitExpression())
            {
                if (varDecl->getDeclarators().isStatic())
                {
                    if (!localVars.insert(make_pair(varDecl->getName(), varDecl)).second)
                    {
                        reportError(Strings::format("Duplicate variable found: '%hs'", varDecl->getName().c_str()));
                    }
                }
                else // is extern
                {
                    if (!externalVars.insert(make_pair(varDecl->getName(), varDecl)).second)
                    {
                        reportError(Strings::format("Duplicate variable found: '%hs'", varDecl->getName().c_str()));
                    }
                }
            }
        }
	}
    for (Declarations::Iterator iter = unit.getGlobals().getFirst(); iter.isValid(); ++iter)
	{
        if (SubroutineDeclaration* subRoutine = iter->cast_ptr<SubroutineDeclaration>())
        {
            const bool isStatic = subRoutine->getDeclarators().isStatic();
            const bool isInline = subRoutine->getDeclarators().isInline();
            if (!subRoutine->hasDefinition())
            {
                if (isStatic || isInline)
                {
                    TSubroutinesMap::iterator found = localSubs.find(subRoutine->getName());
                    if (found != localSubs.end())
                    {
                        subRoutine->setDefinition(*found->second);
                    }
                    else
                    {
                        // This is not an error. User could declare static function, but never use it.
                        //reportError(Strings::format("Static subroutine definition not found: '%hs'", subRoutine.getName().c_str()));
                    }
                }
            }
        }
        else if (VariableDeclaration* varDecl = iter->cast_ptr<VariableDeclaration>())
        {
            if (!varDecl->hasDefinition() && varDecl->getDeclarators().isStatic())
            {
                TVariablesMap::iterator found = localVars.find(varDecl->getName());
                if (found != localVars.end())
                {
                    varDecl->setDefinition(*found->second);
                }
                else
                {
                    varDecl->setDefinition(*varDecl);
                    localVars.insert(make_pair(varDecl->getName(), varDecl));
                }
            }
        }
	}
}

void Resolver::setupDefinitionsUnit(TranslationUnit &unit, TSubroutinesMap &externalSubs, TVariablesMap &externalVars)
{
    for (Declarations::Iterator iter = unit.getGlobals().getFirst(); iter.isValid(); ++iter)
	{
        if (SubroutineDeclaration* subRoutine = iter->cast_ptr<SubroutineDeclaration>())
        {
            const bool isStatic = subRoutine->getDeclarators().isStatic();
            const bool isInline = subRoutine->getDeclarators().isInline();
            if (!isStatic && !isInline && !subRoutine->hasDefinition())
            {
                TSubroutinesMap::iterator found = externalSubs.find(subRoutine->getName());
                if (found != externalSubs.end())
                {
                    subRoutine->setDefinition(*found->second);
                }
                else
                {
                    // This is not an error. There are always be external declarations to standard libraries, etc.
    //				reportError(Strings::format("Static subroutine definition not found: '%hs'", subRoutine.getName().c_str()));
                }
            }
        }
        else if (VariableDeclaration* varDecl = iter->cast_ptr<VariableDeclaration>())
        {
            if (!varDecl->hasDefinition() && !varDecl->getDeclarators().isStatic())
            {
                TVariablesMap::iterator found = externalVars.find(varDecl->getName());
                if (found != externalVars.end())
                {
                    varDecl->setDefinition(*found->second);
                }
                else
                {
                    varDecl->setDefinition(*varDecl);
                    externalVars.insert(make_pair(varDecl->getName(), varDecl));
                }
            }
        }
	}
}

void Resolver::reportError(const std::string& error)
{
	m_errorList.push_back(error);
}


//  Functions implementation

//  Exit namespace
}
}
