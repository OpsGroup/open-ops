project(FragmentToSubroutine_test)

ops_project(
    FragmentToSubroutine_test APP
  SOURCES
    main.cpp
  LIBRARIES
    Reprise
    Frontend
    Shared
    Backends
    Transforms
  TEMPLATES
    QtGui
    QtWidgets
  )
