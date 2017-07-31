#include "Reprise/Reprise.h"

#include <list>


/*
int main()
{
	using namespace OPS::Reprise;
	using namespace std;


	TrackPtr<StatementBase> block(new BlockStatement());
	TrackPtr<BlockStatement> block2(block);
	TrackPtr<BlockStatement> block3;
	typedef list<TrackPtr<StatementBase> > TStatementsList;
	TStatementsList statements;
	block3 = block2;
	if (block2 != block3)
		throw OPS::StateError("Unexpected.");
	if (block2 != block)
		throw OPS::StateError("Unexpected.");
	block->dumpState();

	statements.push_back(block);
	statements.push_back(block2);
	statements.push_back(block3);
	block2.reset(0);

	
	return 0;
}
*/
