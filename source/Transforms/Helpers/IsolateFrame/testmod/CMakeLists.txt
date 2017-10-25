project(IsolateFrameTest)

ops_project(
    IsolateFrameTest APP
  SOURCES
    test.cpp
  LIBRARIES
    Reprise
    Frontend
    Shared
    Backends
    Transforms
  )
