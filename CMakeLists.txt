cmake_minimum_required (VERSION 3.7)
include(GNUInstallDirs)

project (mastorss
         VERSION 0.2.3
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

include(FindCURL)
find_package(CURL REQUIRED)

add_executable(mastorss src/mastorss.cpp src/http.cpp src/config.cpp src/parse.cpp)
target_link_libraries(mastorss mastodon-cpp boost_system boost_filesystem ssl crypto ${CURL_LIBRARIES} curlpp)
install(TARGETS mastorss DESTINATION ${CMAKE_INSTALL_BINDIR})
