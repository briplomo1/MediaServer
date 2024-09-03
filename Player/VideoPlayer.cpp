#include "VideoPlayer.h"


void VideoPlayer::play_video(const std::string& file_path) {
	// Create window and video
	Video* video = new Video(file_path);
	Window* window = new Window();
	window->init_window();
	// Display video on window
	if (!window->showVideo(video)) {
		std::cout << "Video closed." << std::endl;
	};
}



