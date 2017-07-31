
set(opsSourceRootFilePath ${CMAKE_CURRENT_BINARY_DIR}/opsSourceRoot.cpp)

if(NOT EXISTS ${opsSourceRootFilePath})
  #
  file(
    WRITE
    ${opsSourceRootFilePath}
    "#include <string> \nnamespace OPS {std::wstring sourceRoot = L\"${ops_source_root_dir}\";}\n")
  #
endif()

ops_filesin(INTERFACE "../../include/OPS_Core"
	Interfaces/ICoreSettingsService.h
	Core.h
	Plugins/IPlugin.h
	Plugins/PluginManager.h
	Plugins/PluginRegistration.h
	ServiceLocator.h
	Compiler.h
	Console.h
        EntryPoint.h
	Environment.h
	Exceptions.h
	Events.h
	File.h
	Helpers.h
	IniFile.h
	IO.h
	Kernel.h
	Listener.h
	Localization.h
	MemoryHelper.h
	Mixins.h
	msc_leakcheck.h
	OPS_Core.h
	Platform.h
	Serialization.h
	StlHelpers.h
	Strings.h
	Template.h
	Threading.h
	Types.h
	Visitor.h
	XmlBuilder.h
	xmlsp.h
    ExternalCompiler.h
	)

ops_project(
  OPS_Core LIB
  SOURCES
    ${INTERFACE}
    Core.cpp
	CoreSettingsService.h
	CoreSettingsService.cpp
	Plugins/PluginManager.cpp
	Plugins/PluginRegistry.cpp
	Plugins/PluginRegistry.h
	ServiceLocator.cpp
	Console.cpp
	Exceptions.cpp
	File.cpp
	IniFile.cpp
	IO.cpp
	Kernel.cpp
	Listener.cpp
	Mixins.cpp
	Strings.cpp
	Serialization.cpp
	Template.cpp
	Threading.cpp
	Threading_Platform_Unix.cpp
	Threading_Platform_Win.cpp
	XmlBuilder.cpp
	xmlsp.cpp
    ExternalCompiler.cpp
    ${opsSourceRootFilePath}
  PUBLIC
  )
  