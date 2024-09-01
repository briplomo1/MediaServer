#include "Window.h"

bool Window::showVideo(Video* video) {

	GLFWwindow* window;
	if (!glfwInit()) {
		std::cout << "Unable to init glfw!" << std::endl;
		return false;
	}

	window = glfwCreateWindow(1980, 1800, "Hello!", NULL, NULL);
	if (!window) {
		std::cout << "Unable to open window!" << std::endl;
		return false;
	}

	// Setup window
	glfwMakeContextCurrent(window);
	GLuint tex_handle;
	glGenTextures(1, &tex_handle);
	glBindTexture(GL_TEXTURE_2D, tex_handle);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// Vector to hold rgba frame data
	std::vector<uint8_t>* rgba_frame = new std::vector<std::uint8_t>(video->rgba_buff_size);
	//video->start();
	// Set window properties and show window
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		int window_width, window_height;
		glfwGetFramebufferSize(window, &window_width, &window_height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, window_width, window_height, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		

		// While there is frames left get frames
		if (!video->get_video_frame(rgba_frame)) {
			std::cout << "Unable to load video frame data!" << std::endl;
			return false;
		}

		// Display 2D video with RGBA frame and video dimensions
		glBindTexture(GL_TEXTURE_2D, tex_handle);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, video->video_frame_width, video->video_frame_height,
			0, GL_RGBA, GL_UNSIGNED_BYTE, (*rgba_frame).data());

		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		// Map video tp screen
		glTexCoord2d(0, 0); glVertex2i(200, 200);
		glTexCoord2d(1, 0); glVertex2i(200 + video->video_frame_width, 200);
		glTexCoord2d(1, 1); glVertex2i(200 + video->video_frame_width, 200 + video->video_frame_height);
		glTexCoord2d(0, 1); glVertex2i(200, 200 + video->video_frame_height);

		glEnd();
		glDisable(GL_TEXTURE_2D);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	std::cout << "Closed video window" << std::endl;
	return true;
}
