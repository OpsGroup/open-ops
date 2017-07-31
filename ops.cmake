# ops.cmake
# OPS CMake based build system internal stuff

message(
  STATUS
  "Starting OPS build system...")

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 OLD)
endif()

# Checking for out-of-source build

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  #
  message(
    FATAL_ERROR
    "In-source builds are not allowed. Please create "
    "a directory and run cmake from there, passing the "
    "path to this source directory as the last argument. "
    "This process created the file `CMakeCache.txt' and "
    "the directory `CMakeFiles'. Please delete them.")
  #
endif()

# Project options

option(
  OPS_FORCE_UNPACK
  "Force unpack dependencies"
  OFF)

option(
  OPS_FORCE_USE_BOOST
  "Force using Boost instead of std::tr1"
  OFF)

option(
  OPS_BUILD_QT_PROJECTS
  "Build projects that use Qt5"
  ON)

option(
  OPS_BUILD_DVOR
  "Build DVOR application"
  OFF)

option(
  OPS_PARALLEL_BUILD
  "Build multiple files in parallel processes"
  ON)

option(
  OPS_UNITY_BUILD
  "Build project as single source file"
  OFF)

option(
  OPS_NATIVE_WCHAR_T
  "Use wchar_t as a built-in type instead of typedef in MSVC"
  OFF)

set(
  OPS_TESTMODS_TO_BUILD "ALL"
  CACHE STRING
  "Semicolon-separated list of testmods to build or \"ALL\".")

if(MSVC)
  #
  set(
    OPS_LLVM_DIR_DEBUG
    ""
    CACHE PATH
    "A directory containing include and lib subdirs for debug LLVM & Clang")
  #
  set(
    OPS_LLVM_DIR_RELEASE
    ""
    CACHE PATH
    "A directory containing include and lib subdirs for release LLVM & Clang")
  #
  set(
    OPS_ANTLR_DIR_INCLUDE
    ""
    CACHE PATH
    "An include directory for ANTLR")
  #
  set(
    OPS_ANTLR_DIR_LIB_DEBUG
    ""
    CACHE PATH
    "A debug lib directory for ANTLR")
  #
  set(
    OPS_ANTLR_DIR_LIB_RELEASE
    ""
    CACHE PATH
    "A release lib directory for ANTLR")
  #
else()
  #
  set(
    OPS_LLVM_DIR
    ""
    CACHE PATH
    "A directory containing include and lib subdirs for LLVM & Clang")
  #
  set(
    OPS_ANTLR_DIR
    ""
    CACHE PATH
    "A directory containing include and lib subdirs for ANTLR")
  #
endif()

# Setting output directories to bin and lib

if(NOT ops_source_root_dir)
  #
  set(ops_source_root_dir "${CMAKE_CURRENT_SOURCE_DIR}")
  #
endif()

