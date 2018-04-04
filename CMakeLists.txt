cmake_minimum_required(VERSION 2.8)

# Name of the project (will be the name of the plugin)
project(node-indri)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DP_NEEDS_GNU_CXX_NAMESPACE=1 -D_GLIBCXX_USE_CXX11_ABI=0 -std=c++11 -g -O3 -Wno-deprecated-declarations")



IF (APPLE)
    SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "/usr/local/include/")
    SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "/usr/local/lib/")
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "/usr/lib/")
ENDIF (APPLE)

# Build a shared library named after the project from the files in `src/`
file(GLOB SOURCE_FILES "src/*.cc" "src/*.h")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

# Gives our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Essential include files to build a node addon,
# You should add this line in every CMake.js based project
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})

# Essential library files to link to a node addon
# You should add this line in every CMake.js based project



target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} indri z pthread m)
