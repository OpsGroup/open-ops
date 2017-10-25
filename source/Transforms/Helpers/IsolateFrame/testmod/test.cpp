#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"
#include "Backends/OutToC/OutToC.h"
#include "OPS_Core/Exceptions.h"

#include "FrontTransforms/CantoToReprise.h"
#include "Transforms/Helpers/IsolateFrame/IsolateFrame.h"

#include "Shared/SubroutinesShared.h"
#include "Transforms/ITransformation.h"
#include "Transforms/TransformArgs.h"
#include "Transforms/TransformsHub.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>


using namespace OPS::Shared;
using namespace OPS::Transforms;
using namespace OPS::Transforms::Helpers;
using namespace OPS::TransformationsHub;
using namespace OPS::Reprise;
using namespace OPS::Frontend;
using namespace OPS::Backends;
using namespace std;



void testIsolateFrame(string path)
{


	try
	{
		Frontend frontend;
		frontend.clangSettings().m_useStandardIncludes = true;

		const CompileResult& result = frontend.compileSingleFile(path);

		if (result.errorCount() == 0)
        {
            ReplaceParams rep;

            BlockStatement& mainBodyBlock = frontend.getProgramUnit().getUnit(0).getGlobals().getLastSubr()->getBodyBlock();
            Declarations& mdecl = mainBodyBlock.getDeclarations();
            DeclIterator<VariableDeclaration>  i=mdecl.getFirstVar();

            for(; i!=mdecl.getLastVar() ; ++i)
            {
                if (i->getType().is_a<ArrayType>())
                {
                    rep[&*i] = 10;
                }
                else
                {
                    rep[&*i] = 1;
                }
            }
            if (i->getType().is_a<ArrayType>())
            {
                rep[&*i] = 10;
            }
            else
            {
                rep[&*i] = 1;
            }


            StatementBase* for_copy = (mainBodyBlock.getFirst())->clone();
            StatementBase* for_copy2 = (mainBodyBlock.getLast())->clone();
            RepriseList<StatementBase> cpl;
            cpl.add(for_copy);
            cpl.add(for_copy2);


            ReprisePtr<ProgramUnit> pu=isolateFrame(*for_copy, rep);
            OutToC output(cout);
            output.visit(pu->getUnit(0));

            std::ostringstream outc;
            OPS::Backends::OutToC outputc(outc);
            outputc.visit(pu->getUnit(0));

            frontend.clearSources();
            frontend.addSourceBuffer(outc.str());
            if(frontend.compile())
                cout<<"cool";
            cout<<frontend.getResult(0).errorText();


        }

		else
		{
			cout << result.errorText();
		}
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

    string path = "/home/vsh/Src/hlc/source/Transforms/Helpers/IsolateFrame/testmod/Samples/sample01.c";
    testIsolateFrame(path);

	return 0;
}