set(binary_root_dir "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${binary_root_dir}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${binary_root_dir}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${binary_root_dir}/lib)


# Enable C++11 support
if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++0x")
endif()
# MSVC does not require any special flags

#
if(CMAKE_VERSION VERSION_GREATER "2.8.7")
  #
  find_package(Qt5Widgets)
  #
else()
  #
  if(OPS_BUILD_QT_PROJECTS)
    #
    message(
      WARNING
      "You are using old cmake version. All Qt5 project will be disabled."
      " Get cmake 2.8.8 or newer to enable Qt5 projects.")
    #
    set(OPS_BUILD_QT_PROJECTS FALSE)
    #
  endif()
  #
endif()
#

# Unpacking dependencies

set(depends_dir ${binary_root_dir}/depends)
# set(generated_dir ${binary_root_dir}/cmake_generated)

if(
  OPS_FORCE_UNPACK OR
  (
    NOT EXISTS ${depends_dir} OR
    NOT IS_DIRECTORY ${depends_dir}
  ) AND
  (
    NOT OPS_LLVM_DIR_DEBUG OR
    NOT OPS_LLVM_DIR_RELEASE OR
    NOT OPS_ANTLR_DIR_INCLUDE OR
    NOT OPS_ANTLR_DIR_LIB_DEBUG OR
    NOT OPS_ANTLR_DIR_LIB_RELEASE
  ) AND
  (
    NOT OPS_LLVM_DIR OR
    NOT OPS_ANTLR_DIR
  )
  )
  #
  find_program(
    unpacker_path 7z
    PATHS
      "$ENV{ProgramFiles}/7-zip"
      "$ENV{ProgramFiles(x86)}/7-zip"
      "$ENV{ProgramW6432}/7-zip"
      "${ops_source_root_dir}/deps/win32/bin"
      "${ops_source_root_dir}/deps/unix/bin"
    )
  #
  if(NOT unpacker_path)
    #
#    message(
#      SEND_ERROR
#      "Could not find the 7z unpacker")
    #
  endif()
  #
  # message("unpacker_path == ${unpacker_path}")
  #
  if(MSVC OR OPS_FORCE_UNPACK)
    #
    foreach(archive deps)
      #
      set(
        archive_path
        ${ops_source_root_dir}/${archive}.7z)
      #
      execute_process(
        COMMAND
          ${unpacker_path} x ${archive_path}
          -o${depends_dir} -y
          RESULT_VARIABLE result
        )
      #
      if(NOT result EQUAL 0)
        #
        message(
          SEND_ERROR
          "Error while unpacking ${archive_path}"
          )
        #
      endif()
      #
    endforeach()
    #
  endif()
  #
endif()

# Determining the Boost/TR1 use

set(use_boost FALSE)

if(OPS_FORCE_USE_BOOST)
  #
  set(use_boost TRUE)
  #
else()
  #
  if(UNIX)
    #
    include(CheckIncludeFileCXX)
    check_include_file_cxx(tr1/memory tr1_exists)
    #
    if(NOT tr1_exists)
      #
      message(
        STATUS
        "TR1 under GCC not found, using Boost instead")
      #
      set(use_boost TRUE)
      #
    endif()
    #
  endif()
  #
endif()

if(use_boost)
  #
  # ToDo
  #
  message(
    FATAL_ERROR
    "Using Boost instead of TR1 is not implemented yet")
  #
endif()

# Helper functions setting the project options

set(
  clang_libraries
  clangFrontendTool
  clangFrontend
  clangDriver
  clangSerialization
  clangCodeGen
  clangParse
  clangSema
  clangStaticAnalyzerFrontend
  clangStaticAnalyzerCheckers
  clangStaticAnalyzerCore
  clangAnalysis
  clangARCMigrate
  clangRewriteCore
  clangRewriteFrontend
  clangEdit
  clangAST
  clangLex
  clangBasic
  )

set(
  common_llvm_components
  asmparser
  asmprinter
  BitReader
  BitWriter
  codegen
  Core
  engine
  Instrumentation
  ipo
  IRReader
  Linker
  MC
  Object
  SelectionDAG
  Support
  Target
  TransformUtils
  Vectorize
  )

if(MSVC)
  #
  set(libs_dir ${depends_dir}/deps/win32/lib)
  set(configs debug release)
  #
elseif(UNIX)
  #
  set(libs_dir /usr/local)
  #
  if(NOT OPS_ANTLR_DIR)
    #
    set(OPS_ANTLR_DIR ${libs_dir})
    #
  endif()
  #
  if(NOT OPS_LLVM_DIR)
    #
    set(OPS_LLVM_DIR ${libs_dir})
    #
  endif()
  #
  set(libs_dir ${OPS_LLVM_DIR}/lib)
  set(configs common)
  #
else()
  #
  message(
    SEND_ERROR
    "The depend libraries are not built for the "
    "selected platform/compiler. Please put the "
    "corresponding versions to the deps.7z and "
    "modify the ops.cmake to indicate their "
    "location.")
  #
endif()

# message("libs_dir == ${libs_dir}")

foreach(config ${configs})
  #
  set(var_name lib_full_paths_${config})
  set(${var_name})
  #
  string(TOUPPER ${config} config_upper)
  set(llvm_path_var_name OPS_LLVM_DIR_${config_upper})
  set(req_llvm_libs_var_name REQ_LLVM_LIBRARIES_${config_upper})
  set(antlr_path_var_name OPS_ANTLR_DIR_LIB_${config_upper})
  #
  # message("Ku: ${llvm_path_var_name} == ${${llvm_path_var_name}}")
  #
  if(MSVC)
    #
    if(NOT "${${llvm_path_var_name}}" STREQUAL "")
      #
      # message(bingo)
      #
      set(search_path ${${llvm_path_var_name}}/lib)
      set(module_path ${${llvm_path_var_name}}/share/llvm/cmake)
      #
    else()
      #
      set(search_path ${libs_dir}/llvm-${config})
      #
      message(
        FATAL_ERROR
        "In-source library paths not supported, "
        "use externally built libraries (LLVM, etc.) instead")
      #
    endif()
    #
  else()
    #
    set(search_path "${libs_dir}")
    set(module_path ${OPS_LLVM_DIR}/share/llvm/cmake)
    #
  endif()
  #
  # message("module_path == ${module_path}")
  #
  # Including LLVM cmake modules
  #
  set(
    OLD_CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH})
  set(
    CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH} ${module_path})
  include(
    LLVMConfig
    OPTIONAL
    RESULT_VARIABLE LLVMCONFIG_INCLUDED)
  if(LLVMCONFIG_INCLUDED)
    #
    include(AddLLVM)
    llvm_map_components_to_libraries(
      ${req_llvm_libs_var_name}
      ${LLVM_TARGETS_TO_BUILD} ${common_llvm_components})
    #
  else()
    #
    message(
      WARNING
      "LLVMConfig.cmake not found in ${module_path}")
    #
  endif()
  #
  set(
    CMAKE_MODULE_PATH
    ${OLD_CMAKE_MODULE_PATH})
  #
  foreach(lib ${clang_libraries} ${${req_llvm_libs_var_name}})
    #
    set(path_var_name LIB_PATH_${lib}_${config})
    #
    find_library(
      ${path_var_name} ${lib}
      PATHS ${search_path})
    #
    list(APPEND ${var_name} ${${path_var_name}})
    #
    # message("  Lib: ${${path_var_name}}")
    #
  endforeach()
  #
