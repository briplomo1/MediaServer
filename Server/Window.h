#pragma once

#include "GLFW/glfw3.h"
#include "Video.h"
#include <iostream>
#include <inttypes.h>
#include <vector>

class Window {

public:
	bool showWindow(std::uint8_t frame_data[], Video* video_context);
};