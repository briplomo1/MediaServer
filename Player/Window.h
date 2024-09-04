#pragma once

#include <GLFW/glfw3.h>
#include "Video.h"
#include <iostream>
#include <inttypes.h>
#include <vector>

// A desktop window and its methids to opne a window
// and display a given video
class Window {

public:
	// A desktop window to be created with OpneGL that displays the video
	GLFWwindow* window;
	//Open GL textures
	GLuint textures;
	// Window dimensions
	int window_width, window_height;

public:
	// Takes a video instance after it has been initialized.
	// Calls video methods to start and stop playin of video
	bool showVideo(Video* video);
	// Iinitialize GLFW window properties
	bool init_window();
};