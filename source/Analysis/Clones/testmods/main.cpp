//

#include "Analysis/CalculationGraph/CalculationGraph.h"
#include "Analysis/Clones/CloneFinder.h"
#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include"Analysis/Clones/SeqHashDeepWalker.h"

#include <iostream>
#include <map>
#include <set>

using namespace OPS;
using namespace OPS::Reprise;
using namespace OPS::Reprise::Canto;
using namespace OPS::Frontend;
using namespace CalculationGraphSpace;
using namespace std;

int main()
{
	OPS::Frontend::Frontend frontend;
	const OPS::Reprise::CompileResult& result = frontend.compileSingleFile("input//Program.c");
	if (result.errorCount() > 0) { std::cout << result.errorText(); std::cout.flush(); }

	TranslationUnit& unit = frontend.getProgramUnit().getUnit(0);
	SeqHashDeepWalker shdw;
	Service::DeepWalker dw;
	shdw.visit(unit);
	auto buckets = shdw.getBuckets(MassThreshold);

	for (auto &bucket : buckets)
	{
		for (int i = 0; i < bucket.second.size(); i++)
		{
			for (int j = i + 1; j < bucket.second.size(); j++)
			{
				if (isSimilar(bucket.second[i], bucket.second[j]))
				{
					cout << "Seems like clone found" << endl;
					addClonePair(bucket.second[i], bucket.second[j]);
				}
			}
		}
	}

	system("pause");
	return 0;
}
