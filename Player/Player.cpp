#include "Player.h"




int main(int argc, char* argv[]) {
	//if (argc < 2) {
	//	std::cout << "Error, please supply a video file path argument.";
	//	return 1;
	//}
	//const std::string file_path = "C:\\Users\\bripl\\source\\repos\\MediaServer\\out\\build\\x64-debug\\Server\\QA Workshop Test Framework-20140129 1421-1.mp4";
	const std::string file_path = "C:\\Users\\bripl\\source\\repos\\MediaServer\\out\\build\\x64-debug\\Server\\VID-20240812-WA0001.mp4";
	VideoPlayer video_player;
	std::cout << "Playing selected file: " << file_path << std::endl;
	video_player.play_video(file_path);
	return 0;
}
