# CMakeList.txt : CMake project for Server, include source and define
# project specific logic here.
#
# Add source to this project's executable.
add_executable (Server "Server.cpp" "Server.h")

include(FetchContent)

FetchContent_Declare(
	fmt
	GIT_REPOSITORY https://github.com/fmtlib/fmt.git
	GIT_TAG 11.0.2
)

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG 3.4
)

FetchContent_MakeAvailable(fmt glfw)

target_link_libraries(Server glfw)
target_link_libraries(Server fmt)



if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Server PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
