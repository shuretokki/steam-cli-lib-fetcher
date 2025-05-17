cmake_minimum_required(VERSION 3.21.7)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/Users/Administrator/vcpkg/scripts/buildsystems/vcpkg.cmake" 
        CACHE STRING "Vcpkg toolchain file")
endif()


if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-mingw-static" CACHE STRING "Vcpkg target triplet")
endif()

message(STATUS "Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "Vcpkg target triplet: ${VCPKG_TARGET_TRIPLET}")

project(steamlibfetcher)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCE_FILES
    src/main.cpp
    src/slf.cpp
)

set(CMAKE_EXE_LINKER_FLAGS "-lstdc++ -lgcc")

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
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE 
    -Wall 
    -Wextra 
    -Wpedantic 
    -Werror
    -fstack-protector-strong
    -D_FORTIFY_SOURCE=2
  )
  if(MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE
      -Wl,--nxcompat
      -Wl,--dynamicbase
      -mconsole  # Explicitly link as console application
    )
  endif()
else()
  target_compile_options(${PROJECT_NAME} PRIVATE 
    -Wall 
    -Wextra 
    -Wpedantic
  )
endif()

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/include)

message(STATUS "Building with ${CMAKE_CXX_COMPILER_ID} compiler")
message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    fmt::fmt
    nlohmann_json::nlohmann_json
    httplib::httplib)