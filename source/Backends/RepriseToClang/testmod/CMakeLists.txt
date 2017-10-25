# CMakeLists.txt for the R2Clang project

#
# Create an OPS subproject
#
ops_project(
  R2Clang_test APP
  SOURCES
    R2Clang_main.cpp
  LIBRARIES
    Analysis
    Backends
    Frontend
    Reprise
    Shared
    #Transforms
  DEPENDS
    ClangParser
    OPS_Core
  )

# End of File
