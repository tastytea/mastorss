cmake_minimum_required (VERSION 3.7)
include(GNUInstallDirs)

project (rss2mastodon
         VERSION 0.1.3
         LANGUAGES CXX
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")

add_executable(rss2mastodon src/rss2mastodon.cpp src/http.cpp)
target_link_libraries(rss2mastodon mastodon-cpp boost_system boost_filesystem ssl crypto pthread)
install(TARGETS rss2mastodon DESTINATION ${CMAKE_INSTALL_BINDIR})
