project(LoopCycleOffset_test)

ops_project(
    LoopCycleOffset_test APP
  SOURCES
    main.cpp
  LIBRARIES
    Reprise
    Frontend
    Shared
    Backends
    Transforms
  )
