cmake_minimum_required (VERSION 3.6)
project (mastorss
         VERSION 0.8.5
         LANGUAGES CXX
)

include(GNUInstallDirs)
find_package(CURL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURLPP REQUIRED curlpp)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_BINARY_DIR})

include_directories(${CURL_INCLUDE_DIR})
include_directories(${CURLPP_INCLUDE_DIR})
include_directories(${JSONCPP_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})

link_directories(${CURL_LIBRARY_DIRS})
link_directories(${CURLPP_LIBRARY_DIRS})
link_directories(${JSONCPP_LIBRARY_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

add_definitions(${Boost_DEFINITIONS})

# Write version in header
configure_file (
    "${PROJECT_SOURCE_DIR}/src/version.hpp.in"
    "${PROJECT_BINARY_DIR}/version.hpp"
)

file(GLOB sources src/*.cpp)
add_executable(mastorss ${sources})
target_link_libraries(mastorss
                      mastodon-cpp ${CURLPP_LIBRARIES}
                      ${JSONCPP_LIBRARIES} ${Boost_LIBRARIES} stdc++fs)
install(TARGETS mastorss DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES watchwords.json
        DESTINATION ${CMAKE_INSTALL_DATADIR}/mastorss)
