# Created by the script cgal_create_cmake_script
# This is the CMake script for compiling a CGAL application.

cmake_minimum_required(VERSION 3.12...3.31)
project(Spatial_sorting_Tests)

find_package(CGAL REQUIRED)

create_single_source_cgal_program("test_hilbert.cpp")
create_single_source_cgal_program("test_multiscale.cpp")

find_package(TBB QUIET)
include(CGAL_TBB_support)
if(TARGET CGAL::TBB_support)
  message(STATUS "Found TBB")
  target_link_libraries(test_hilbert PRIVATE CGAL::TBB_support)
endif()
