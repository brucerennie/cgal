cmake_minimum_required(VERSION 3.12...3.31)
project(Compact_container_benchmark)

find_package(CGAL REQUIRED)

find_package(TBB QUIET)
include(CGAL_TBB_support)

create_single_source_cgal_program("cc_benchmark.cpp")

if(TARGET CGAL::TBB_support)
  message(STATUS "Found TBB")
  target_link_libraries(cc_benchmark PRIVATE CGAL::TBB_support)
endif()
