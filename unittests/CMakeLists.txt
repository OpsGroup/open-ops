# CMakeLists.txt for unittests project

project(unittests)

file(GLOB_RECURSE CPP_FILES "./*.cpp")
file(GLOB_RECURSE H_FILES "./*.h")

set(opsSourceRootFilePath ${CMAKE_CURRENT_BINARY_DIR}/opsSourceRoot.cpp)

if(NOT EXISTS ${opsSourceRootFilePath})
  #
  file(
    WRITE
    ${opsSourceRootFilePath}
    "#include <string> \nstd::wstring sourceRoot = L\"${ops_source_root_dir}\";\n")
  #
endif()

list(APPEND CPP_FILES ${opsSourceRootFilePath})

source_group(Generated FILES ${opsSourceRootFilePath})

ops_project(
    unittests APP
  SOURCES
    ${CPP_FILES}
    ${H_FILES}
    ../include/GTestIncludeWrapper.h
    FrontendHelper.h
  LIBRARIES
    ClangRepriseClang
    OPSCoreSystem
  TEMPLATES
    GTest
  )

add_test(
  NAME
    UnitTests
  COMMAND
    unittests ${ops_source_root_dir}
  )

# End of File
