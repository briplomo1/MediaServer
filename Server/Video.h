#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}
#include <string>
#include <vector>
#include <array>
#include <thread>

class Video {

public:
	// Frame buffer holds frames to be processed
	std::array<std::vector<std::uint8_t>, 100>* buffer;
	// Size of rbga pixel values array after conversion from yuv
	int rgba_buff_size;
	// Dimensions of video frame
	int video_frame_width;
	int video_frame_height;
	// Duration of video
	double duration;

private:
	// Thread to manage reading of file to buffers

	
	// Path of valid video file to be played
	std::string file_path;
	// Allocated video frame data
	AVFrame* video_frame = nullptr;
	// Allcated audio frame
	AVFrame* audio_frame = nullptr;
	// Video packet holds compressed frames
	// Typically video packet holds one compressed frame
	AVPacket* video_packet = nullptr;
	// Audio packet holds compressed audio frames
	// One packet contains several frames depending on the formattting
	AVPacket* audio_packet = nullptr;
	// Index of the video stream from available streams in file header
	int video_stream_index;
	// Index of the audio stream from available streams in file header
	int audio_stream_index;
	// AVlib context holds context about the file opened
	AVFormatContext* av_format_context = nullptr;
	// Audio context holds properties about the audio stream codec if found
	AVCodecContext* audio_codec_context = nullptr;
	// Video context holds properties about the video stream codec if found
	AVCodecContext* video_codec_context = nullptr;
	// Scaling/transfomrative operations to be conducted on video frames
	SwsContext* scaler_context = nullptr;
	
	
public:
	// Constructor initializes video from given video file
	Video(const std::string& file);
	// Starts populating buffer with frames and returns pointer to buffer
	decltype(buffer) start();
	// Read and decode a single frame of the video
	bool get_video_frame(std::vector<std::uint8_t>* data_out);
	// Video destructor will deallocate resources by free_context
	~Video();
	
private:
	// Allocate contexts necessary to define file properties and read file
	bool set_context();
	// Find and set a/v codec contexts and scaler if possible
	bool set_codecs();
	// Scale/transform pixel data of video frame into the displays expected format
	bool scale_frame_data(std::vector<std::uint8_t>* data_out);
	// Free resources allocated in set_context
	void free_context();
};
