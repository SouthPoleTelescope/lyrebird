cmake_minimum_required (VERSION 2.6)
project (Lyrebird)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unused -Werror")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(GLFW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(FFTW REQUIRED)
find_package(X11 REQUIRED)


include_directories("../contrib/AntTweakBar/include/" ${GLFW_INCLUDE_PATH} ${FFTW_INCLUDE_DIRS} )
add_library(lyrebird STATIC
  geometryutils.cpp genericutils.cpp shader.cpp nanosvg.cpp simplerender.cpp configparsing.cpp datastreamer.cpp datavals.cpp teststreamer.cpp jsoncpp.cpp visualelement.cpp cameracontrol.cpp polygon.cpp highlighter.cpp equation.cpp plotter.cpp plotbundler.cpp equationmap.cpp logging.cpp
)
add_executable(Lyrebird main.cpp)
target_link_libraries(Lyrebird lyrebird ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/AntTweakBar/lib/libAntTweakBar.so 
  ${GLFW_LIBRARY} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLEW_LIBRARY} ${X11_LIBRARIES}
  ${FFTW_LIBRARIES})  
