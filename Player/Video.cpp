#include "Video.h"


Video::Video(const std::string& file)
	: file_path(file), audio_stream_index(-1), video_stream_index(-1),
	video_frame_height(0), video_frame_width(0), bit_depth(0),
	fps(0), sps(0), duration(0), rgba_frame_size(0), tot_frames(0),
	curr_frame(new std::vector<uint8_t>){};


bool Video::do_nothing(std::uint8_t* ch) {
	return true;
}

void Video::set_frame(std::vector<std::uint8_t>* frame, 
	std::mutex& m, std::shared_ptr<buff> r_buff, std::shared_ptr<buff> w_buff, double fps) {

	int fps_ms = (1 / fps) * 1000;
	const auto interval = std::chrono::milliseconds(fps_ms);
	while (1) {
		std::cout << "read in read ----------------------------------------------------------------- " << r_buff << std::endl;
		// If queue is empty, swap buffers. If both buffers empty throw error!
		if (r_buff->empty()) {
			m.lock();
			std::cout << "Swap: "<< r_buff << std::endl;
			w_buff.swap(r_buff);
			std::cout << r_buff->size() << std::endl;
			if (r_buff->empty()) {
				std::cout << "Both buffers are empty! Unable to read any data!" << std::endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}
			std::cout << r_buff->size() << std::endl;
			std::cout << "After swap: " << r_buff  << "new budd size: " << r_buff->size() << std::endl;
			m.unlock();
		}
		else {
			// Get next frame and remove from queue
			*frame = r_buff->front();
			r_buff->pop();
			std::cout << r_buff->size() << std::endl;
			std::this_thread::sleep_for(interval);
		}
	}
	return;

}

bool Video::set_context() {
	// Allocate file context
	av_format_context = avformat_alloc_context();
	if (!av_format_context) {
		std::cout << "Could not allocate video format context!" << std::endl;
		return false;
	}
	// Try to open file and populate context. Reads file header based on file type to get properties
	if (avformat_open_input(&av_format_context, file_path.c_str(), 0, 0) != 0) {
		std::cout << "Could not open video file! Check path and file type." << std::endl;
		return false;
	}
	return true;
}

bool Video::set_codecs() {
	AVCodecParameters* video_codec_params = nullptr;
	const AVCodec* video_codec = nullptr;

	AVCodecParameters* audio_codec_params = nullptr;
	const AVCodec * audio_codec = nullptr;

	

	// Iterate through streams in context to get indexes of video and audio streams
	for (unsigned int i = 0; i < av_format_context->nb_streams; ++i) {
		// Get codec params from stream
		auto stream = av_format_context->streams[i];
		auto codec_params = stream->codecpar;
		// Get codec for stream if it exists
		auto codec = avcodec_find_decoder(codec_params->codec_id);

		if (!codec) {
			std::cout << "A stream was identified but no appropriate codec was found!" << codec_params->codec_type << std::endl;
			continue;
		}
		// If we find video stream, populate video properties
		if (codec->type == AVMEDIA_TYPE_VIDEO) {
			std::cout << "Found video source: " << codec->name << std::endl;
			video_stream_index = i;
			video_codec = codec;
			video_codec_params = codec_params;
			fps = av_q2d(stream->avg_frame_rate);
			tot_frames = stream->nb_frames;
			duration = tot_frames / fps;
			std::cout << "\tFPS: " << fps << std::endl;
			std::cout << "\tDuration: " << duration << " secs" << std::endl;
			continue;
		}
		// If we find audio stream, populate audio properties
		else if (codec_params->codec_type == AVMEDIA_TYPE_AUDIO) {
			std::cout << "Found audio source: " << codec->name << std::endl;
			audio_stream_index = i;
			audio_codec = codec;
			audio_codec_params = codec_params;
			sps = audio_codec_params->sample_rate;
			bit_depth = audio_codec_params->bit_rate;
			std::cout << "\tSample rate: " << sps << std::endl;
			std::cout << "\tBit depth: " << bit_depth << std::endl;
			continue;
		}
	}

	if (video_stream_index < 0) {
		std::cout << "Unable to find a valid video stream!" << std::endl;
		return false;
	}
	if (audio_stream_index < 0) {
		std::cout << "unable to find a valid audio stream!" << std::endl;
		return false;
	}
	
	// Create video codec context and use codec if possible
	video_codec_context = avcodec_alloc_context3(video_codec);

	if (!video_codec_context) {
		std::cout << "Unable to create video codec context!" << std::endl;
		return false;
	}
	if (avcodec_parameters_to_context(video_codec_context, video_codec_params) < 0) {
		std::cout << "Unable to populate video codec context!" << std::endl;
		return false;
	}
	if (avcodec_open2(video_codec_context, video_codec, NULL) < 0) {
		std::cout << "Couldn't initialize the video codec context to use the given video codec!" << std::endl;
		return false;
	}
	// Create audio codec context and use codec if possible
	audio_codec_context = avcodec_alloc_context3(audio_codec);

	if (!audio_codec_context) {
		std::cout << "Unable to allocate audio codec context!" << std::endl;
		return false;
	}
	if (avcodec_parameters_to_context(audio_codec_context, audio_codec_params) < 0) {
		std::cout << "Unable to populate audio codec context!" << std::endl;
		return false;
	}
	if (avcodec_open2(audio_codec_context, audio_codec, NULL) < 0) {
		std::cout << "Couldn't initialize the audio codec context to use the given audio codec!" << std::endl;
		return false;
	}

	// Set video frame dimensions
	video_frame_width = video_codec_context->width;
	video_frame_height = video_codec_context->height;
	// Set the anticipated size of the frame once converted to rgba buffer
	rgba_frame_size = video_frame_width * video_frame_height * 4;
	
	return true;
}

