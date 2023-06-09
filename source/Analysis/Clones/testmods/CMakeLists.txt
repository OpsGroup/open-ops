project(Induction_testmod)

ops_project(
    Clones_testmod APP
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
