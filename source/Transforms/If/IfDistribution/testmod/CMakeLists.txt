project(IfDistribution_test)

ops_project(
    IfDistribution_test APP
  SOURCES
    main.cpp
  LIBRARIES
    Reprise
    Frontend
    Shared
    Backends
    Transforms
  )
