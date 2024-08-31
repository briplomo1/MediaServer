#include "Video.h"
#include <iostream>

Video::Video(const std::string& file)
	: file_path(file), 
	audio_stream_index(-1), video_stream_index(-1),
	video_frame_height(0), video_frame_width(0) {
	init_context();
	set_codecs();
};

bool Video::init_context() {
	av_format_context = avformat_alloc_context();
	if (!av_format_context) {
		std::cout << "Couldn't init format context" << std::endl;
		return false;
	}
	// Try to open file and populate context. Reads file header based on file type to get properties
	if (avformat_open_input(&av_format_context, file_path.c_str(), NULL, NULL) != 0) {
		std::cout << "Couldn't open video file" << std::endl;
		return false;
	}

}

void Video::set_codecs() {
	
	AVCodecParameters* video_codec_params;
	const AVCodec* video_codec;

	AVCodecParameters* audio_codec_params;
	const AVCodec* audio_codec;

	

	// Iterate through streams in context to get index of video and audio streams
	for (int i = 0; i < av_format_context->nb_streams; ++i) {
		// Get codec params from stream
		auto codec_params = av_format_context->streams[i]->codecpar;
		// Get codec for stream if it exists
		auto codec = avcodec_find_decoder(codec_params->codec_id);

		if (!codec) {
			std::cout << "Codec " << codec_params->codec_id << " not found for stream!" << std::endl;
			continue;
		}
		// If we find video stream, populate video codec properties
		if (codec->type == AVMEDIA_TYPE_VIDEO) {
			// std::cout << "Found video codec: " << codec->type << std::endl;
			video_stream_index = i;
			video_codec = codec;
			video_codec_params = codec_params;
			continue;
		}
		// If we find audio stream, populate audio codec properties
		else if (codec_params->codec_type == AVMEDIA_TYPE_AUDIO) {
			std::cout << "Found audio codec: " << codec->type << std::endl;
			audio_stream_index = i;
			audio_codec = codec;
			audio_codec_params = codec_params;
			continue;
		}
	}

	if (video_stream_index < 0) {
		std::cout << "Unable to find a valid video stream!" << std::endl;
		return;
	}
	if (audio_stream_index < 0) {
		std::cout << "unable to find a valid audio stream!" << std::endl;
		return;
	}
	
	// Create video codec context and use codec if possible
	video_codec_context = avcodec_alloc_context3(video_codec);

	if (!video_codec_context) {
		std::cout << "Unable to create video codec context!" << std::endl;
		return;
	}
	if (avcodec_parameters_to_context(video_codec_context, video_codec_params) < 0) {
		std::cout << "Unable to populate video codec context!" << std::endl;
		return;
	}
	if (avcodec_open2(video_codec_context, video_codec, NULL) < 0) {
		std::cout << "Couldn't initialize the video codec context to use the given video codec!" << std::endl;
		return;
	}
	// Create audio codec context and use codec if possible
	audio_codec_context = avcodec_alloc_context3(audio_codec);

	if (!audio_codec_context) {
		std::cout << "Unable to allocate audio codec context!" << std::endl;
		return;
	}
	if (avcodec_parameters_to_context(audio_codec_context, audio_codec_params) < 0) {
		std::cout << "Unable to populate audio codec context!" << std::endl;
		return;
	}
	if (avcodec_open2(audio_codec_context, audio_codec, NULL) < 0) {
		std::cout << "Couldn't initialize the audio codec context to use the given audio codec!" << std::endl;
		return;
	}

	// Set video frame dimensions
	video_frame_width = video_codec_context->width;
	video_frame_height = video_codec_context->height;

	return;
}

// Destructor frees context resources by calling libav native methods
Video::~Video() {
	avformat_close_input(&av_format_context);
	avformat_free_context(av_format_context);
	avcodec_free_context(&video_codec_context);
	avcodec_free_context(&audio_codec_context);
}