// testmod1.cpp : Defines the entry point for the console application.
//
#pragma warning(disable : 4786)	// Запрещаем сообщение об обрубании имен в debug-версии
#pragma warning(disable : 4503)	// Запрещаем сообщение об обрубании имен в debug-версии
#pragma warning(disable : 4355)	// Запрещаем сообщение о this в конструкторе

#define _DEBUG_MSG_PARANOID


#include "Analysis/CalculationGraph/CalculationGraph.h"
#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"

#include <conio.h>
//#include <string>
#include <iostream>

using namespace OPS::Reprise;
using namespace OPS::Reprise::Canto;
using namespace OPS::Frontend;
using namespace CalculationGraphSpace;
using namespace std;

int main()
{
	
	// Input program creation begin

	/*
		double A[10];
	
	void main()
	{
		int i;
		for (i = 0; i < 10; i++)
		{
			A[i] = 10 - A[i-1];
		}
		
		
	}
	
	*/

	
	Frontend frontend;
	const CompileResult& result = frontend.compileSingleFile("input//Program.c");

	if (result.errorCount() != 0)
	{
		std::cout << "Parsed program error!";
		return 1;
	}

	BlockStatement::Iterator FirstInBlock = frontend.getProgramUnit().getUnit(0).getGlobals().getSubroutine(0).getBodyBlock().getFirst();
	ForStatement* Loop;
	StatementBase& Stmt = *FirstInBlock;
	if (Stmt.is_a<ForStatement>())
	{
		Loop = &FirstInBlock ->cast_to<ForStatement>();
	}
	
	CalculationGraph* G = new CalculationGraph(CalculationGraph::GCM_RECONFIGURABLE_PIPILINE); 

	if (G->createGraph(Loop, ""))
	{
		G->executeAllAlgorithms();
		
		int n;
		vector<deque<int>> ll = G->getReachabilityList(n);
		cout << endl << "Graph Reachability List" << endl;
		for(int i=0; i<n; i++)
		{
			deque<int>::iterator itCols = ll[i].begin();
			int j = 0;
			cout << i << ": ";
			while(itCols != ll[i].end())
			{
				cout << *itCols << " ";
				itCols++;
				j++;
			}

			cout << endl;
		}
	}

	
	getch();
	return 0;
}
