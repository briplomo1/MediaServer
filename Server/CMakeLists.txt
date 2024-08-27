# CMakeList.txt : CMake project for Server, include source and define
# project specific logic here.
#
add_subdirectory(lib)

include(FetchContent)
FetchContent_Declare(
	fmt
	GIT_REPOSITORY https://github.com/fmtlib/fmt.git
	GIT_TAG 11.0.2
)

FetchContent_MakeAvailable(fmt)

# Add source to this project's executable.
add_executable (Server "Server.cpp" "Server.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Server PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
