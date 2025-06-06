# This is the CMake script for compiling a CGAL application.

cmake_minimum_required(VERSION 3.12...3.31)
project(Arrangement_on_surface_2_Demo)

if(NOT POLICY CMP0070 AND POLICY CMP0053)
  # Only set CMP0053 to OLD with CMake<3.10, otherwise there is a warning.
  cmake_policy(SET CMP0053 OLD)
endif()

if(POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
endif()

find_package(CGAL REQUIRED OPTIONAL_COMPONENTS Core Qt6)
find_package(Qt6 QUIET COMPONENTS Widgets)

if (CGAL_Qt6_FOUND AND Qt6_FOUND)
  add_compile_definitions(QT_NO_KEYWORDS)
  set(CMAKE_INCLUDE_CURRENT_DIR ON)

  # Arrangement package includes
  add_compile_definitions(QT_NO_KEYWORDS)
  option(COMPILE_UTILS_INCREMENTALLY
    "Compile files in Utils directory incrementally, or compile them all as a unit. \
    Incremental compilation will be better for development and consume less \
    memory while compiling but will take longer to compile."
    OFF)

  set(UTILS_SOURCE_FILES
       "Utils/Utils.cpp"
       "Utils/PointLocationFunctions.cpp"
       "Utils/ConstructBoundingBox.cpp"
       "Utils/EnvelopeFunctions.cpp"
       "Utils/ConstructSegment.cpp"
       "Utils/IntersectCurves.cpp"
       "Utils/SplitAndMerge.cpp"
  )

  if (COMPILE_UTILS_INCREMENTALLY)
    set(UTILS_COMPILE_FILES ${UTILS_SOURCE_FILES})
  else()
    set(UTILS_CPP_FILES_INCLUDES "")
    foreach(utils_src IN LISTS UTILS_SOURCE_FILES)
      string(APPEND UTILS_CPP_FILES_INCLUDES "#include \"${utils_src}\"\n")
    endforeach()
    file(WRITE "${CMAKE_BINARY_DIR}/CombinedUtils.cpp" ${UTILS_CPP_FILES_INCLUDES})
    set(UTILS_COMPILE_FILES "${CMAKE_BINARY_DIR}/CombinedUtils.cpp")
  endif()


  qt6_wrap_ui(arrangement_2_uis
              ArrangementDemoWindow.ui
              NewTabDialog.ui
              OverlayDialog.ui
              ArrangementDemoPropertiesDialog.ui
              AlgebraicCurveInputDialog.ui
              RationalCurveInputDialog.ui)

  qt6_wrap_cpp(CGAL_Qt6_MOC_FILES
               ArrangementDemoWindow.h
               ArrangementDemoTab.h
               GraphicsViewCurveInput.h
               Callback.h
               OverlayDialog.h
               ArrangementDemoPropertiesDialog.h
               AlgebraicCurveInputDialog.h
               RationalCurveInputDialog.h
               ColorItemEditor.h
               PropertyValueDelegate.h)

  qt6_add_resources(CGAL_Qt6_RESOURCE_FILES Arrangement_on_surface_2.qrc)

  add_executable(arrangement_2
                 arrangement_2.cpp
                 ArrangementDemoWindow.cpp
                 ArrangementDemoTab.cpp
                 ArrangementDemoGraphicsView.cpp
                 ArrangementGraphicsItem.cpp
                 Callback.cpp
                 VerticalRayShootCallback.cpp
                 EnvelopeCallback.cpp
                 SplitEdgeCallback.cpp
                 FillFaceCallback.cpp
                 MergeEdgeCallback.cpp
                 PointLocationCallback.cpp
                 NewTabDialog.cpp
                 OverlayDialog.cpp
                 ArrangementDemoPropertiesDialog.cpp
                 AlgebraicCurveInputDialog.cpp
                 RationalCurveInputDialog.cpp
                 ColorItemEditor.cpp
                 PropertyValueDelegate.cpp
                 PointsGraphicsItem.cpp
                 VerticalRayGraphicsItem.cpp
                 DeleteCurveCallback.cpp
                 CurveGraphicsItem.cpp
                 ArrangementPainterOstream.cpp
                 GraphicsViewCurveInput.cpp
                 AlgebraicCurveParser.cpp
                 GraphicsSceneMixin.cpp
                 GridGraphicsItem.cpp
                 PointSnapper.cpp
                 CurveInputMethods.cpp
                 FloodFill.cpp
                 ArrangementIO.cpp
                 ${UTILS_COMPILE_FILES}
                 ${arrangement_2_uis}
                 ${CGAL_Qt6_RESOURCE_FILES}
                 ${CGAL_Qt6_MOC_FILES})

  target_link_libraries(arrangement_2 PRIVATE Qt6::Widgets)
  target_link_libraries(arrangement_2 PRIVATE CGAL::CGAL CGAL::CGAL_Qt6)
  if(CGAL_Core_FOUND)
    target_link_libraries(arrangement_2 PRIVATE CGAL::CGAL_Core)
  endif()

  add_to_cached_list(CGAL_EXECUTABLE_TARGETS arrangement_2)

  include(${CGAL_MODULES_DIR}/CGAL_add_test.cmake)
  cgal_add_compilation_test(arrangement_2)

else()
  set(MISSING_DEPS "")

  if(NOT CGAL_Qt6_FOUND)
    set(MISSING_DEPS "the CGAL Qt6 library, ${MISSING_DEPS}")
  endif()
  if(NOT Qt6_FOUND)
    set(MISSING_DEPS "Qt6, ${MISSING_DEPS}")
  endif()
  message("NOTICE: This demo requires ${MISSING_DEPS} and will not be compiled.")
endif()
