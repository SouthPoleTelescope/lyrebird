set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unused -Werror")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
find_package(GLFW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(FFTW REQUIRED)
find_package(X11 REQUIRED)

find_package(Spt3g REQUIRED)

find_package(PythonInterp)
find_package(PythonLibs)
find_package(Boost COMPONENTS python serialization iostreams filesystem REQUIRED)

add_subdirectory(AntTweakBar)

include_directories("./AntTweakBar/include/" ${GLFW_INCLUDE_DIR} ${FFTW_INCLUDE_DIRS} 
  ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS} ${X11_INCLUDE_DIRS} ${SPT3G_INCLUDES})

include_directories(${Boost_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_PATH})

add_library(lyrebirdvis STATIC
  geometryutils.cpp genericutils.cpp shader.cpp nanosvg.cpp simplerender.cpp configparsing.cpp 
  datastreamer.cpp datavals.cpp teststreamer.cpp jsoncpp.cpp visualelement.cpp cameracontrol.cpp 
  polygon.cpp highlighter.cpp equation.cpp plotter.cpp plotbundler.cpp equationmap.cpp logging.cpp
  dfmuxstreamer.cpp numberlineart.cpp sockethelper.cpp
)

add_executable(lyrebird main.cpp)

find_library(rt_LIBRARY rt)

target_link_libraries(lyrebird lyrebirdvis AntTweakBar
  ${GLFW_LIBRARIES} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLEW_LIBRARY} ${X11_LIBRARIES}
  ${X11_Xxf86vm_LIB} ${FFTW_LIBRARIES} ${SPT3G_LIBRARIES} ${CMAKE_DL_LIBS} ${rt_LIBRARY}
  ${X11_Xrandr_LIB} ${X11_Xinerama_LIB} ${X11_Xi_LIB} ${X11_Xcursor_LIB} pthread)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

