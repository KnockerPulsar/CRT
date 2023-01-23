cmake_minimum_required(VERSION 3.0.0)
project(CRT VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -funroll-loops -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -Wall")


set(SOURCES 
  src/CLUtil.cpp
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
