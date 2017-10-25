project(Profiler_test)

ops_project(
    Profiler_test APP
  SOURCES
    testmod/Profiler_test.cpp
  LIBRARIES
    Reprise
    Frontend
    Analysis
    Transforms
  )
