#include "VideoPlayer.h"

int main(int argc, char* argv[]) {
	//if (argc < 2) {
	//	cout << "Error, please supply a video file path argument.";
	//	return 1;
	//}
	
	const string file_path = "C:/Users/bripl/source/repos/MediaServer/out/build/x64-debug/Server/VID-20240812-WA0001.mp4";
	VideoPlayer video_player;
	cout << "Playing selected file: " << file_path << endl;
	video_player.play_video(file_path);
	return 0;
}
