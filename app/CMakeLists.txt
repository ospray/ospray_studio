## Copyright 2018-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if (APPLE)
  set_source_files_properties(imgui_impl_glfw_gl3.cpp GLFWSgWindow.cpp
    PROPERTIES COMPILE_FLAGS -w)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_SILENCE_DEPRECATION")
endif()

add_executable(ospStudio
  ospStudio.cpp
  PluginManager.cpp

  MainWindow.cpp
  Batch.cpp
  TimeSeriesWindow.cpp
  AnimationManager.cpp

  ${OSPRAY_STUDIO_RESOURCE_FILE}
)

target_compile_definitions(ospStudio PRIVATE OSPRAY_CPP_RKCOMMON_TYPES)
if(APPLE)
  set_target_properties(ospStudio PROPERTIES
    SKIP_INSTALL_RPATH OFF
    INSTALL_RPATH "@executable_path;@executable_path/../${CMAKE_INSTALL_LIBDIR};${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
  )
elseif(UNIX)
  set_target_properties(ospStudio PROPERTIES
    SKIP_INSTALL_RPATH OFF
    INSTALL_RPATH "\$ORIGIN:\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
    LINK_OPTIONS "LINKER:-disable-new-dtags"
  )
endif()

add_library(ospray_ui STATIC
  ArcballCamera.cpp
  imgui_impl_glfw.cpp
  imgui_impl_opengl2.cpp
  widgets/TransferFunctionWidget.cpp
  widgets/FileBrowserWidget.cpp
  widgets/AnimationWidget.cpp
)

target_link_libraries(ospray_ui
  ospray_sg
  imgui
  imguiFileDialog
  json
  ${OPENGL_LIBRARIES}
  glfw
)

target_link_libraries(ospStudio
  ospray_ui # Not all run modes should link against UI (ie batch).
  ospray_sg
  json
)

_target_strip_and_sign(ospStudio)

install(TARGETS ospStudio
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)

get_target_property(OSPRAY_LIBNAME ospray::ospray IMPORTED_LOCATION_RELEASE)

string(REGEX MATCH "^.*/" _sharedlib_glob "${OSPRAY_LIBNAME}")
string(APPEND _sharedlib_glob "*${CMAKE_SHARED_LIBRARY_SUFFIX}*")
# message(STATUS "_sharedlib_glob: ${_sharedlib_glob}")
file(GLOB _sharedlibs LIST_DIRECTORIES false "${_sharedlib_glob}")
# message(STATUS "_sharedlibs: ${_sharedlibs}")

if(WIN32)
  install(FILES
    ${_sharedlibs} DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
else()
  install(FILES
    ${_sharedlibs} DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endif()
