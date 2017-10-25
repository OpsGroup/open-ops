project(IfSplitting_test)

ops_project(
    IfSplitting_test APP
  SOURCES
    main.cpp
  LIBRARIES
    Reprise
    Frontend
    Shared
    Backends
    Transforms
  )
