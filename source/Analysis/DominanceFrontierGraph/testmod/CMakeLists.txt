project(DominanceFrontierGraph_test)

ops_project(
    DominanceFrontierGraph_test APP
  SOURCES
    main.cpp
  LIBRARIES
    Reprise
    Transforms
    Frontend
    Analysis
    Shared
    Backends
  )
