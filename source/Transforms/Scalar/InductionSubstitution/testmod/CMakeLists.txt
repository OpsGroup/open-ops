project(InductionSubstitution_testmod)

ops_project(
    InductionSubstitution_testmod APP
  SOURCES
    main.cpp
  LIBRARIES
    Reprise
    Frontend
    Shared
    Backends
    Transforms
  )
