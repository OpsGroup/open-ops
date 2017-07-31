/*
	FrontTransforms/Resolver.h - FrontTransforms module, resolver header

*/

//  Multiple include guard start
#ifndef OPS_FRONT_TRANSFORMS_RESOLVER_H__
#define OPS_FRONT_TRANSFORMS_RESOLVER_H__

//  Standard includes

//  OPS includes
#include "Reprise/Reprise.h"

//  Local includes

//  Global defines and macros

//  Enter namespace
namespace OPS
{
namespace Transforms
{

//  Constants and enums

//  Global classes
///		Resolver class
class Resolver
{
public:
	Resolver(void);

	void setProgram(Reprise::ProgramUnit& program);

	int getErrorCount(void) const;
	std::string getError(int index) const;

	void resolve();


private:
//		Subroutines map
	typedef std::map<std::string, Reprise::SubroutineDeclaration*> TSubroutinesMap;
    typedef std::map<std::string, Reprise::VariableDeclaration*> TVariablesMap;
	typedef std::vector<std::string> TErrorList;

    void resolveUnit(Reprise::TranslationUnit& unit, TSubroutinesMap& externalSubs, TVariablesMap& externalVars);

    void setupDefinitionsUnit(Reprise::TranslationUnit& unit, TSubroutinesMap& externalSubs, TVariablesMap& externalVars);

	void reportError(const std::string&);

	Reprise::ProgramUnit* m_program;
	TErrorList m_errorList;
};

//  Global functions

//  Exit namespace
}
}

//  Multiple include guard end
#endif 						//	OPS_FRONT_TRANSFORMS_RESOLVER_H__
