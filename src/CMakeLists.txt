include(GNUInstallDirs)

# The minimum versions should be in Debian oldstable, if possible.
find_package(Boost 1.62 REQUIRED COMPONENTS filesystem log regex)
find_package(PkgConfig REQUIRED)
pkg_check_modules(jsoncpp REQUIRED IMPORTED_TARGET jsoncpp)
find_package(Threads REQUIRED)
find_package(mastodonpp 0.5.6 REQUIRED CONFIG)

add_subdirectory(curl_wrapper)

# Write version in header.
configure_file ("${PROJECT_SOURCE_DIR}/src/version.hpp.in"
  "${PROJECT_BINARY_DIR}/version.hpp" @ONLY)

include_directories("${PROJECT_BINARY_DIR}")

file(GLOB sources *.cpp)
add_executable(mastorss ${sources})
target_link_libraries(mastorss
  PRIVATE
  PkgConfig::jsoncpp curl_wrapper mastodonpp::mastodonpp
  Boost::filesystem Boost::log Boost::regex)
if(BUILD_SHARED_LIBS)
  target_compile_definitions(mastorss PRIVATE "BOOST_ALL_DYN_LINK=1")
endif()
install(TARGETS mastorss DESTINATION "${CMAKE_INSTALL_BINDIR}")
