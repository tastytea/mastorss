include(GNUInstallDirs)

# The minimum versions should be in Debian oldstable, if possible.
find_package(Boost 1.62 REQUIRED COMPONENTS system filesystem)
find_package(jsoncpp 1.7 REQUIRED CONFIG)
find_package(CURL 7.52 REQUIRED)
find_package(Threads REQUIRED)
find_package(restclient-cpp 0.5 REQUIRED CONFIG)
find_package(mastodon-cpp REQUIRED CONFIG)

# Write version in header.
configure_file (
  "${PROJECT_SOURCE_DIR}/src/version.hpp.in"
  "${PROJECT_BINARY_DIR}/version.hpp")

include_directories("${PROJECT_BINARY_DIR}")

file(GLOB sources *.cpp)
add_executable(mastorss ${sources})
target_link_libraries(mastorss
  mastodon-cpp::mastodon-cpp Boost::system Boost::filesystem restclient-cpp)
install(TARGETS mastorss DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(FILES watchwords.json
  DESTINATION "${CMAKE_INSTALL_DATADIR}/mastorss")
