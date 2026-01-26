cmake_minimum_required(VERSION 3.16)
project(MiniSnowflake VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(Threads REQUIRED)

# Add compile options for performance and debugging
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -Wextra -fsanitize=address")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -DNDEBUG")
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.cc"
)

file(GLOB_RECURSE HEADERS
    "include/*.h"
    "include/*.hpp"
)

# Create main executable
add_executable(mini_snowflake ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(mini_snowflake Threads::Threads)

# Install targets
install(TARGETS mini_snowflake DESTINATION bin)