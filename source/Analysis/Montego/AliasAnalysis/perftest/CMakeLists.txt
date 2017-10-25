project(MontegoAliases_perftest)

ops_project(
    MontegoAliases_perftest APP
  SOURCES
    main.cpp
  LIBRARIES
    Reprise
    Analysis
    Frontend
    Shared
    Backends
    Transforms
  )
