# CMakeList.txt : CMake project for Server, include source and define
# project specific logic here.
#
# Add source to this project's executable.


include(FetchContent)

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG 3.4
)

find_package( OpenGL REQUIRED )

include_directories(${OPENGL_INCLUDE_DIRS})

FetchContent_GetProperties(glfw)

if (NOT glfw_POPULATED)
	FetchContent_Populate(glfw)
	set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
	set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
    set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
    set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")

	add_subdirectory("${glfw_SOURCE_DIR}" "${glfw_BINARY_DIR}")
endif()

# Platform dependent libs
if(APPLE)
	list(APPEND EXTRA_LIBS "-framework OpenGL")
elseif(WIN32)
	list(APPEND EXTRA_LIBS "-lglu32 -lopengl32")
	set(CMAKE_EXE_LINKER_FLAGS "-std=gnu99 -static -static-libgcc -static-libstdc++ -mwindows")
else()
	list(APPEND EXTRA_LIBS "-lGl -lGLU -lX11")
endif()

add_executable (Server "Server.cpp" "Server.h")

target_link_libraries(Server glfw ${OPENGL_LIBRARIES})


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Server PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
