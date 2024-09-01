#include "Video.h"
#include <iostream>

Video::Video(const std::string& file)
	: file_path(file), audio_stream_index(-1), video_stream_index(-1), video_frame_height(0), video_frame_width(0) {
	if (!set_context()) {
		std::cout << "Unable to initiate resources necessary to play video!" << std::endl;
	};
	if (!set_codecs()) {
		std::cout << "Unable to set necessary codecs to play video!" << std::endl;
	};
	
};

decltype(buffer) Video::start() {
	// Vector to hold rgba frame data
	std::vector<uint8_t>* rgba_frame = new std::vector<std::uint8_t>(rgba_buff_size);

	 while(get_video_frame(rgba_frame)) {
		 std::cout << "Frame" << std::endl;
	}
	 std::cout << "Exiting video" << std::endl;
	 return buffer;
}

bool Video::set_context() {
	std::cout << "Setting video context..." << std::endl;
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
	std::cout << "Setting video codecs..." << std::endl;
	AVCodecParameters* video_codec_params = nullptr;
	const AVCodec* video_codec = nullptr;

	AVCodecParameters* audio_codec_params = nullptr;
	const AVCodec * audio_codec = nullptr;

	

	// Iterate through streams in context to get indexes of video and audio streams
	for (int i = 0; i < av_format_context->nb_streams; ++i) {
		// Get codec params from stream
		auto codec_params = av_format_context->streams[i]->codecpar;
		// Get codec for stream if it exists
		auto codec = avcodec_find_decoder(codec_params->codec_id);

		if (!codec) {
			std::cout << "A stream was identified but no appropriate codec was found!" << codec_params->codec_type << std::endl;
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
	rgba_buff_size = video_frame_width * video_frame_height * 4;
	
	return true;
}

bool Video::get_video_frame(std::vector<std::uint8_t>* rgba_frame) {

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
	
			av_packet_unref(video_packet);
			break;
		
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
	// Presentation time stamp in seconds. When the frame should show in sec
	//std::int64_t video_dts_seconds = video_packet->dts *  av_q2d(video_frame->time_base);
	//std::cout <<"Dts" <<  video_dts_seconds << std::endl;
	// Frame reformated into rgba frame. 4 bytes per pixel * total pixels is one rgba frame
	if (!scale_frame_data(rgba_frame)) {
		std::cout << "Unable to transform/scale frame!" << std::endl;
		return false;
	}
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