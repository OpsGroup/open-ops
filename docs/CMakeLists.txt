if(UNIX)
  find_package(Doxygen)
elseif(WIN32)
  set(DOXYGEN_FOUND 1)
  set(DOXYGEN_EXECUTABLE ${CMAKE_BINARY_DIR}/depends/deps/win32/bin/doxygen.exe)
endif()

if(DOXYGEN_FOUND)

  # Add alldoc target for building dumb documentation for all project

  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.all.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.all" @ONLY)

  add_custom_target(alldoc 
    COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.all")

  # Add doc target for building smart documentation

  configure_file("${ops_source_root_dir}/source/Reprise/Doxyfile.in"
                 "${binary_root_dir}/source/Reprise/Doxyfile" @ONLY)

  add_custom_target(doc 
    COMMAND ${DOXYGEN_EXECUTABLE} "${binary_root_dir}/source/Reprise/Doxyfile"
    COMMENT "Generating documentation with with Doxygen" VERBATIM)

else(DOXYGEN_FOUND)
    
  message(STATUS "WARNING: Doxygen not found - Documentation will not be created")

endif(DOXYGEN_FOUND)
