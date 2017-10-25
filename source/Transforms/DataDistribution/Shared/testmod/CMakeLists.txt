project(DataDistribution_Shared_testmod)

ops_project(
    DataDistribution_Shared_testmod APP
  SOURCES
    main.cpp
  LIBRARIES
    Reprise
    Frontend
    Shared
    Backends
    Transforms
    OPSCoreSystem
  )
