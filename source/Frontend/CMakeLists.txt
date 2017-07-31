ops_project(
    Frontend LIB
  SOURCES
    ../../include/Frontend/Frontend.h
	FrontendPlugin.h
	FrontendPlugin.cpp
	Frontend.cpp
  LIBRARIES
    OPS_Core
    Reprise
    ClangParser
    FrontTransforms
  PUBLIC
  )

#if(OPS_HAVE_ANTLR)
#  target_link_libraries(Frontend F2003Parser)
#endif()
