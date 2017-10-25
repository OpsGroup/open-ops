# OPS Backends project description

ops_filesin(
  INTERFACE "../../include/Backends/"
  BackendsHub.h
  OutToC/OutToC.h
  RepriseToClang/ClangGenerator.h
  RepriseToClang/ClangGeneratorTypes.h
  RepriseXml/RepriseXml.h
  RepriseXml/RepriseXmlPass.h
  )

ops_project(
    #
    Backends LIB
    #
  SOURCES
    #
    ${INTERFACE}
    BackendsPlugin.h
    BackendsPlugin.cpp
    BackendsHub.cpp
    #
    OutToC/OutToC.cpp
    #
    RepriseToClang/ClangExpressions.cpp
    RepriseToClang/ClangExpressions.h
    RepriseToClang/ClangGenerator.cpp
    RepriseToClang/ClangGeneratorTypes.cpp
    RepriseToClang/ClangStatements.cpp
    RepriseToClang/ClangStatements.h
    RepriseToClang/ClangTypes.cpp
    RepriseToClang/ClangTypes.h
    RepriseToClang/ClangWalker.cpp
    RepriseToClang/ClangWalker.h
    RepriseToClang/R2CDeclNodes.h
    RepriseToClang/RepriseClangifier.cpp
    RepriseToClang/RepriseClangifier.h
    #
    RepriseXml/RepriseXml.cpp
    RepriseXml/RepriseXmlPass.cpp
    #
    #
  LIBRARIES
    #
    OPS_Core
    Reprise
    Analysis
    Transforms
    #
  PUBLIC
  )

add_subdirectory(RepriseToClang/testmod)


# End of File
