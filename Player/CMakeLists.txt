# CMakeList.txt : CMake project for Server, include source and define
# project specific logic here.
#
# Add source to this project's executable.


include(FetchContent)

include(CMakePrintHelpers)

# Need openGL
find_package( OpenGL REQUIRED )

include_directories(${OPENGL_INCLUDE_DIRS})

add_executable (Player "Player.cpp" "Player.h" "Window.cpp" "VideoPlayer.cpp" "VideoPlayer.h" "Window.h" "Video.cpp" "Video.h" )

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG 3.4
)

FetchContent_Declare(
	ffmpeg
	URL https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-full-shared.7z
	URL_HASH SHA256=a0b6e8c7978b95d019a93dcf4b4ab74b17d9e53e0a87cfd463e1376c5927e30b
)

FetchContent_GetProperties(glfw)

# Populate glfw
if (NOT glfw_POPULATED)
	FetchContent_Populate(glfw)
	set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
	set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
    set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
    set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")

	add_subdirectory("${glfw_SOURCE_DIR}" "${glfw_BINARY_DIR}")
endif()

# Populate ffmpeg
if (NOT ffmpeg_POPULATED)
	FetchContent_Populate(ffmpeg)
	set(FFMPEG_SRC ${ffmpeg_SOURCE_DIR})
endif()

cmake_print_variables(FETCHCONTENT_BASE_DIR)
cmake_print_variables(CMAKE_CURRENT_BINARY_DIR)

# Move ffmpeg's dlls to project binary dir
add_custom_command(
TARGET Player POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
${FETCHCONTENT_BASE_DIR}/ffmpeg-src/bin
${CMAKE_CURRENT_BINARY_DIR}
)



if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Player PROPERTY CXX_STANDARD 20)
endif()

# Include 
target_include_directories(Player PUBLIC
	${FFMPEG_SRC}/include
)
target_link_directories(Player PUBLIC
	${FFMPEG_SRC}/lib
)
target_link_libraries(Player glfw ${OPENGL_LIBRARIES}
	avdevice avformat avcodec avutil swscale swresample
)





# TODO: Add tests and install targets if needed.
