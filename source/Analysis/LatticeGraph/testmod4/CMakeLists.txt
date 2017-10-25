project(QuastParser_testmod)

ops_project(
    QuastParser_testmod APP
  SOURCES
    main.cpp
    generated/SimpleLexer.cpp
    generated/SimpleParser.cpp
    generated/SimpleLexer.h
    generated/SimpleParser.h
  LIBRARIES
    Reprise
    Frontend
    Shared
    Backends
    Transforms
  TEMPLATES
    ANTLR
  )