endforeach()

set(OPS_HAVE_ANTLR FALSE)

if(MSVC)
  #
  if(OPS_ANTLR_DIR_LIB_DEBUG)
    #
    find_library(
      lib_path_antlr_debug antlr3cd
      PATHS ${OPS_ANTLR_DIR_LIB_DEBUG})
    #
  else()
    #
    if (MSVC_C_ARCHITECTURE_ID MATCHES 64 OR MSVC_CXX_ARCHITECTURE_ID MATCHES 64)
      find_library(
              lib_path_antlr_debug antlr-3.2-ops-x64-debug
              PATHS ${libs_dir})
    else()
      find_library(
        lib_path_antlr_debug antlr-3.2-ops-debug
        PATHS ${libs_dir})
    endif()
    #
  endif()
  #
  if(OPS_ANTLR_DIR_LIB_RELEASE)
    #
    find_library(
      lib_path_antlr_release antlr3c
      PATHS ${OPS_ANTLR_DIR_LIB_RELEASE})
    #
  else()
    #
    if (MSVC_C_ARCHITECTURE_ID MATCHES 64 OR MSVC_CXX_ARCHITECTURE_ID MATCHES 64)
      find_library(
              lib_path_antlr_release antlr-3.2-ops-x64
              PATHS ${libs_dir})
    else()
      find_library(
        lib_path_antlr_release antlr-3.2-ops
        PATHS ${libs_dir})
    endif()
    #
  endif()
  #
  if(lib_path_antlr_debug)
    #
    set(OPS_HAVE_ANTLR TRUE)
    #
  endif()
  #
elseif(UNIX)
  #
  unset(lib_path_antlr_common CACHE)
  #
  find_library(
    lib_path_antlr_common antlr3c
    PATHS ${OPS_ANTLR_DIR}/lib)
  #
  if(lib_path_antlr_common)
    #
    set(OPS_HAVE_ANTLR TRUE)
    #
  else()
    #
    set(OPS_HAVE_ANTRL FALSE)
    #
  endif()
  #
endif()

foreach(config ${configs})
  #
  # message("cfg: ${config}")
  #
  set(var_name lib_full_paths_${config})
  #
  foreach(lib_path ${${var_name}})
    #
    if(NOT lib_path)
      #
      message(
        WARNING
        "Library ${lib_path} not found in ${search_path}")
      #
    endif()
    #
    # message("${lib_path}")
    #
  endforeach()
  #
endforeach()