bool Video::populate_buffer(Video* this_vid, std::shared_ptr<buff> write_buffer) {
	std::mutex& m = this_vid->buffer_mutex;
	auto sz = this_vid->buffer_size;
	
	// Allocate a frame to hold encoded video data pulled from packet
	AVFrame* v_frame = av_frame_alloc();
	if (!v_frame) {
		std::cout << "Could not allocate video frame!" << std::endl;
		return false;
	}

	// Video packet holds compressed frames
	// Typically video packet holds one compressed frame
	AVPacket* v_pckt = av_packet_alloc();
	if (!v_pckt) {
		std::cout << "Could not allocate video packet!" << std::endl;
		return false;
	}

	// lambda checks buffer is not full while blocking with mutex to ensure size is reliable
	auto buff_is_full = [&write_buffer, &m, sz]() {
		m.lock();
		std::cout << "Buff: " << write_buffer << std::endl;
		bool full = !(write_buffer->size() < sz);
		m.unlock();
		return full;
		};

	for (;;) {
		//std::cout << "write" << std::endl;
		// If the buffer is not full. Add frames to buffer
		//std::cout << "write in write " << write_buffer << std::endl;
		if (!buff_is_full()) {
			if (!(av_read_frame(this_vid->av_format_context, v_pckt) >= 0)) {
				std::cout << "Could not read packet!" << std::endl;
				return false;
			}
			// Found a video packet. Decode and push frame into buffer
			if (v_pckt->stream_index == this_vid->video_stream_index) {
				
				// Decode video packet with codec
				int res = avcodec_send_packet(this_vid->video_codec_context, v_pckt);
				if (res < 0) {
					std::cout << "Failed to decode video packet!" << std::endl;
					return false;
				}
				// Receive frame from decoder and put it in our frame
				res = avcodec_receive_frame(video_codec_context, v_frame);
				if (res == AVERROR(EAGAIN)) {
					continue;
				}
				if (res == AVERROR_EOF) {
					std::cout << "End of video!" << std::endl;
					return false;
				}
				else if (res < 0) {
					std::cout << "Failed to decode packet!" << std::endl;
					return false;
				}
				// Create vector to hold a single frames data transformewd to rgba format
				std::vector<std::uint8_t> frame_data(this_vid->rgba_frame_size);
				// Reshape frame data into rgba
				if (!this_vid->scale_frame_data(&frame_data, v_frame)) {
					std::cout << "Unable to transform/scale frame!" << std::endl;
					return false;
				}
				m.lock();
				write_buffer->push(frame_data);
				m.unlock();
				av_packet_unref(v_pckt);
			}

		} else{
			std::cout << "full" << std::endl;
		}
		
	}
	std::cout << "Ending video..." << std::endl;
	return true;
}

bool Video::scale_frame_data(std::vector<std::uint8_t>* data_out, AVFrame* frame) {
	// Scaling context to transform  or scale data  in this case from YUV to RBGA.
	// Initialize if it has not already. Cannot be intialized prior as it needs data from a frame.
	// May be slow and will need to move to using hardware acceleration with GPU
	if (!scaler_context) {
		scaler_context = sws_getContext(video_frame_width, video_frame_height, video_codec_context->pix_fmt,
										video_frame_width, video_frame_height, AV_PIX_FMT_RGB0,
										SWS_BILINEAR, 0, 0, 0);
		if (!scaler_context) {
			std::cout << "Could not initialize scaler context!" << std::endl;
			return false;
		}

	}
	// Reshape pixels buffer to contain 4 pixels per color(RGBA) * totalpixels.
	std::array<uint8_t*, 4> dest_buffer{(*data_out).data(), 0, 0, 0};
	// Vector to describe line size of the data
	int line_sizes[4] = { video_frame_width * 4, 0, 0, 0 };
	// Scale / reformat data into RGBA data buffer
	sws_scale(scaler_context, frame->data, frame->linesize, 0, frame->height, dest_buffer.data(), line_sizes);
	return true;
}

void Video::start() {

	if (!set_context()) {
		throw std::exception("Unable to initiate resources necessary to play video!");
	};
	if (!set_codecs()) {
		throw std::exception("Unable to set necessary codecs to play video!");
	};

	// Define memb funcs to pasws to threads
	bool(Video:: * p_buff)(Video*, std::shared_ptr<buff>) = &Video::populate_buffer;
	void(Video:: * set_f)(std::vector<std::uint8_t>*, std::mutex&,
		std::shared_ptr<buff>, std::shared_ptr<buff>, double) = &Video::set_frame;

	// Init thread to decode and load frames into buffer.
	std::jthread write(p_buff, this, this, write_buff);
	// Init thread to populate current frame dynamically at the frame rate using timer.
	std::jthread read(set_f, this, curr_frame, std::ref(buffer_mutex), read_buff, write_buff, fps);

	std::cout << "Closing video...----------------------------------------------------" << std::endl;
	return;
}

void Video::free_context() {
	// Close file
	avformat_close_input(&av_format_context);
	// Free contexts
	avformat_free_context(av_format_context);
	avcodec_free_context(&video_codec_context);
	avcodec_free_context(&audio_codec_context);
	sws_freeContext(scaler_context);
}

Video::~Video() {
	free_context();
}