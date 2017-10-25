project(MontegoAliases_testmod)

ops_project(
    MontegoAliases_testmod APP
  SOURCES
    AliasAnalysisTester.cpp
    AliasAnalysisTester.h
    main.cpp
  LIBRARIES
    Reprise
    Analysis
    Frontend
    Shared
    Backends
    Transforms
  )