function(
  internal_common_settings
  target_name
  target_type
  set_qt_defs
  qt_libraries
  use_antlr)
  #
  # message("> ${CMAKE_CURRENT_BINARY_DIR}")
  # message("> ${CMAKE_CURRENT_BINARY_DIR}")
  #
  # ${ops_source_root_dir}/source/3rd is included to make
  #   ill-formed includes like
  #   ../../source/Analysis/LatticeGraph/foo.h
  #   work from deeply located sources e. g.
  #   source\Analysis\LatticeGraph\testmod4\generated\bar.cpp
  #
  include_directories(
    ${ops_source_root_dir}/include
    ${ops_source_root_dir}/source/3rd
    )
  #
  if(MSVC)
    #
    if(OPS_ANTLR_DIR_INCLUDE)
      #
      include_directories(
        ${OPS_ANTLR_DIR_INCLUDE}
        )
      #
    endif()
    #
    if(NOT OPS_ANTLR_DIR_INCLUDE)
      #
      include_directories(
        ${depends_dir}/deps/include
        )
      #
    endif()
    #
    if(NOT OPS_LLVM_DIR_DEBUG AND NOT OPS_LLVM_DIR_RELEASE)
      #
      include_directories(
        ${depends_dir}/source/3rd/LLVM/include
        ${depends_dir}/source/3rd/LLVM/tools/clang/include
        )
      #
    else()
      #
      set_property(
        TARGET ${target_name}
        APPEND
        PROPERTY
          INCLUDE_DIRECTORIES
          $<$<CONFIG:Debug>:${OPS_LLVM_DIR_DEBUG}/include>
          $<$<NOT:$<CONFIG:Debug>>:${OPS_LLVM_DIR_RELEASE}/include>)
      #
    endif()
    #
  elseif(UNIX)
    #
    include_directories(
      ${OPS_LLVM_DIR}/include
      )
    #
  endif()
  #
  include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    )
  #
  add_definitions(
    -DUNICODE
    -D_UNICODE
    -DOPS_LOCALE=0
    )
  #
  if(MSVC)
    #
    # message("QT_DEFINITIONS == ${QT_DEFINITIONS}")
    #
    # Fix MSVC 2012 error with gtest includes using std::tr1::tuple with too many arguments
    #
    add_definitions(
      -D_VARIADIC_MAX=10
      )
    #
    # Turning off wchar_t as a built-in type if needed
    #
    if(NOT OPS_NATIVE_WCHAR_T OR QT_DEFINITIONS MATCHES /Zc:wchar_t-)
      #
      # message("wchar_t bingo")
      #
      add_definitions(
        /Zc:wchar_t-
        -DOPS_NATIVE_WCHAR_T=0
        )
      #
    endif()
    #
    # If Visual C++ 9.0 (2008) or later is used
    #
    if(
      OPS_PARALLEL_BUILD AND
      MSVC_VERSION GREATER 1400)
      #
      # Process files for one projects in parallel
      #
      add_definitions(/MP)
      #
    endif()
    #
  elseif(UNIX)
    #
    if(CMAKE_BUILD_TYPE STREQUAL Debug)
      #
      add_definitions(
        -D_DEBUG)
      #
    else()
      #
      add_definitions(
        -DNDEBUG)
      #
    endif()
    #
    if(OPS_HAVE_ANTLR)
      #
      add_definitions(-DOPS_HAVE_ANTLR=1)
      #
    else()
      #
      add_definitions(-DOPS_HAVE_ANTLR=0)
      #
    endif()
    #
    # To build LLVM under Linux
    #
    add_definitions(
      -D__STDC_LIMIT_MACROS
      -D__STDC_CONSTANT_MACROS
      #-fpermissive
      )
    #
  endif()
  #
  # Adding Qt specific defines
  #
  if(set_qt_defs)
    #
    add_definitions(
      -DQT_CORE_LIB
      -DQT_THREAD_SUPPORT
      )
    #
    if(MSVC)
      #
      add_definitions(
        -DQT_LARGEFILE_SUPPORT
        )
      #
    endif()
    #
  endif()
  #
  # message("  target_name = ${target_name}")
  #
  # Linking libraries
  #
  # message("'configs' == '${configs}'")
  #
  foreach(config ${configs})
    #
    # message("'config' == '${config}'")
    #
    set(config_group)
    if(config STREQUAL debug)
      #
      # message("bingo debug")
      #
      set(config_group debug)
      #
    elseif(config STREQUAL release)
      #
      # message("bingo release")
      #
      set(config_group optimized)
      #
    endif()
    #
    # message("'config_group' == '${config_group}'")
    # message("'lib_full_paths_${config}' == '${lib_full_paths_${config}}'")
    #
    foreach(path ${lib_full_paths_${config}})
      #
      if(path)
        #
        # message("path == ${path}")
        #
        target_link_libraries(
          ${target_name}
          ${config_group} ${path})
        #
      endif()
      #
    endforeach()
    #
    if(use_antlr)
      #
      if(MSVC)
        #
        if(NOT OPS_ANTLR_DIR_INCLUDE)
          #
          include_directories(
            ${depends_dir}/deps/include/antlr-3.2
          )
          #
        endif()
        #
      elseif(UNIX)
        #
        include_directories(
          ${OPS_ANTLR_DIR}/include
        )
        #
      endif()
      #
      foreach(config ${configs})
        #
        set(config_group)
        if(config STREQUAL debug)
          #
          set(config_group debug)
          #
        elseif(config STREQUAL release)
          #
          set(config_group optimized)
          #
        endif()
        #
        target_link_libraries(
          ${target_name}
          ${config_group} ${lib_path_antlr_${config}})
        #
      endforeach()
      #
    endif()
    #
  endforeach()
  #
  # Adding Qt libraries if needed
  #
  if(qt_libraries)
    #
    target_link_libraries(
      ${target_name} ${qt_libraries})
    #
  endif()
  #
  # Adding POSIX libraries
  #
  if(UNIX)
    #
    target_link_libraries(
      ${target_name}
      -lpthread -ldl
      )
    #
  endif()
  #
  # Adding settings for libraries
  #
  if(target_type STREQUAL LIB)
    #
    # add_definitions(
    #   -D_LIB
    #   )
    #
  endif()
  #
