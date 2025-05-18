cmake_minimum_required(VERSION 3.21.7)

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET "x64-mingw-static" CACHE STRING "Vcpkg target triplet")

message(STATUS "Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "Vcpkg target triplet: ${VCPKG_TARGET_TRIPLET}")

project(steamlibfetcher)

set(SOURCE_FILES
    src/main.cpp
    src/slf.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/steamlibfetcher"
)

message(STATUS "Detected compiler: ${CMAKE_CXX_COMPILER_ID}")
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE 
    /W4 
    /WX 
    /permissive- 
    /utf-8
    /MP
    /sdl
    /guard:cf
  )
else()
  target_compile_options(${PROJECT_NAME} PRIVATE 
    -Wall 
    -Wextra 
    -Wpedantic
  )
  target_link_options(${PROJECT_NAME} PRIVATE
    -static-libgcc
    -static-libstdc++
  )
endif()

message(STATUS "Building with ${CMAKE_CXX_COMPILER_ID} compiler")
message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE 
                           ${CMAKE_SOURCE_DIR}/src/include)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    fmt::fmt
    nlohmann_json::nlohmann_json
    httplib::httplib)