// Server.cpp : Defines the entry point for the application.
//

#include "Server.h"
#include <stdio.h>
#include<GLFW/glfw3.h>

using namespace std;

int main(int argc, const char** argv) {
	cout << "Hello CMake." << endl;
	GLFWwindow* window;
	if (!glfwInit()) {
		printf("Unable to init glfw!\n");
		return 1;
	}
	
	window = glfwCreateWindow(640, 480, "Hello!", NULL, NULL);
	if (!window) {
		printf("Unable to open window!\n");
		return 1;
	}

	glfwMakeContextCurrent(window);

	while (!glfwWindowShouldClose(window)) {
		glfwWaitEvents();
	}
	return 0;
}
