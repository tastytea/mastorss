cmake_minimum_required (VERSION 3.7)
include(GNUInstallDirs)

project (rss2mastodon
         VERSION 0.2.0
         LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FindCURL)
find_package(CURL REQUIRED)

add_executable(rss2mastodon src/rss2mastodon.cpp src/http.cpp)
target_link_libraries(rss2mastodon mastodon-cpp boost_system boost_filesystem ssl crypto ${CURL_LIBRARIES} curlpp)
install(TARGETS rss2mastodon DESTINATION ${CMAKE_INSTALL_BINDIR})
