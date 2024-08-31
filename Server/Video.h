#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#include <string>

class Video {

public:
	int video_frame_width;
	int video_frame_height;
	int video_stream_index;
	int audio_stream_index;
	AVFormatContext* av_format_context;
	AVCodecContext* audio_codec_context;
	AVCodecContext* video_codec_context;
	
private:
	const std::string& file_path;

public:
	Video(const std::string& file);
	~Video();
	
private:
	bool init_context();
	void set_codecs();

};
