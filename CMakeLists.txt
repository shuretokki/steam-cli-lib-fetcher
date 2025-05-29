cmake_minimum_required(VERSION 3.21.7)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(steamf LANGUAGES CXX)

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

set(SOURCE_FILES
    src/main.cpp
    src/steam/steam.cpp
    src/steam/handler.cpp
    src/steam/loader.cpp
    src/steam/process.cpp
    src/steam/prefix.cpp
    src/steam/utility.cpp
    src/steam/api_key.cpp
  )

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  LINKER_LANGUAGE CXX
)

# --- Fetch FTXUI --------------------------------------------------------------
# include(FetchContent)

# set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
# set(FETCHCONTENT_QUIET OFF)

# FetchContent_Declare(ftxui
#   GIT_REPOSITORY https://github.com/arthursonzogni/ftxui.git
#   GIT_TAG        v5.0.0
#   GIT_PROGRESS   TRUE
#   GIT_SHALLOW    TRUE
#   EXCLUDE_FROM_ALL
# )
# FetchContent_MakeAvailable(ftxui)


# --- Fetch Vcpkg Manifest -----------------------------------------------------
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)

 #OpenSSL
find_package(OpenSSL REQUIRED)
target_compile_definitions(${PROJECT_NAME} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)




target_include_directories(${PROJECT_NAME} PRIVATE 
                           ${INCLUDE_DIR}
                           ${INCLUDE_DIR}/laserpants/dotenv
                          )

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    fmt::fmt-header-only
    nlohmann_json::nlohmann_json
    httplib::httplib
    OpenSSL::SSL
    OpenSSL::Crypto
    winpthread
    # ftxui::component
    # ftxui::dom
    # ftxui::screen
  )

target_link_options(${PROJECT_NAME} 
  PRIVATE
    -static-libgcc
    -static-libstdc++
  )