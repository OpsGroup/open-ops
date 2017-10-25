project(LatGraph_testmod)

ops_project(
    LatGraph_testmod APP
  SOURCES
    main.cpp
  LIBRARIES
    Reprise
    Frontend
    Shared
    Backends
    Transforms
  )
