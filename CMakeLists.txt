cmake_minimum_required (VERSION 3.7)
include(GNUInstallDirs)
include(FindCURL)
include(FindBoost)

project (mastorss
         VERSION 0.2.6
         LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${MY_SOURCE_DIR}/src)
include_directories(${PROJECT_BINARY_DIR})

# Write version in header
configure_file (
    "${PROJECT_SOURCE_DIR}/src/version.hpp.in"
    "${PROJECT_BINARY_DIR}/version.hpp"
)

find_package(CURL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem)

file(GLOB sources src/*.cpp)
add_executable(mastorss ${sources})
target_link_libraries(mastorss mastodon-cpp ${Boost_LIBRARIES} ssl crypto ${CURL_LIBRARIES} curlpp)
install(TARGETS mastorss DESTINATION ${CMAKE_INSTALL_BINDIR})