endfunction()

function(
  internal_create_source_groups
  relativeSourcePath
  sourceFiles)
  #
  foreach(currentSourceFile ${ARGN})
    #
    get_filename_component(
      absolute_path ${currentSourceFile} ABSOLUTE)
    #
    file(
      RELATIVE_PATH folder ${relativeSourcePath} ${absolute_path})
    #
    get_filename_component(
      filename ${folder} NAME)
    #
    string(REPLACE ${filename} "" folder ${folder})
    #
    if(NOT folder STREQUAL "")
      #
      string(REGEX REPLACE "/+$" "" folderlast "${folder}")
      string(REPLACE "/" "\\" folderlast "${folderlast}")
      string(REPLACE "..\\" "" folderlast "${folderlast}")
      #
      if(${currentSourceFile} MATCHES ".*\\.h")
        #
        set(sourceGroupName "Header Files")
        #
      else()
        #
        set(sourceGroupName "Source Files")
        #
      endif()
      #
      # message("${sourceGroupName}\\${folderlast} += ${currentSourceFile}")
      #
      source_group(
        "${sourceGroupName}\\${folderlast}" FILES ${currentSourceFile})
      #
    endif()
    #
  endforeach()
  #
endfunction()

# OPS project description interface functions

function(
  ops_project
  target_name target_type)
  #
  set(
    enities
    SOURCES
    QT_UIS
    QT_MOC_HEADERS
    QT_RESOURCES
    TEMPLATES
    LIBRARIES
    DEPENDS
    )
  #
  foreach(entity ${enities})
    #
    set(list_${entity})
    #
  endforeach()
  #
  # Sorting out the input arguments into lists
  #
  set(auto_find_moc_hdrs FALSE)
  set(inside_entity SOURCES)
  set(public_project FALSE)
  foreach(arg ${ARGN})
    #
    list(FIND enities ${arg} index)
    if(NOT index EQUAL -1)
      #
      set(inside_entity ${arg})
      #
    elseif(arg STREQUAL QT_AUTO_FIND_MOC_HDRS)
      #
      set(auto_find_moc_hdrs True)
      #
    elseif(arg STREQUAL PUBLIC)
      #
      set(public_project TRUE)
      #
    else()
      #
      list(APPEND list_${inside_entity} ${arg})
      #
    endif()
    #
  endforeach()
  #
  # Check for testmods
  #
  if(${target_name} MATCHES ".*_test" OR
     ${target_name} MATCHES ".*testmod")
    #
    if(NOT OPS_TESTMODS_TO_BUILD STREQUAL "ALL")
      #
      set(skip_testmod True)
      #
      foreach(testmod ${OPS_TESTMODS_TO_BUILD})
        #
        if(${target_name} STREQUAL ${testmod})
          #
          set(skip_testmod FALSE)
          #
        endif()
        #
      endforeach()
      #
      if(skip_testmod)
        #
        return()
        #
      endif()
      #
    endif()
    #
  endif()
  #
  # Qt, Antlr setup from templates
  #
  set(use_antlr FALSE)
  set(set_qt_defs FALSE)
  set(qt_modules)
  set(qt_modules_winmain FALSE)
  set(target_flags)
  #
  foreach(name ${list_TEMPLATES})
    #
    if(name STREQUAL GTest)
      #
      list(
        APPEND
        list_SOURCES
        "${ops_source_root_dir}/source/3rd/gtest/gtest-all.cc")
      #
    elseif(name STREQUAL ANTLR)
      #
      if(NOT OPS_HAVE_ANTLR)
        #
        message(
          STATUS
          "ANTLR library was not found, "
          "project ${target_name} will not be built")
        #
        return()
        #
      endif()
      #
      set(use_antlr TRUE)
      #
    elseif(name STREQUAL QtWinMain)
      #
      if(MSVC)
        #
        set(qt_modules_winmain TRUE)
        set(target_flags WIN32)
        #
      endif()
      #
    else()
      #
      set(
        known_qt_modules
        QtCore QtGui QtWidgets QtXml QtSvg QtScript)
      #
      list(FIND known_qt_modules "${name}" index)
      #
      if(NOT index EQUAL -1)
        #
        string(SUBSTRING ${name} 2 -1 qt_module_name)
        list(APPEND qt_modules "${qt_module_name}")
        #
      endif()
      #
    endif()
    #
    if(name STREQUAL Qt OR name STREQUAL QtGui)
      #
      set(set_qt_defs True)
      #
    endif()
    #
  endforeach()
  #
  if(NOT "${qt_modules}" STREQUAL "")
    #
    if(NOT OPS_BUILD_QT_PROJECTS)
      #
      message(
        STATUS
        "Building Qt projects is turned off, "
        "project ${target_name} will not be built")
      #
      return()
      #
    endif()
    #
    if(NOT Qt5Widgets_FOUND)
      message(STATUS "Qt5 is not found, project ${target_name} will not be built.")
      return()
    endif()
    #
    list(REMOVE_DUPLICATES qt_modules)
    #
    if(list_QT_UIS)
      #
      qt5_wrap_ui(files_ui_gen ${list_QT_UIS})
      #
    endif()
    #
    # Searching for Qt MOC headers
    #
    if(list_QT_MOC_HEADERS)
      #
      list(
        APPEND
        list_SOURCES ${list_QT_MOC_HEADERS})
      #
    elseif(auto_find_moc_hdrs)
      #
      #message(${list_SOURCES})
      foreach(fname ${list_SOURCES})
        #
        string(TOLOWER "${fname}" fname_lower)
        if(fname_lower MATCHES "[.]*.h(|pp|xx)$")
          #
          file(READ "${fname}" h_contents)
          #
          if(
            "${h_contents}" MATCHES
              "[.]*(Q_|QOM_)(OBJECT|GADGET|M_OBJECT)[.]*")
            #
            list(
              APPEND
              list_QT_MOC_HEADERS ${fname})
            #
            # message("  bingo: ${fname}")
            #
          endif()
          #
        endif()
        #
      endforeach()
      #
    endif()
    #
    if(list_QT_MOC_HEADERS)
      #
      qt5_wrap_cpp(
        files_moc_gen ${list_QT_MOC_HEADERS})
      #
      # message("files_moc_gen == ${files_moc_gen}")
      #
    endif()
    #
    # message("list_QT_RESOURCES == ${list_QT_RESOURCES}")
    #
    if(list_QT_RESOURCES)
      #
      qt5_add_resources(
        files_resource_gen ${list_QT_RESOURCES})
      #
      # message("files_resource_gen == ${files_resource_gen}")
      #
    endif()
    #
    if(QT_INCLUDE_DIR)
      #
      include_directories(
        ${QT_QTCORE_INCLUDE_DIR}
        ${QT_INCLUDE_DIR}
        ${QT_MKSPECS_DIR}/default
        )
      #
    endif()
    #
  endif()
  #
  internal_create_source_groups(
    ${CMAKE_CURRENT_SOURCE_DIR} ${list_SOURCES})
  #
  #
  # message("list_SOURCES == ${list_SOURCES}")
  # message("list_QT_UIS == ${list_QT_UIS}")
  # message("qt_libs == ${qt_libs}")
  # message("files_ui_gen == ${files_ui_gen}")
  #
  if(files_ui_gen)
    #
    list(APPEND list_SOURCES ${files_ui_gen})
    #
  endif()
  #
  if(files_moc_gen)
    #
    list(APPEND list_SOURCES ${files_moc_gen})
    #
  endif()
  #
  if(files_resource_gen)
    #
    list(APPEND list_SOURCES ${files_resource_gen})
    #
  endif()
  #
  if(
    files_ui_gen OR
    files_moc_gen OR
    files_resource_gen)
    #
    source_group(
      Generated
      FILES
        ${files_ui_gen}
        ${files_moc_gen}
        ${files_resource_gen})
    #
  endif()
  #
  if(OPS_UNITY_BUILD)
    #
    # Generate a unique filename for the unity build translation unit
    #
    set(unity_build_file ${CMAKE_CURRENT_BINARY_DIR}/ub_${target_name}.cpp)
    #
    # Exclude all translation units from compilation
    # set_source_files_properties(${list_SOURCES} PROPERTIES HEADER_FILE_ONLY true)
    #
    # Open the ub file
    #
    file(
      WRITE
      ${unity_build_file} "// Unity Build generated by CMake\n")
    #
    # Add include statement for each translation unit
    #
    foreach(source_file ${list_SOURCES})
      #
      get_filename_component(source_file_ext ${source_file} EXT)
      #
      #message(${source_file_ext})
      #
      if(${source_file_ext} STREQUAL ".cpp")
        #
        file(
          APPEND ${unity_build_file}
          "#include <${CMAKE_CURRENT_SOURCE_DIR}/${source_file}>\n")
        #
      endif()
      #
    endforeach(source_file)
    #
    # Complement list of translation units with the name of ub
    #
    set(list_SOURCES ${unity_build_file})
    #
  endif()
  #
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
  #
  if("${target_type}" STREQUAL "APP")
    #
    add_executable(
      "${target_name}" ${target_flags} ${list_SOURCES})
    #
    if(public_project)
      #
      install(
        TARGETS ${target_name}
        RUNTIME DESTINATION bin
		COMPONENT Programs)
      #
    endif()
    #
  elseif("${target_type}" STREQUAL "LIB")
    #
    add_library(
      "${target_name}" ${list_SOURCES})
    #
    if(public_project)
      #
	  if(CMAKE_BUILD_SHARED_LIBS)
	    set(component_name SharedLibs)
      else()
	    set(component_name StaticLibs)
	  endif()

      install(
        TARGETS ${target_name}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
		COMPONENT ${component_name})
      #
    endif()
    #
  else()
    #
    message(
      SEND_ERROR
      "Unknown configration type (${target_type}) "
      "for the project ${target_name}, should be "
      "APP or LIB")
    #
  endif()
  #
  if(qt_modules)
    qt5_use_modules(${target_name} ${qt_modules})
    if(qt_modules_winmain)
      target_link_libraries(${target_name} Qt5::WinMain)
    endif()
  endif()
  #
  #
  if(list_LIBRARIES)
    #
    target_link_libraries(
      "${target_name}" ${list_LIBRARIES})
    #
  endif()
  #
  if(list_DEPENDS)
    #
    # message("  Deps: ${${var_name}}")
    #
    # add_dependencies(
    #   "${target_name}" ${list_DEPENDS})
    #
  endif()
  #
  internal_common_settings(
    "${target_name}" ${target_type}
    ${set_qt_defs} "${QT_LIBRARIES}"
    ${use_antlr})
  #
endfunction()

function(
  ops_filesin
  result
  prefix)
  #
  string(REGEX MATCH "/$" last ${prefix})
  #
  if(NOT last STREQUAL "/")
    #
    set(prefix ${prefix}/)
    #
  endif()
  #
  set(files_prefixed)
  foreach(f ${ARGN})
    #
    list(APPEND files_prefixed "${prefix}${f}")
    #
  endforeach()
  #
  set(${result} ${files_prefixed} PARENT_SCOPE)
  #
endfunction()

# End of File
