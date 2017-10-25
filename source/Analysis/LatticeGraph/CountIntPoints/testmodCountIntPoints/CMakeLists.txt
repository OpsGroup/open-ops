project(CountIntPoints_testmod)

ops_project(
    CountIntPoints_testmod APP
  SOURCES
    main.cpp
  LIBRARIES
    Reprise
    Frontend
    Shared
    Backends
    Transforms
  )
