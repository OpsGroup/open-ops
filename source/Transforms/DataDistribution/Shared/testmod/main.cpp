#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Backends/OutToC/OutToC.h"
#include "Backends/OutToFortran/OutToF.h"
#include "OPS_Core/Exceptions.h"
#include "OPSCoreSystem/OPSCoreSystem.h"

#include "FrontTransforms/CantoToReprise.h"
#include "Transforms/DataDistribution/Shared/DataDistributionForSharedMemory.h"

#include "Shared/SubroutinesShared.h"
#include "Transforms/ITransformation.h"
#include "Transforms/TransformArgs.h"
#include "Transforms/TransformsHub.h"

#include <iostream>
#include <fstream>
#include <cmath>


using namespace OPS::Shared;
using namespace OPS::Transforms;
using namespace OPS::Transforms::DataDistribution;
using namespace OPS::TransformationsHub;
using namespace OPS::Reprise;
using namespace OPS::Frontend;
using namespace std;

void testDataDistributionForSharedMemory(string path, string resutPath, bool isC)
{
	cout<<"Test for DataDistributionForSharedMemory started..."<<endl;

	try
	{
		Frontend frontend;
        frontend.clangSettings().initWithStdSettingsForCurrentPlatform();
		const CompileResult& result = frontend.compileSingleFile(path);

		if (result.errorCount() == 0)
		{
			ofstream fOut(resutPath.c_str());
            if (fOut.is_open() == false)
            {
                cout << "Couldn't create output file!\n";
                return;
            }
			
			OPS::Transforms::DataDistribution::DataDistributionForSharedMemory transform;
            transform.makeTransform(&frontend.getProgramUnit());

            OPS::Backends::OutToC writer(fOut);
            frontend.getProgramUnit().getUnit(0).accept(writer);
		}
        else
            cout << "error: " << result.errorText() << endl;
	}
	catch(RepriseError e)
	{
		cout<<"OPS Error! " + e.getMessage() + "\n";
	}
	catch(exception e)
	{
		cout<<"Some problems:\n\t"<<e.what();
	}

	system("pause");
}
int main()
{
    OPS::OPSCoreSystem opscore;

    string dirWithExamples = "/home/misha/Desktop/WebOpsExamples/";
    string example = "test9";
    string path = dirWithExamples + example + ".c";
    string resultPath = dirWithExamples + example + "_result.c";
	testDataDistributionForSharedMemory(path, resultPath, true);

	return 0;
}
