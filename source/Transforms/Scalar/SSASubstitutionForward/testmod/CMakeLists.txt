project(SSASubstitutionForward_test)

ops_project(
    SSASubstitutionForward_test APP
  SOURCES
    main.cpp
  LIBRARIES
    Reprise
    Frontend
    Shared
    Backends
    Transforms
  )
