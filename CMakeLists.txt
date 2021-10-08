cmake_minimum_required(VERSION 3.11)
include(FetchContent)

project(splat VERSION 2.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-O3 -Wall -ffast-math")
set(CMAKE_C_FLAGS "-O3 -Wall -ffast-math")	# required for optimization of itwom3.c until it is ported to C++

if(WIN32)
  add_definitions(-DHAVE_LIBPNG)
  set(SHAREDIR ".")
  set(DOCDIR "docs")
elseif(APPLE)
  add_definitions(-DHAVE_LIBPNG -DHAVE_LIBJPEG -DHAVE_LIBGDAL)
  set(SHAREDIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}")
  set(DOCDIR "share/docs/${PROJECT_NAME}")
  set(MANDIR "share/man")
  # See https://stackoverflow.com/questions/36523911/osx-homebrew-cmake-libpng-version-mismatch-issue
  set(CMAKE_FIND_FRAMEWORK LAST)
elseif(UNIX) # Linux, BSD etc
  add_definitions(-DHAVE_LIBPNG -DHAVE_LIBJPEG -DHAVE_LIBGDAL)
  set(SHAREDIR "share/${PROJECT_NAME}")
  set(DOCDIR "share/docs/${PROJECT_NAME}")
  set(MANDIR "share/man")
endif()

FetchContent_Declare(cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG v2.1.1
)

FetchContent_GetProperties(cli11)
if(NOT cli11_POPULATED)
  FetchContent_Populate(cli11)

  # Set any custom variables, etc. here, then
  # populate the content as part of this build

  add_subdirectory(${cli11_SOURCE_DIR} ${cli11_BINARY_DIR} EXCLUDE_FROM_ALL)
  # include_directories(${cli11_SOURCE_DIR}/include)
endif()

add_subdirectory(src)
add_subdirectory(utils)
add_subdirectory(docs)

install(DIRECTORY sample_data DESTINATION "${SHAREDIR}")
