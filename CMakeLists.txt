cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0042 NEW)

project (node-indri)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DP_NEEDS_GNU_CXX_NAMESPACE=1 -D_GLIBCXX_USE_CXX11_ABI=0 -std=c++11 -g -O3 -Wno-deprecated-declarations")

include_directories(${CMAKE_JS_INC})

set(INDRI_PATH ./lib/)
include_directories(${INDRI_PATH}/include/)
link_directories(${INDRI_PATH}/lib/)

file(GLOB SOURCE_FILES "src/*.cc")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Include N-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE NODE_ADDON_API_DIR
)

string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} indri z pthread m)

# define NPI_VERSION
add_definitions(-DNAPI_VERSION=3)
