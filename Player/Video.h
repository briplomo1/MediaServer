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
#include <iostream>
#include <functional>
#include <mutex>
#include <queue>
#include <chrono>
#include <memory>

using buff = std::queue<std::vector<std::uint8_t>>;

class Video {

public:
	// The frame which should currently be displayed as specified by fps
	// Frame is dynamically updated to always display the frame that should currently be displayed.
	std::vector<std::uint8_t>* curr_frame;
	// Buffer size
	static const size_t buffer_size = 256;
	// Current index of buffer
	// Size of rbga pixel values array after conversion from yuv
	int rgba_frame_size;
	// Dimensions of video frame
	int video_frame_width;
	int video_frame_height;
	// Duration of video
	double duration;
	// Video fps
	double fps;
	// Audio sample rate
	double sps;
	// Audio bit depth/ bits per sample
	std::int64_t bit_depth;
	// Total frames in stream
	std::int64_t tot_frames;

private:
	// Define dual buffers which get swapped
	volatile std::shared_ptr<buff> read_buff = std::make_shared<buff>();
	volatile std::shared_ptr<buff> write_buff = std::make_shared<buff>();
	// Mutex to manage buffer access
	std::mutex buffer_mutex;
	// Path of valid video file to be played
	std::string file_path;
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
	void start();
	// Video destructor will deallocate resources by free_context
	~Video();
	
private:
	bool do_nothing(std::uint8_t* ch);
	// Allocate contexts necessary to define file properties and read file
	bool set_context();
	// Find and set a/v codec contexts and scaler if possible
	bool set_codecs();
	// Decode frames of video and populate given buffer until buffer is full or video ends
	bool populate_buffer(Video* video, std::shared_ptr<buff> write_buffer);
	// Set current frame to be displayed. Frame is set using a timer, the fps, and the frames' timstamps.
	void set_frame(std::vector<std::uint8_t>* frame, std::mutex& buffer_mutex,
		std::shared_ptr<buff> read_buff, std::shared_ptr<buff> write_buff, double fps);
	// Scale/transform pixel data of video frame into the displays expected format.
	bool scale_frame_data(std::vector<std::uint8_t>* data_out, AVFrame* frame);
	// Free resources allocated in set_context.
	void free_context();
};
