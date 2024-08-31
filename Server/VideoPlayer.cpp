#include "VideoPlayer.h"


void VideoPlayer::play_video(const std::string& file_path) {
	Video* video_context = new Video(file_path);
	Window* window = new Window();
	std::uint8_t* rgba_data;
	if (!decode_frame(video_context, &rgba_data)) {
		std::cout << "Video player unable to load frames!" << std::endl;
		return;
	};
	window->showWindow(rgba_data, video_context);

}
bool VideoPlayer::decode_frame(const Video* video_context, std::uint8_t* data_out[]) {
	std::cout << "Decoding frame..." << std::endl;
	// Try to get frame/packets
	// format context, stream_indexes, codec_context, 
	AVFrame* av_frame = av_frame_alloc();
	if (!av_frame) {
		std::cout << "Could not allocate frame!" << std::endl;
		return false;
	}

	AVPacket* av_packet = av_packet_alloc();
	if (!av_packet) {
		std::cout << "Could not allocate packet!" << std::endl;
		return false;
	}

	// Packet may contain several frames
	while (av_read_frame(video_context->av_format_context, av_packet) >= 0) {

		int stream_index = av_packet->stream_index;
		// Filter out packets that arent video packets
		if (stream_index == video_context->video_stream_index) {
			std::cout << "Found video stream...Getting codec..." << std::endl;
			// Decode video packet with video codec
			// Send packet to decoder
			int response = avcodec_send_packet(video_context->video_codec_context, av_packet);

			if (response < 0) {
				std::cout << "Failed to decode video packet!" << std::endl;
				return false;
			}
			// Receive frame from decoder and put it in our frame
			response = avcodec_receive_frame(video_context->video_codec_context, av_frame);
			if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
				continue;
			}
			else if (response < 0) {
				std::cout << "Failed to decode packet!" << std::endl;
				return false;
			}
			av_packet_unref(av_packet);
			break;
		}
		else if (stream_index == video_context->audio_stream_index) {
			continue;
		}
		else {
			continue;
		}
		
	}

	// Scaling context to transform data from YUV to RBG0
	SwsContext* sws_scaler_ctx = sws_getContext(av_frame->width, av_frame->height, video_context->video_codec_context->pix_fmt,
												av_frame->width, av_frame->height, AV_PIX_FMT_RGB0, 
												SWS_BILINEAR, NULL, NULL, NULL);

	if (!sws_scaler_ctx) {
		std::cout << "Could not initialize scaler context!" << std::endl;
	}
	

	// Vector holds rgb pixels. 4 bytes per pixel for R-G-B-A data
	int data_size = av_frame->width * av_frame->height * 4;
	std::uint8_t* data = new uint8_t[data_size];
	// Buffer to be populated by scaler
	std::uint8_t* dest_buffer[4] = {data, NULL, NULL, NULL};
	// Vector to describe line size of the data
	int line_sizes[4] = { av_frame->width * 4, 0, 0, 0 };
	// Scale/ reformat data into RGBA buffer
	sws_scale(sws_scaler_ctx, av_frame->data, av_frame->linesize, 0, av_frame->height, dest_buffer, line_sizes);
	// Free the scalar context
	sws_freeContext(sws_scaler_ctx);

	// Set decoded and reformatted data
	*data_out = data;

	// Free resources
	av_frame_free(&av_frame);
	av_packet_free(&av_packet);
}


