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

include_directories("./AntTweakBar/include/" ${GLFW_INCLUDE_PATH} ${FFTW_INCLUDE_DIRS} 
  ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS} ${X11_INCLUDE_DIRS} ${SPT3G_INCLUDES})

include_directories(${Boost_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_PATH})

add_library(lyrebirdvis STATIC
  geometryutils.cpp genericutils.cpp shader.cpp nanosvg.cpp simplerender.cpp configparsing.cpp 
  datastreamer.cpp datavals.cpp teststreamer.cpp jsoncpp.cpp visualelement.cpp cameracontrol.cpp 
  polygon.cpp highlighter.cpp equation.cpp plotter.cpp plotbundler.cpp equationmap.cpp logging.cpp
  dfmuxstreamer.cpp numberlineart.cpp sockethelper.cpp
)

add_executable(lyrebird main.cpp)

target_link_libraries(lyrebird lyrebirdvis AntTweakBar
  ${GLFW_LIBRARY} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLEW_LIBRARY} ${X11_LIBRARIES}
  ${FFTW_LIBRARIES} ${SPT3G_LIBRARIES} )

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)