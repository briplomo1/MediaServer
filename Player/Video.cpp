#include "Video.h"


Video::Video(const std::string& file)
	: file_path(file), audio_stream_index(-1), video_stream_index(-1),
	video_frame_height(0), video_frame_width(0), frame_count(0), bit_depth(0),
	fps(0), sps(0), duration(0), buffer(nullptr), rgba_frame_size(0), tot_frames(0){};

void Video::start() {

	if (!set_context()) {
		throw std::exception("Unable to initiate resources necessary to play video!");
	};
	if (!set_codecs()) {
		throw std::exception("Unable to set necessary codecs to play video!");
	};

	auto get_f = Video::get_video_frame;
	auto lambda = [&get_f](
		std::queue<std::vector<std::uint8_t>>* buff, decltype(max_buffer_size) b_size, int f_size,
		std::mutex& b_mutex) {
		
			for (;;) {
				b_mutex.lock();
				if (buff->size() < max_buffer_size) {
					std::vector<std::uint8_t> frame(f_size);
					if (!(get_f)(&frame)) {
						return;
					}
					buff->push(frame);
				}
				b_mutex.unlock();
			}
		};
	// Fill buffer in seperate thread and use a mutex to control buffer access.
	std::jthread load_video_T = std::jthread(lambda, &Video::get_video_frame, buffer, max_buffer_size, rgba_frame_size, std::ref(buffer_mutex));

	 std::cout << "Closing video..." << std::endl;
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
	// Allocate a frame to hold video datpulled from packet
	video_frame = av_frame_alloc();
	if (!video_frame) {
		std::cout << "Could not allocate video frame!" << std::endl;
		return false;
	}
	// Allocate a packet to hold video data
	video_packet = av_packet_alloc();
	if (!video_packet) {
		std::cout << "Could not allocate video packet!" << std::endl;
		return false;
	}

	audio_frame = av_frame_alloc();
	if (!audio_frame) {
		std::cout << "Could not allocate audio frame!" << std::endl;
		return false;
	}

	audio_packet = av_packet_alloc();
	if (!audio_packet) {
		std::cout << "Could not allocate audio packet!" << std::endl;
		return false;
	}

}

bool Video::set_codecs() {
	AVCodecParameters* video_codec_params = nullptr;
	const AVCodec* video_codec = nullptr;

	AVCodecParameters* audio_codec_params = nullptr;
	const AVCodec * audio_codec = nullptr;

	

	// Iterate through streams in context to get indexes of video and audio streams
	for (int i = 0; i < av_format_context->nb_streams; ++i) {
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

bool Video::get_video_frame(std::queue<std::vector<std::uint8_t>>* buffer, decltype(rgba_frame_size) frame_size) {

	// Packet may contain several frames
	while (av_read_frame(av_format_context, video_packet) >= 0) {

		int stream_index = video_packet->stream_index;
		// Filter out packets that arent video packets
		if (stream_index == video_stream_index) {
			
			// Decode video packet with video codec
			// Send packet to decoder
			int response = avcodec_send_packet(video_codec_context, video_packet);
		
			if (response < 0) {
				std::cout << "Failed to decode video packet!" << std::endl;
				return false;
			}
			video_frame->pts = frame_count++;
			// Receive frame from decoder and put it in our frame
			response = avcodec_receive_frame(video_codec_context, video_frame);
			if (response == AVERROR(EAGAIN)) {
				continue;
			}
			if (response == AVERROR_EOF) {
			std::cout << "End of video!" << std::endl;
			return false;
			}
			
			else if (response < 0) {
				std::cout << "Failed to decode packet!" << std::endl;
				return false;
			}
			std::vector<std::uint8_t> rgba_frame(frame_size);
			// Frame reformated into rgba frame. 4 bytes per pixel * total pixels is one rgba frame
			if (!scale_frame_data(&rgba_frame)) {
				std::cout << "Unable to transform/scale frame!" << std::endl;
				return false;
			}
			buffer->push(rgba_frame);
			
			av_packet_unref(video_packet);
		
		}
		else if (stream_index == audio_stream_index) {
			// Handle audio packet
			continue;
		}
		else {
			// Handle unkown stream packet
			continue;
		}
		
	}
	
	return true;
}

bool Video::scale_frame_data(std::vector<std::uint8_t>* data_out) {
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
	sws_scale(scaler_context, video_frame->data, video_frame->linesize, 0, video_frame->height, dest_buffer.data(), line_sizes);
	return true;
}

void Video::free_context() {
	// Free packets and frames
	av_frame_free(&video_frame);
	av_packet_free(&video_packet);
	av_frame_free(&audio_frame);
	av_packet_free(&audio_packet);
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