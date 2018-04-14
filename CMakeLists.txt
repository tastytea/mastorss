cmake_minimum_required (VERSION 3.7)
project (mastorss
         VERSION 0.5.2
         LANGUAGES CXX
)

include(GNUInstallDirs)
include(FindCURL)
include(FindBoost)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_BINARY_DIR})

# Write version in header
configure_file (
    "${PROJECT_SOURCE_DIR}/src/version.hpp.in"
    "${PROJECT_BINARY_DIR}/version.hpp"
)

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})
find_package(Boost REQUIRED COMPONENTS system filesystem)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})
add_definitions(${Boost_DEFINITIONS})

file(GLOB sources src/*.cpp)
add_executable(mastorss ${sources})
target_link_libraries(mastorss mastodon-cpp ${Boost_LIBRARIES} stdc++fs curl curlpp jsoncpp)
install(TARGETS mastorss DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES watchwords.json
        DESTINATION ${CMAKE_INSTALL_DATADIR}/mastorss)
