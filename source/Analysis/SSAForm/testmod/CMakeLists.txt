project(SSAForm_test)

ops_project(
    SSAForm_test APP
  SOURCES
    main.cpp
  LIBRARIES
    Reprise
    Frontend
    Analysis
    Shared
    Backends
  )
