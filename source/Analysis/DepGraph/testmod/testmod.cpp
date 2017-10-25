//// TestMod.cpp : Defines the entry point for the console application.
////
//
//#include <iostream>
//#include "../src/DepGraph.h"
//#include "../src/id.h"									// ====================исправить=================
//#include "Frontend/Frontend.h"
//
//int main()
//{
//	using namespace DepGraph;
//	using namespace DepGraph::Id;
//	OPS::Frontend::Frontend frontend;
//	const OPS::Frontend::CompileResult& result = frontend.compileSingleFile("test\\test1.с");
//	//frontend.getProgramUnit().getUnit(0).getGlobals().getSubroutine(1).getBodyBlock();
//	id  my_id( &(frontend.getProgramUnit().getUnit(0).getGlobals().getSubroutine(1).getBodyBlock()) );
//
//	LamportGraph lamp;
//	lamp.Build(my_id/*,LMP_CONSIDER_INPUTDEP*/);
//	std::cout<<"Lamport Graph:\n"<<lamp;
//	char b;
//	std::cin>>b;
//	return 0;
//}

#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
//#include "Shared/LinearExpressions.h"
#include "Analysis/DepGraph/DepGraph.h"
#include "Analysis/DepGraph/id.h"	

#include <conio.h>
#include <string>
#include <iostream>

using namespace OPS::Reprise;
using namespace OPS::Reprise::Canto;
using namespace OPS::Frontend;
//using namespace OPS::Shared;
using namespace DepGraph;
using namespace DepGraph::Id;
using namespace std;

int main()
{
	Frontend frontend;
	const CompileResult& result = frontend.compileSingleFile("test\\test1.c");

	id  my_id( &(frontend.getProgramUnit().getUnit(0).getGlobals().getSubroutine(0).getBodyBlock()) );
	
		LamportGraph lamp;
		lamp.Build(my_id,LMP_CONSIDER_INPUTDEP);
		std::cout<<"Lamport Graph:\n"<<lamp;
		char b;
		std::cin>>b;
		return 0;

}
