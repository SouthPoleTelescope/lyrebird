cmake_minimum_required(VERSION 2.8)
project(AntTweakBar)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
include_directories(include)
################### Build the library ###################
if (APPLE)
set (platform MACOSX)
elseif (UNIX)
set (platform UNIX)
find_package(X11)
else()
message(SEND_ERROR "This platform (${CMAKE_SYSTEM_NAME}) is not handled yet! Consider a pull-request.")
endif()
set(ANTWB_SOURCES
src/TwColors.cpp
src/TwFonts.cpp
src/TwOpenGL.cpp
src/TwOpenGLCore.cpp
src/TwBar.cpp
src/TwMgr.cpp
src/TwPrecomp.cpp
src/LoadOGL.cpp
src/LoadOGLCore.cpp
src/TwEventGLFW.c
src/TwEventGLUT.c
src/TwEventSDL.c
src/TwEventSDL12.c
src/TwEventSDL13.c
src/TwEventSFML.cpp
src/TwEventX11.c
)
add_definitions(-Wall -Wno-unused -Wno-tautological-compare -fno-strict-aliasing -D_${platform} -D__PLACEMENT_NEW_INLINE)
include_directories(${CMAKE_SOURCE_DIR}/../include ${X11_INCLUDE_DIR})
add_library(AntTweakBar ${ANTWB_SOURCES})
