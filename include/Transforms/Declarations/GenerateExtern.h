/// Transforms/Declarations/GenerateExtern.h
///   Generate necessary extern declarations if any declarations reference
/// another translation unit.
/// Author: Denis Dubrov (dubrov@sfedu.ru)
/// Created: 9.03.2013

#ifndef GENERATEEXTERN_H__
#define GENERATEEXTERN_H__

namespace OPS
{
	namespace Reprise
	{
		class ProgramUnit;
	}
	//
	namespace Transforms
	{
		namespace Declarations
		{
			void generateExtern(OPS::Reprise::ProgramUnit& rProgramUnit);
			//
		}    // namespace Declarations
	}    // namespace Transforms
}    // namespace OPS

#endif    // GENERATEEXTERN_H__

// End of File
