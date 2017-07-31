if (NOT OPS_BUILD_DVOR)
    return()
endif()

set(CPACK_COMPONENTS_ALL Programs SharedLibs Samples)
SET(CPACK_INSTALL_CMAKE_PROJECTS  "${CMAKE_BINARY_DIR};${CMAKE_PROJECT_NAME};ALL;/" )
#SET(CPACK_PACKAGE_FILE_NAME "OPS")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "OPS")

if(UNIX)
  if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    SET(CPACK_SYSTEM_NAME "Linux32")
  else()
    SET(CPACK_SYSTEM_NAME "Linux64")
  endif()
endif()

if(WIN32)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION bin COMPONENT Programs)
  include (InstallRequiredSystemLibraries)
endif()

if(UNIX)
  SET(CPACK_GENERATOR "TGZ") #STGZ
else()
  SET(CPACK_GENERATOR "NSIS")
endif()

SET(CMAKE_INSTALL_RPATH ".") 

find_package(Qt5Core REQUIRED)
get_target_property(QtCore_location_Release Qt5::Core LOCATION_Release)
get_filename_component(QT_LIBRARIES ${QtCore_location_Release} PATH)
get_filename_component(QT_DIR ${QT_LIBRARIES} PATH)
set(QT_PLUG ${QT_DIR}/plugins)

SET(BundleUtilities_DIRS "${QT_LIBRARIES};${OPS_LLVM_DIR}/lib;${OPS_ANTLR_DIR}/lib")

if(UNIX)
  SET(DVOR_EXECUTABLE "bin/dvor")
else()
  SET(DVOR_EXECUTABLE "bin/dvor.exe")
endif()

INSTALL(CODE "
    #fixing library dependencies
    include(BundleUtilities)
    fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${DVOR_EXECUTABLE}\"   \"\"   \"${BundleUtilities_DIRS};\${CMAKE_INSTALL_PREFIX}/lib\")
    
    if(UNIX)
    #bundle utilities copy libs to the folder of executable
    #moving libs from bin to lib
    file(GLOB TO_MOVE \"\${CMAKE_INSTALL_PREFIX}/bin/*.so*\")
    file(COPY \${TO_MOVE} DESTINATION \${CMAKE_INSTALL_PREFIX}/lib)
    file(REMOVE \${TO_MOVE})
    endif(UNIX)
    "
	COMPONENT Programs)

if(UNIX)
  INSTALL(PROGRAMS ${CMAKE_SOURCE_DIR}/install/ihop DESTINATION .)
  INSTALL(PROGRAMS ${CMAKE_SOURCE_DIR}/install/opsc DESTINATION .)
  INSTALL(PROGRAMS ${CMAKE_SOURCE_DIR}/install/opsdemo DESTINATION .)
else()
  SET(CPACK_PACKAGE_EXECUTABLES "dvor;Interactive High-level Optimizing Parallelizer" "OpsDemo5;OPS Demo Application")
endif()

#qt-applications require some libraries, not determined by ldd from elf
#we need to copy them manually (see "Deploying QT-applications" on Nokia site)

#plugins must obey directory structure
if(UNIX)
  INSTALL(PROGRAMS "${QT_PLUG}/platforms/libqxcb.so" DESTINATION lib/plugins/platforms COMPONENT Programs)
elseif(WIN32)
  INSTALL(PROGRAMS "${QT_PLUG}/platforms/qwindows.dll" DESTINATION bin/platforms COMPONENT Programs)
  INSTALL(PROGRAMS "${QT_DIR}/bin/libEGL.dll" DESTINATION bin COMPONENT Programs)
endif()

#libs, which are symbolic links
if(UNIX)
  set(additional_libs ${QT_DIR}/lib/libQt5DBus.so.5)
  foreach(alib ${additional_libs})
    if(EXISTS ${alib})
      get_filename_component(resolved_alib "${alib}" REALPATH)
      get_filename_component(fileName ${alib} NAME)
      INSTALL(CODE "
      configure_file(${resolved_alib} \${CMAKE_INSTALL_PREFIX}/lib/${fileName} COPYONLY)
      ")
    endif()
  endforeach()
endif()

include(CPack)
