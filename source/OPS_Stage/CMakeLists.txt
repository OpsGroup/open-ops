project(OPS_Stage)

ops_filesin(INTERFACE "../../include/OPS_Stage/"
    Config.h
    GenericWriter.h
    OPS_Stage.h
    Passes.h
    StdStreamWriter.h
    Utils.h
    WorkContext.h
  )

ops_project(
    OPS_Stage LIB
  SOURCES
    ${INTERFACE}
    Config.cpp 
    Passes.cpp 
    StdStreamWriter.cpp 
    Utils.cpp 
    WorkContext.cpp
  LIBRARIES
    OPS_Core
    Reprise
  PUBLIC
)

add_subdirectory(testmod)
