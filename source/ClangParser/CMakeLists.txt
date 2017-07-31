# CMakeLists.txt for ClangParser project

project(ClangParser)

ops_filesin(
  INTERFACE
  "../../include/ClangParser/"
  clangParser.h
  clangParserSettings.h
  ClangGenerator.h
  PragmaHandler.h
  )

ops_project(
    #
    ClangParser LIB
    #
  SOURCES
    #
    ${INTERFACE}
    AST2RepriseConsumers.h
    RepriseDiagnosticClient.h
    ClangParserPlugin.h
    ClangParserPlugin.cpp
    AST2RepriseConsumers.cpp
    clangParser.cpp
    clangParserRepriseFacade.cpp
    clangParserSettings.cpp
    RepriseDiagnosticClient.cpp
    ClangGenerator.cpp
    PragmaHandler.cpp
    #
  LIBRARIES
    #
    OPS_Core
    OPS_Stage
    Reprise
    #
  PUBLIC
  )


# End of File
