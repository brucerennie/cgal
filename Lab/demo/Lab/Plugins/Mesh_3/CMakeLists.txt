include(CGALlab_macros)

remove_definitions(-DQT_STATICPLUGIN)

set(CMAKE_AUTOMOC ON)

qt6_wrap_cpp(VOLUME_MOC_OUTFILES
             ${CMAKE_CURRENT_SOURCE_DIR}/Volume_plane_thread.h)
qt6_wrap_cpp(VOLUME_MOC_OUTFILES
             ${CMAKE_CURRENT_SOURCE_DIR}/Volume_plane_interface.h)
qt6_wrap_ui(meshingUI_FILES Meshing_dialog.ui Smoother_dialog.ui
            Local_optimizers_dialog.ui)
cgal_lab_plugin(
  mesh_3_plugin
  Mesh_3_plugin
  Mesh_3_plugin_cgal_code.cpp
  Meshing_thread.cpp
  split_polylines.cpp
  ${meshingUI_FILES}
  KEYWORDS
  Mesh_3)
target_link_libraries(
  mesh_3_plugin
  PRIVATE scene_polygon_soup_item
          scene_polylines_item
          scene_implicit_function_item
          scene_image_item
          scene_surface_mesh_item
          scene_c3t3_item
          ${OPENGL_gl_LIBRARY})
target_include_directories(mesh_3_plugin PRIVATE include)
CGAL_Lab_target_use_c3t3_type(mesh_3_plugin)

set(CMAKE_POLICY_DEFAULT_CMP0167 NEW)
find_package(ITK NAMES ITK InsightToolkit
                 QUIET COMPONENTS ITKCommon ITKThresholding ITKSmoothing ITKImageIntensity)
if(ITK_FOUND)
  include(CGAL_ITK_support)
  message(STATUS "ITK found")
  include(${ITK_USE_FILE})
  target_link_libraries(mesh_3_plugin PRIVATE CGAL::ITK_support)
endif(ITK_FOUND)

find_package(VTK 9.0 QUIET COMPONENTS ImagingGeneral IOImage IOXML NO_MODULE)
if(VTK_FOUND AND VTK_LIBRARIES)
  message(STATUS "VTK ${VTK_VERSION} found ${VTK_LIBRARIES}")
else()
  message(STATUS "NOTICE: DICOM files (.dcm) require the VTK libraries, and will not be readable.")
endif()

find_package(Boost QUIET OPTIONAL_COMPONENTS filesystem system)
if(Boost_FILESYSTEM_FOUND AND Boost_SYSTEM_FOUND)
  qt6_wrap_ui( imgUI_FILES Image_res_dialog.ui raw_image.ui)
  cgal_lab_plugin(io_image_plugin Io_image_plugin
    Volume_plane_intersection.cpp
    Raw_image_dialog.cpp
    ${imgUI_FILES}
    ${VOLUME_MOC_OUTFILES}
    KEYWORDS Viewer Mesh_3)
  target_link_libraries(io_image_plugin PRIVATE scene_image_item ${VTK_LIBRARIES} CGAL::CGAL_ImageIO)
  if(VTK_LIBRARIES)
    target_compile_definitions(io_image_plugin PRIVATE -DCGAL_USE_VTK -DNOMINMAX)
  endif()

  target_link_libraries(io_image_plugin PRIVATE Boost::filesystem Boost::system)

else()
  message(STATUS "NOTICE: the Io_image_plugin requires boost-filesystem, and will not be compiled")
endif()
cgal_lab_plugin(
  mesh_3_optimization_plugin
  Optimization_plugin
  Optimization_plugin_cgal_code.cpp
  Optimizer_thread.cpp
  ${meshingUI_FILES}
  KEYWORDS
  Mesh_3)
target_link_libraries(
  mesh_3_optimization_plugin
  PRIVATE scene_c3t3_item scene_surface_mesh_item scene_image_item
          scene_implicit_function_item)
CGAL_Lab_target_use_c3t3_type(mesh_3_optimization_plugin)

# Use Eigen
find_package(Eigen3 3.1.0 QUIET) #(3.1.0 or greater)
include(CGAL_Eigen3_support)
if(TARGET CGAL::Eigen3_support)
  target_link_libraries(mesh_3_optimization_plugin PRIVATE CGAL::Eigen3_support)
else()
  message(STATUS "NOTICE: The Mesh_3_optimization_plugin requires Eigen, which was not found."
                 "A deprecated class will be used to replace it. Warnings are to be expected.")
endif()

cgal_lab_plugin(c3t3_io_plugin C3t3_io_plugin KEYWORDS Viewer Mesh_3)
target_link_libraries(c3t3_io_plugin PRIVATE scene_c3t3_item)
CGAL_Lab_target_use_c3t3_type(c3t3_io_plugin)

qt6_wrap_ui(tetraUI_FILES Tetrahedra_filter_widget.ui)
cgal_lab_plugin(tetrahedra_filtering_plugin Tetrahedra_filtering_plugin ${tetraUI_FILES} KEYWORDS Mesh_3 Viewer)
target_link_libraries(tetrahedra_filtering_plugin  PRIVATE scene_c3t3_item scene_tetrahedra_item)
CGAL_Lab_target_use_c3t3_type(tetrahedra_filtering_plugin)

qt6_wrap_ui(ribUI_FILES Rib_dialog.ui)
cgal_lab_plugin(c3t3_rib_exporter_plugin C3t3_rib_exporter_plugin
                       ${ribUI_FILES} KEYWORDS Mesh_3)
target_link_libraries(c3t3_rib_exporter_plugin PRIVATE scene_c3t3_item)
CGAL_Lab_target_use_c3t3_type(c3t3_rib_exporter_plugin)

qt6_wrap_ui(offsetMeshingUI_FILES Offset_meshing_dialog.ui)
cgal_lab_plugin(offset_meshing_plugin Offset_meshing_plugin
                       ${offsetMeshingUI_FILES})
target_link_libraries(offset_meshing_plugin PRIVATE scene_surface_mesh_item
                                                    scene_polygon_soup_item
                                                    scene_polylines_item)
if(TARGET CGAL::Eigen3_support)
  target_link_libraries(offset_meshing_plugin PRIVATE CGAL::Eigen3_support)
endif()
if(TARGET CGAL::TBB_support)
  target_link_libraries(offset_meshing_plugin PRIVATE CGAL::TBB_support)
endif()
