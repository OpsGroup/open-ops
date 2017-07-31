#ifndef OPS_CORE_ENTRYPOINT_H__
#define OPS_CORE_ENTRYPOINT_H__

#include "OPS_Core/Mixins.h"
#include "OPS_Core/Exceptions.h"
#include <memory>
#include <vector>

namespace OPS
{
namespace Core
{

//      Process function
typedef void (*process_function_type)(void* parameter);
//      Command-line arguments
typedef std::vector<std::string> CommandLineArgumentsType;
	
//		Entry point data
struct EntrypointData
{
//  Members
//      Argument count
	int argc;
//      Unicode arguments or 0
	wchar_t** wargv;
//      MBCS arguments or 0
	char** argv;
//		Result
	int result;

//      Constructors/destructor
	inline EntrypointData(int ep_argc, wchar_t* ep_wargv[], char* ep_argv[]) :
		argc(ep_argc), wargv(ep_wargv), argv(ep_argv), result(0)
	{
	}
};

class Application : public OPS::NonCopyableMix
{
public:
//  Constructors/destructor
	Application(void);

//  Methods
	void process(void);

	void end_process(void);

//  Static methods
	static bool is_console_only(void);

	static const char* get_name(void);

	static const wchar_t* get_title(void);

	static const char* get_default_language(void);

	static bool handle_error(const wchar_t* text, const wchar_t* header);

private:
//  Members
//      Optional private data
	void* m_private_data;
};

//	Global functions
bool process_function(process_function_type function, void* parameter, 
	std::unique_ptr<OPS::Exception>* result_exception = 0, bool report_result_exception = true);

const CommandLineArgumentsType& getCommandLineArguments(void);


}
}

#endif 						//	OPS_CORE_ENTRYPOINT_H__
