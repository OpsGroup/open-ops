#include "OPS_Core/EntryPoint.h"
#include "OPS_Core/Helpers.h"

#include <iostream>

namespace OPS
{
namespace Core
{

OPS_DEFINE_EXCEPTION_CLASS(StdException, OPS::Exception)


//      Command-line arguments (THREADED: single-point/read-only | attributes)
static CommandLineArgumentsType v_arguments;
//
static Application* v_application;

//		Global functions
static void wrap_exceptions(process_function_type function, void* parameter);
//      Error handler
static void handle_error(const wchar_t* text, const wchar_t* header);
//		Fatal error
static void fatal_error(const wchar_t* text) throw();
//		Report exception
static void report_exception(const Exception& error);
//		Process start
static bool process_start(const EntrypointData& entrypoint_data);
//		Process finish
static bool process_finish(void);
//      Application creator
static void create_application(void* parameter);
//      Application destroyer
static void destroy_application(void* parameter);




bool process_function(process_function_type const function, void* const parameter, 
					  std::unique_ptr<Exception>* const result_exception, const bool report_result_exception)
{
	try
	{
		std::unique_ptr<Exception> current_exception;
		try
		{
			wrap_exceptions(function, parameter);
			return true;
		}
		catch (const Exception& exception)
		{
			current_exception.reset(exception.clone());
		}
		catch (const Exception* const exception)
		{
			current_exception.reset(exception->clone());
		}
		catch (const std::exception& exception)
		{
			current_exception.reset(new StdException(exception.what()));
		}
		catch (const std::exception* const exception)
		{
			current_exception.reset(new StdException(exception->what()));
		}
		catch (const std::wstring& text)
		{
			current_exception.reset(new Exception(Strings::narrow(text)));
		}
		catch (const std::wstring* const text)
		{
			current_exception.reset(new Exception(Strings::narrow(*text)));
		}
		catch (const std::string& text)
		{
			current_exception.reset(new Exception(text));
		}
		catch (const std::string* const text)
		{
			current_exception.reset(new Exception(*text));
		}
		catch (const wchar_t* const text)
		{
			current_exception.reset(new Exception(Strings::narrow(text)));
		}
		catch (const char* const text)
		{
			current_exception.reset(new Exception(text));
		}
		catch (const byte& value)
		{
			current_exception.reset(new Exception(Strings::format("Unsigned byte value '%u'.", 
				static_cast<unsigned>(value))));
		}
		catch (const sbyte& value)
		{
			current_exception.reset(new Exception(Strings::format("Signed byte value '%i'.", 
				static_cast<int>(value))));
		}
		catch (const word& value)
		{
			current_exception.reset(new Exception(Strings::format("Unsigned word value '%hu'.", value)));
		}
#if OPS_NATIVE_WCHAR_T == 1
		catch (const word* const value)
		{
			current_exception.reset(new Exception(Strings::format("Unsigned word pointer value '%hu'.", *value)));
		}
#endif
		catch (const sword& value)
		{
			current_exception.reset(new Exception(Strings::format("Signed word value '%hi'.", value)));
		}
		catch (const sword* const value)
		{
			current_exception.reset(new Exception(Strings::format("Signed word pointer value '%hi'.", *value)));
		}
		catch (const dword& value)
		{
			current_exception.reset(new Exception(Strings::format("Unsigned dword value '%u'.", value)));
		}
		catch (const dword* const value)
		{
			current_exception.reset(new Exception(Strings::format("Unsigned dword pointer value '%u'.", *value)));
		}
		catch (const sdword& value)
		{
			current_exception.reset(new Exception(Strings::format("Signed dword value '%i'.", value)));
		}
		catch (const sdword* const value)
		{
			current_exception.reset(new Exception(Strings::format("Signed dword pointer value '%i'.", *value)));
		}
		catch (const qword& value)
		{
			current_exception.reset(new Exception(Strings::format("Unsigned qword value '%" 
				OPS_CRT_FORMAT_LONG_LONG_PREFIX "u'.", value)));
		}
		catch (const qword* const value)
		{
			current_exception.reset(new Exception(Strings::format("Unsigned qword pointer value '%" 
				OPS_CRT_FORMAT_LONG_LONG_PREFIX "u'.", *value)));
		}
		catch (const sqword& value)
		{
			current_exception.reset(new Exception(Strings::format("Signed qword value '%" 
				OPS_CRT_FORMAT_LONG_LONG_PREFIX "i'.", value)));
		}
		catch (const sqword* const value)
		{
			current_exception.reset(new Exception(Strings::format("Signed qword pointer value '%" 
				OPS_CRT_FORMAT_LONG_LONG_PREFIX "i'.", *value)));
		}
		catch (const float& value)
		{
			current_exception.reset(new Exception(Strings::format("Unsigned float value '%g'.", value)));
		}
		catch (const float* const value)
		{
			current_exception.reset(new Exception(Strings::format("Unsigned float pointer value '%g'.", *value)));
		}
		catch (const double& value)
		{
			current_exception.reset(new Exception(Strings::format("Unsigned double value '%g'.", value)));
		}
		catch (const double* const value)
		{
			current_exception.reset(new Exception(Strings::format("Unsigned double pointer value '%g'.", *value)));
		}
/*
		catch (const Mixins::TypeNameIdentifiable& object)
		{
			current_exception.reset(new Exception(Strings::format(L"Type-identifiable object %ls.", 
				object.get_object_description().c_str())));
		}
		catch (const Mixins::TypeNameIdentifiable* const object)
		{
			current_exception.reset(new Exception(Strings::format(L"Type-identifiable object %ls.", 
				object->get_object_description().c_str())));
		}
*/
		catch (const void* const value)
		{
			current_exception.reset(new Exception(Strings::format("Pointer value '%p'.", value)));
		}
		catch (...)
		{
			current_exception.reset(new Exception("Unknown exception."));
		}
		if (current_exception.get() != 0)
		{
			if (result_exception != 0)
			{
				result_exception->reset(current_exception.release());
				if (report_result_exception)
					report_exception(**result_exception);
			}
			else
			{
//				display_exception(*current_exception);
				const std::wstring text = Strings::widen(current_exception->getMessage());
//				const std::string header = Strings::format(L"Exception caught at thread %p '%hs'", 
//					Threading::get_current_thread(), Threading::get_thread_name(Threading::get_current_thread()).c_str());

				handle_error(text.c_str(), L"");
			}
		}
	}
	catch (...)
	{
		fatal_error(L"Unhandled exception passed through root exception handler"
			L" (may be due to lack of memory or heap corruption).");
	}
	return false;
}

const CommandLineArgumentsType& getCommandLineArguments(void)
{
    return v_arguments;
}

void wrap_exceptions(process_function_type function, void* parameter)
{
	function(parameter);
}

void handle_error(const wchar_t* const text, const wchar_t* const header)
{
	bool result;
	try
	{
		result = Application::handle_error(text, header);
	}
	catch (...)
	{
		result = false;
	}
	if (!result)
	{
		std::cout << Strings::narrow(text) << ": \n" << Strings::narrow(text) << std::endl;
		//Kernel_Platform::display_error(text, header);
	}
}

void fatal_error(const wchar_t* const text) throw()
{
	handle_error(text, L"Fatal error");
#if OPS_PLATFORM_IS_WIN32
	abort();
//	::ExitProcess(static_cast<UINT>(-2));
#elif OPS_PLATFORM_IS_UNIX
	abort();
#else
#error Needs porting (unsupported platform)
#endif
}

void report_exception(const Exception& error)
{
    std::cerr << error.getMessage() << std::endl;
}

bool process_start(const EntrypointData& entrypoint_data)
{
	v_arguments.clear();
	if (entrypoint_data.argc > 1 && (entrypoint_data.wargv != 0 || entrypoint_data.argv != 0))
	{
		try
		{
			v_arguments.reserve(entrypoint_data.argc - 1);
			if (entrypoint_data.wargv != 0)
			{
				for (int argument = 1;argument < entrypoint_data.argc;++argument)
                    v_arguments.push_back(Strings::narrow(entrypoint_data.wargv[argument]));
			}
			else
				if (entrypoint_data.argv != 0)
				{
					for (int argument = 1;argument < entrypoint_data.argc;++argument)
                        v_arguments.push_back(entrypoint_data.argv[argument]);
				}
		}
		catch (...)
		{
			v_arguments.clear();
			throw;
		}
	}

	return process_function(create_application, const_cast<void*>(static_cast<const void*>(&entrypoint_data)));
}

static bool process_finish(void)
{
	return process_function(destroy_application, 0);
}

static void create_application(void* const parameter)
{
	EntrypointData* const entrypoint_data = reinterpret_cast<EntrypointData*>(parameter);
	OPS_ASSERT(entrypoint_data != 0)
//  NOTE: entrypoint data is unused intentionally
	OPS_UNUSED(entrypoint_data)
	v_application = new Application();
	v_application->process();
}

static void destroy_application(void* const parameter)
{
	OPS_UNUSED(parameter)
	if (v_application != 0)
		v_application->end_process();
	delete v_application;
	v_application = 0;
}

}
}

//      main function
int main(const int argc, char* argv[])
{
    const OPS::Core::EntrypointData entrypoint_data(argc, 0, argv);
    const bool start_result = OPS::Core::process_start(entrypoint_data);
    const bool finish_result = OPS::Core::process_finish();
	return (start_result && finish_result) ? 0 : -1;
}

//      wmain function
int wmain(const int argc, wchar_t* wargv[])
{
    const OPS::Core::EntrypointData entrypoint_data(argc, wargv, 0);
    const bool start_result = OPS::Core::process_start(entrypoint_data);
    const bool finish_result = OPS::Core::process_finish();
	return (start_result && finish_result) ? 0 : -1;
}

