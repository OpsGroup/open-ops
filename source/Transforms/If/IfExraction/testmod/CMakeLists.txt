project(IfExtraction_test)

ops_project(
    IfExtraction_test APP
  SOURCES
    main.cpp
  LIBRARIES
    Reprise
    Frontend
    Shared
    Backends
    Transforms
  )
