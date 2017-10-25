project(SubstitutionForward_test)

ops_project(
    SubstitutionForward_test APP
  SOURCES
    main.cpp
  LIBRARIES
    Reprise
    Frontend
    Shared
    Backends
    Transforms
  )
