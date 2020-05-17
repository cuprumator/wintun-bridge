cmake_minimum_required(VERSION 3.17)
project(wintun)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /UMBCS /D_UNICODE /DUNICODE")

file(GLOB src "*.h" "*.cpp" "*.def" "*.rc" "*.rgs")
add_library(wintun ${src})

set_property(TARGET wintun PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(wintun Setupapi)

set_target_properties(wintun PROPERTIES COMPILE_FLAGS "/Yustdafx.h")
set_source_files_properties(stdafx.cpp PROPERTIES COMPILE_FLAGS "/Ycstdafx.h")