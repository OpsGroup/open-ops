project(Reprise)

ops_filesin(INTERFACE "../../include/Reprise/"
	Collections.h
	Common.h
        Canto.h
	Declarations.h
	Exceptions.h
	Expressions.h
	Layouts.h
	Lifetime.h
	ParserResult.h
	ProgramFragment.h
	Reprise.h
	ServiceFunctions.h
	Statements.h
	Types.h
	Units.h
	Utils.h
	Canto/HirCExpressions.h
        Canto/HirCStatements.h
	Canto/HirCTypes.h
	Canto/HirCUtils.h
	Canto/HirFExpressions.h
	Canto/HirFTypes.h
	Service/DeepWalker.h
	Service/Marker.h
	Service/Service.h
        Service/Traversal.h
	Service/WalkerBase.h
  )

ops_project(
    Reprise LIB
  SOURCES
    ${INTERFACE}
	Common.cpp
	Declarations.cpp
	Expressions.cpp
	Layouts.cpp
	Lifetime.cpp
	ProgramFragment.cpp
	ServiceFunctions.cpp
	Statements.cpp
	Types.cpp
	Units.cpp
	Utils.cpp
	
	Canto/HirCExpressions.cpp
	Canto/HirCTypes.cpp
	Canto/HirCUtils.cpp
        Canto/HirCStatements.cpp
	Canto/HirFExpressions.cpp
	Canto/HirFTypes.cpp
	Service/DeepWalker.cpp
	Service/Marker.cpp
        Service/Traversal.cpp
	Service/WalkerBase.cpp
  LIBRARIES
    OPS_Core
  PUBLIC
)

