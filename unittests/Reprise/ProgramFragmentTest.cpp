#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"

#include <string>

#include "GTestIncludeWrapper.h"
#include "../FrontendHelper.h"

using namespace std;
using namespace OPS;
using namespace OPS::Reprise;

//TEST(FactorialTest, HandlesZeroInput) 
//{
//  EXPECT_EQ(1, 1);
//}

TEST(Reprise, ProgramFragment) 
{
    COMPILE_FILE("tests/Reprise/UnitTests/program_fragment.c");

	{
		ProgramFragment emptyFragment;
		EXPECT_TRUE(emptyFragment.isEmpty());
	}

	ASSERT_EQ(1, frontend.getProgramUnit().getUnitCount());
	TranslationUnit& unit = frontend.getProgramUnit().getUnit(0);
	ASSERT_EQ(1, unit.getGlobals().getSubroutineCount());

	Declarations::SubrIterator firstSubr = unit.getGlobals().getFirstSubr();
	if (firstSubr.isValid())
	{
		SubroutineDeclaration& main = *firstSubr;
		ASSERT_TRUE(main.hasImplementation());

		BlockStatement& body = main.getBodyBlock();
		ASSERT_FALSE(body.isEmpty());
		
		{
			ProgramFragment blockFragment = ProgramFragment::fromBlock(body);
			EXPECT_FALSE(blockFragment.isEmpty());
			EXPECT_EQ(&blockFragment.getFirst(), &*body.getFirst());
			EXPECT_EQ(&blockFragment.getLast(), &*body.getLast());
			blockFragment.setFromBlock(body);
			EXPECT_EQ(&blockFragment.getFirst(), &*body.getFirst());
			EXPECT_EQ(&blockFragment.getLast(), &*body.getLast());
		}
		{
			ProgramFragment blockFragment(*body.getFirst(), *body.getLast());
			EXPECT_FALSE(blockFragment.isEmpty());
			EXPECT_EQ(&blockFragment.getFirst(), &*body.getFirst());
			EXPECT_EQ(&blockFragment.getLast(), &*body.getLast());
		}
		{
			ProgramFragment single(*body.getFirst());
			EXPECT_FALSE(single.isEmpty());
			EXPECT_EQ(&single.getFirst(), &*body.getFirst());
			EXPECT_EQ(&single.getLast(), &*body.getFirst());
		}
        {
            ProgramFragment bodyFragment;
            EXPECT_NO_THROW(bodyFragment = ProgramFragment(body));
            EXPECT_NO_THROW(bodyFragment.set(body));
        }
	}

}
