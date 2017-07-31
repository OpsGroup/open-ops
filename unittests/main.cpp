#include "OPS_Core/IO.h"
#include "OPS_Core/Strings.h"
#include "OPS_Core/Kernel.h"
#include "OPSCoreSystem/OPSCoreSystem.h"
#include "FrontendHelper.h"

#include "GTestIncludeWrapper.h"

#include <iostream>

int main(int argc, char **argv) 
{
	OPS::OPSCoreSystem opscore;
	
	::testing::InitGoogleTest(&argc, argv);

	const int run_result = RUN_ALL_TESTS();
#if OPS_PLATFORM_IS_WIN32
	system("pause");
#endif
	return run_result;
}
