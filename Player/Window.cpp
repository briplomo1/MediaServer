#include "Window.h"


bool Window::init_window() {
	if (!glfwInit()) {
		cout << "Unable to init glfw!" << endl;
		return false;
	}
	// Create window with initial size
	window = glfwCreateWindow(1600, 1200, "Bryan's Video Player", NULL, NULL);
	if (!window) {
		cout << "Unable to open window!" << endl;
		return false;
	}
	// Setup window
	glfwMakeContextCurrent(window);
	
	// Generate texture names
	glGenTextures(1, &textures);
	// Bind texture to 2D target
	glBindTexture(GL_TEXTURE_2D, textures);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glfwGetFramebufferSize(window, &window_width, &window_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, window_width, window_height, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	return true;
}

bool Window::showVideo(Video* video) {

	
	// Show window refreshing as fast as possible
	while (!glfwWindowShouldClose(window)) {

		// Get next frame in buffer
		rgba_frame rgba_frame = *video->curr_frame;

		// Display 2D video with RGBA frame and video dimensions
		glBindTexture(GL_TEXTURE_2D, textures);
		// Populate texture data
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, video->video_frame_width, video->video_frame_height,
			0, GL_RGBA, GL_UNSIGNED_BYTE, rgba_frame.data());
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		// Map video to screen
		glTexCoord2d(0, 0); glVertex2i(200, 200);
		glTexCoord2d(1, 0); glVertex2i(200 + video->video_frame_width, 200);
		glTexCoord2d(1, 1); glVertex2i(200 + video->video_frame_width, 200 + video->video_frame_height);
		glTexCoord2d(0, 1); glVertex2i(200, 200 + video->video_frame_height);
		// Disable texture
		glEnd();
		glDisable(GL_TEXTURE_2D);
		
		// Swap window buffers
		glfwSwapBuffers(window);
		// Poll for user events
		glfwPollEvents();
	}
	cout << "Closed video window" << endl;
	video->stop();
	return true;
}
