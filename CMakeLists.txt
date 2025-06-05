cmake_minimum_required(VERSION 3.21.7)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
project(steamfetcher LANGUAGES CXX)

set(SOURCE_FILES
    src/main.cpp
    src/steam/steam.cpp
    src/steam/handler.cpp
    src/steam/loader.cpp
    src/steam/process.cpp
    src/steam/prefix.cpp
    src/steam/utility.cpp
    src/steam/api_key.cpp
    src/steam/graph.cpp
    src/steam/undo.cpp
  )

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  LINKER_LANGUAGE CXX
)

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

target_compile_definitions(${PROJECT_NAME} PRIVATE 
                           CPPHTTPLIB_OPENSSL_SUPPORT
                          )

target_include_directories(${PROJECT_NAME} PRIVATE 
                           ${CMAKE_SOURCE_DIR}/include
                           ${CMAKE_SOURCE_DIR}/lib/laserpants/dotenv
                          )
                          
target_link_libraries(${PROJECT_NAME} PRIVATE
                      fmt::fmt-header-only
                      nlohmann_json::nlohmann_json
                      httplib::httplib
                      OpenSSL::SSL
                      OpenSSL::Crypto
                      winpthread
                     )
target_link_options(${PROJECT_NAME} 
  PRIVATE
    -static-libgcc
    -static-libstdc++
  )