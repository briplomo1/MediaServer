#pragma once

#include "GLFW/glfw3.h"
#include "Video.h"
#include <iostream>
#include <inttypes.h>
#include <vector>

class Window {

public:
	GLFWwindow* window;
	//Open GL textures
	GLuint textures;
	// Window dimensions
	int window_width, window_height;

public:
	bool showVideo(Video* video);

	bool init_window();
};