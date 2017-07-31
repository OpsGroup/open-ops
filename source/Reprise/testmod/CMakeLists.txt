ops_project(
    Reprise_test APP
  SOURCES
    c2r.cpp
	FullUnrollFor.cpp
	main.cpp
	StatementsList.cpp
	TestCollections.cpp
	TrackPtrTest.cpp
	c2r.h
	NodeCollector.h
	Tests.h
  LIBRARIES
    Reprise
    Frontend
    Backends
  )