include(GNUInstallDirs)

# The minimum versions should be in Debian oldstable, if possible.
find_package(Boost 1.62 REQUIRED COMPONENTS filesystem log)
find_package(jsoncpp REQUIRED CONFIG) # 1.7.4 (Debian buster) has no version.
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
  PRIVATE
  jsoncpp restclient-cpp mastodon-cpp::mastodon-cpp
  Boost::filesystem Boost::log)
if(BUILD_SHARED_LIBS)
  target_compile_definitions(mastorss PRIVATE "BOOST_ALL_DYN_LINK=1")
endif()
install(TARGETS mastorss DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(FILES watchwords.json
  DESTINATION "${CMAKE_INSTALL_DATADIR}/mastorss")
