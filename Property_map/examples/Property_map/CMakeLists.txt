cmake_minimum_required(VERSION 3.12...3.31)
project(Property_map_Examples)

# CGAL and its components
find_package(CGAL REQUIRED)

create_single_source_cgal_program("dynamic_properties.cpp")

find_package(Eigen3 3.1.0 QUIET) #(requires 3.1.0 or greater)
include(CGAL_Eigen3_support)
if(TARGET CGAL::Eigen3_support)
  create_single_source_cgal_program("custom_property_map.cpp")
  target_link_libraries(custom_property_map PRIVATE CGAL::Eigen3_support)
else()
  message(STATUS "NOTICE: The example 'custom_property_map' requires Eigen 3.1 (or greater), and will not be compiled.")
endif()
