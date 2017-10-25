project(Induction_testmod)

ops_project(
    Induction_testmod APP
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
