#ifndef _OPS_REPRISE_VIRTUALREGISTRY_H_INCLUDED_
#define _OPS_REPRISE_VIRTUALREGISTRY_H_INCLUDED_

#include "Reprise/Reprise.h"



namespace OPS 
{
namespace Analysis 
{
namespace SSAForms
{

	using namespace OPS::Reprise;

//orders variables by name, used for std::set and std::map
struct VarNameLess: public std::binary_function< const OPS::Reprise::VariableDeclaration*, const OPS::Reprise::VariableDeclaration*, bool >
{ 
	bool operator()(const OPS::Reprise::VariableDeclaration* p1, const OPS::Reprise::VariableDeclaration* p2) const;
};



//class for assigning virtual registry to variables of predictable behavior
//has (will have, in fact :) several levels of analysis depth and exactness
//level 1:				every external (for the initializing block) variable is considered unpredictable
//						every internal variable that was dereferenced is considered unpredictable
//						others are predictable and are laid on virtual registry
//level 2 and so on:    not implemented yet
class VirtualRegistry : public OPS::NonCopyableMix, public OPS::Reprise::IntrusivePointerBase
{
public:
	typedef std::set<const OPS::Reprise::VariableDeclaration*, VarNameLess> VarSet;

public: 
	
	//get virtual registry set
	const VarSet& getRegistredVars() const;
	//is variable laid on virtual registry, e.g. is variable predictable
	bool isRegistred(const ExpressionBase& eb) const;
	bool isRegistred(const OPS::Reprise::VariableDeclaration& vd) const;

	//lay variable on v.r. 
	//return false if already laid
	bool registerVar(const OPS::Reprise::VariableDeclaration& variable);
	//returns false if variable got laid through registerVar function
	bool isOriginallyRegistred(const OPS::Reprise::VariableDeclaration& eb) const;


	VirtualRegistry();
	//fill virtual registry for block
	void fill(const BlockStatement& block);
	//get initializing block
	const BlockStatement& getBlock() const;

	~VirtualRegistry();





private:
	const BlockStatement* m_block;
	VarSet m_registred;
	VarSet m_originallyRegistered;


	void layDeclarations(const Declarations& decs);
	void layDeclaration(const OPS::Reprise::VariableDeclaration& variable);
	void excludeDeclaration(const OPS::Reprise::VariableDeclaration& vd);
	void excludeDeclaration(const ExpressionBase& eb);

	friend class VirtualRegistryHelper;


};

} //namespace OPS
} //namespace Reprise
} //namespace SSAForm
#endif
