#include "VideoPlayer.h"


void VideoPlayer::play_video(const string& file_path) {
	// Create window and video
	Video* video = new Video(file_path);
	Window* window = new Window();
	// Initialize window props
	window->init_window();
	video->start();
	// Start loading video and display video on window
	window->showVideo(video);
}



