ops_project(
    Shared_test APP
  SOURCES
    main.cpp
  LIBRARIES
    Shared
    Frontend
    Analysis
    Backends
  )