#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}
#include <inttypes.h>
#include <vector>
#include <array>
#include <string>
#include <iostream>
#include "Video.h"
#include "Window.h"


class VideoPlayer {

public:
	static void play_video(const string& file);

};
