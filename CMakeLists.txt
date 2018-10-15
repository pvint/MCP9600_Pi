cmake_minimum_required(VERSION 3.0)
project(MCP9600)

# add flags
#set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -D_BSD_SOURCE -std=c11 -Wall -pedantic -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -D_BSD_SOURCE -std=c11 -pedantic -lm")



# build
include_directories ("${PROJECT_SOURCE_DIR}/src")
add_executable (mcp9600 src/mcp9600.c)
target_link_libraries(mcp9600)

# install
install(TARGETS mcp9600 DESTINATION bin)
