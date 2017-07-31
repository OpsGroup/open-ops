# CMakeLists.txt for OPS5 project

cmake_minimum_required(VERSION 2.8.7)

# Add path for custom modules
set(
  CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}")

project(ops)

include(ops)

enable_testing()

add_subdirectory(source)
#add_subdirectory(unittests)
add_subdirectory(docs)
add_subdirectory(samples)
add_subdirectory(install)

# End of File
