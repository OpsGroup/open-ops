#ifndef _COMPOSITION_CHECK_OBJECTS_H_
#define _COMPOSITION_CHECK_OBJECTS_H_

#include <set>

namespace OPS
{
namespace Shared
{
namespace Checks
{

class CompositionCheckObjects
{
public:
	enum CompositionCheckObjectTypes
	{
		// Statements
		CCOT_BlockStatement = 0,
		CCOT_BreakStatement,
		CCOT_ContinueStatement,
		CCOT_EmptyStatement,
		CCOT_ExpressionStatement,
		CCOT_ForStatement,
		CCOT_GotoStatement,
		CCOT_IfStatement,
		CCOT_ReturnStatement,
		CCOT_SwitchStatement,
		CCOT_WhileStatement,

		// Other
		CCOT_Label,

		CCOT_ObjectsCount
	};

	CompositionCheckObjects();

	CompositionCheckObjects& operator << (CompositionCheckObjectTypes object);
	
	bool contains(CompositionCheckObjectTypes object) const;

private:
	std::set<CompositionCheckObjectTypes> m_objects;
};

}	// Checks
}	// Shared
}	// OPS

#endif	// _COMPOSITION_CHECK_OBJECTS_H_
