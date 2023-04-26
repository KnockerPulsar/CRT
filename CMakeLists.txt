cmake_minimum_required(VERSION 3.0.0)
project(CRT VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -funroll-loops -Wall ")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -Wall -pedantic-errors -fmax-errors=2")
set(CMAKE_CXX_FLAGS "")

# Enable to use POCL (usually for debugging with GDB)

# if(CMAKE_BUILD_TYPE EQUAL "DEBUG")
# add_compile_definitions(USE_POCL)
# endif()

set(SOURCES 
  src/host/CLUtil.cpp
  src/host/Utils.cpp
  src/host/CLMath.cpp
  src/host/Random.cpp
)

include_directories(
  src/
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY          ${PROJECT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG    ${PROJECT_SOURCE_DIR}/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE  ${PROJECT_SOURCE_DIR}/release)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

add_executable(${PROJECT_NAME} src/main.cpp ${SOURCES})

target_link_libraries(
  ${PROJECT_NAME} 
  -lm 
  -lOpenCL
)
