#include "Video.h"


Video::Video(const string& file)
	: file_path(file), audio_stream_index(-1), video_stream_index(-1),
	video_frame_height(0), video_frame_width(0), bit_depth(0),
	fps(0), sps(0), duration(0), rgba_frame_size(0), tot_frames(0),
	curr_frame(new vector<uint8_t>), write_buff(new queue<vector<uint8_t>>()), read_buff(new queue<vector<uint8_t>>()),
	read_buff_ptr(&read_buff), write_buff_ptr(&write_buff){};

void Video::set_frame(Video* v, stop_token stoken) {
	mutex& m = v->buffer_mutex;
	auto w_buff = v->write_buff_ptr;
	auto r_buff = v->read_buff_ptr;
	auto frame = v->curr_frame;
	auto fps = v->fps;

	// The desired time difference between frames in microseconds
	long long frame_diff_micros = (1000000 / fps);
	chrono::system_clock::time_point last_frame_timestamp = chrono::system_clock::now();
	const auto interval = chrono::microseconds(frame_diff_micros);
	while (!stoken.stop_requested()) {
		chrono::system_clock::time_point now = chrono::system_clock::now();
		chrono::duration elapsed = chrono::duration_cast<chrono::microseconds>(now - last_frame_timestamp);
		// If current buffer is empty, swap to second buffer. If both buffers empty wait and buffer!
		if ((*r_buff)->empty()) {
			// Perform swapping of buffers while locking buffers
			m.lock();
			auto temp = *r_buff;
			*r_buff = *w_buff;
			*w_buff = temp;
			m.unlock();

			// Future use condition variable to indicate full buffer and stop bufferin here
			if ((*r_buff)->empty()) {
				cout << "Buffering video 1s..." << endl;
				this_thread::sleep_for(chrono::milliseconds(1000));
			}
			cout << (*r_buff)->size() << endl;
		}
		else if (elapsed >= interval){
			// Get next frame and remove from queue .
			// Will lock frame while swapping the value
			v->frame_mutex.lock();
			*frame = (*r_buff)->front();
			v->frame_mutex.unlock();
			(*r_buff)->pop();
			last_frame_timestamp = chrono::system_clock::now();
		}
	}
	return;

}

bool Video::set_context() {
	// Allocate file context
	av_format_context = avformat_alloc_context();
	if (!av_format_context) {
		cout << "Could not allocate video format context!" << endl;
		return false;
	}
	// Try to open file and populate context. Reads file header based on file type to get properties
	if (avformat_open_input(&av_format_context, file_path.c_str(), 0, 0) != 0) {
		cout << "Could not open video file! Check path and file type." << endl;
		return false;
	}
	av_dump_format(av_format_context, 0, file_path.c_str(), 0);
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
			cout << "A stream was identified but no appropriate codec was found!" << codec_params->codec_type << endl;
			continue;
		}
		// If we find video stream, populate video properties
		if (codec->type == AVMEDIA_TYPE_VIDEO) {
			cout << "Found video source: " << codec->name << endl;
			video_stream_index = i;
			video_codec = codec;
			video_codec_params = codec_params;
			fps = av_q2d(stream->r_frame_rate);
			tot_frames = stream->nb_frames;
			duration = tot_frames / fps;
			cout << "\tFPS: " << fps << endl;
			cout << "\tDuration: " << duration << " secs" << endl;
			continue;
		}
		// If we find audio stream, populate audio properties
		else if (codec_params->codec_type == AVMEDIA_TYPE_AUDIO) {
			cout << "Found audio source: " << codec->name <<endl;
			audio_stream_index = i;
			audio_codec = codec;
			audio_codec_params = codec_params;
			sps = audio_codec_params->sample_rate;
			bit_depth = audio_codec_params->bit_rate;
			cout << "\tSample rate: " << sps << endl;
			cout << "\tBit depth: " << bit_depth << endl;
			continue;
		}
	}

	if (video_stream_index < 0) {
		cout << "Unable to find a valid video stream!" << endl;
		return false;
	}
	if (audio_stream_index < 0) {
		cout << "unable to find a valid audio stream!" << endl;
		return false;
	}
	
	// Create video codec context and use codec if possible
	video_codec_context = avcodec_alloc_context3(video_codec);

	if (!video_codec_context) {
		cout << "Unable to create video codec context!" << endl;
		return false;
	}
	if (avcodec_parameters_to_context(video_codec_context, video_codec_params) < 0) {
		cout << "Unable to populate video codec context!" << endl;
		return false;
	}
	if (avcodec_open2(video_codec_context, video_codec, NULL) < 0) {
		cout << "Couldn't initialize the video codec context to use the given video codec!" << endl;
		return false;
	}
	// Create audio codec context and use codec if possible
	audio_codec_context = avcodec_alloc_context3(audio_codec);

	if (!audio_codec_context) {
		cout << "Unable to allocate audio codec context!" << endl;
		return false;
	}
	if (avcodec_parameters_to_context(audio_codec_context, audio_codec_params) < 0) {
		cout << "Unable to populate audio codec context!" << endl;
		return false;
	}
	if (avcodec_open2(audio_codec_context, audio_codec, NULL) < 0) {
		cout << "Couldn't initialize the audio codec context to use the given audio codec!" << endl;
		return false;
	}

	// Set video frame dimensions
	video_frame_width = video_codec_context->width;
	video_frame_height = video_codec_context->height;
	// Set the anticipated size of the frame once converted to rgba buffer
	rgba_frame_size = video_frame_width * video_frame_height * 4;

	return true;
}

rgba_frame Video::get_frame() {
	// Lock frame  and read out
	frame_mutex.lock();
	auto frame = *curr_frame;
	frame_mutex.unlock();
	return frame;
}

bool Video::populate_buffer(Video* this_vid, stop_token stoken) {
	mutex& m = this_vid->buffer_mutex;
	auto sz = this_vid->buffer_size;
	auto write_buffer = this_vid->write_buff_ptr;
	
	// Allocate a frame to hold encoded video data pulled from packet
	AVFrame* v_frame = av_frame_alloc();
	if (!v_frame) {
		cout << "Could not allocate video frame!" << endl;
		return false;
	}

	// Video packet holds compressed frames
	// Typically video packet holds one compressed frame
	AVPacket* v_pckt = av_packet_alloc();
	if (!v_pckt) {
		cout << "Could not allocate video packet!" << endl;
		return false;
	}

	// While stop token stop has not been called will load frames into buffer
	while(!stoken.stop_requested()) {
		// If t
		m.lock();
		auto is_full = (*write_buffer)->size() >= sz;
		m.unlock();
		if (!is_full) {
			if (!(av_read_frame(this_vid->av_format_context, v_pckt) >= 0)) {
				cout << "Could not read packet!" << endl;
				return false;
			}
			// Found a video packet. Decode and push frame into buffer
			if (v_pckt->stream_index == this_vid->video_stream_index) {
				
				// Decode video packet with codec
				int res = avcodec_send_packet(this_vid->video_codec_context, v_pckt);
				if (res < 0) {
					cout << "Failed to decode video packet!" << endl;
					return false;
				}
				// Receive frame from decoder and put it in our frame
				res = avcodec_receive_frame(video_codec_context, v_frame);
				if (res == AVERROR(EAGAIN)) {
					continue;
				}
				if (res == AVERROR_EOF) {
					cout << "End of video!" << endl;
					return false;
				}
				else if (res < 0) {
					cout << "Failed to decode packet!" << endl;
					return false;
				}
				// Create vector to hold a single frames data transformewd to rgba format
				rgba_frame frame_data(this_vid->rgba_frame_size);
				// Reshape frame data into rgba
				if (!this_vid->scale_frame_data(&frame_data, v_frame)) {
					cout << "Unable to transform/scale frame!" << endl;
					return false;
				}
				m.lock();
				(*write_buffer)->push(frame_data);
				m.unlock();
				
				// Dereference packet for next iteration
				av_packet_unref(v_pckt);
			}

		}
		
	}
	av_frame_free(&v_frame);
	av_packet_free(&v_pckt);
	cout << "Stop loading video......." << endl;
	return true;
}

bool Video::scale_frame_data(rgba_frame* data_out, AVFrame* frame) {
	// Scaling context to transform  or scale data  in this case from YUV to RBGA.
	// Initialize if it has not already. Cannot be intialized prior as it needs data from a frame.
	//TODO:  May be slow and will need to move to using hardware acceleration with GPU
	if (!scaler_context) {
		scaler_context = sws_getContext(video_frame_width, video_frame_height, video_codec_context->pix_fmt,
										video_frame_width, video_frame_height, AV_PIX_FMT_RGB0,
										SWS_BILINEAR, 0, 0, 0);
		if (!scaler_context) {
			cout << "Could not initialize scaler context!" << endl;
			return false;
		}

	}
	// Reshape pixels buffer to contain 4 pixels per color(RGBA) * totalpixels.
	array<uint8_t*, 4> dest_buffer{(*data_out).data(), 0, 0, 0};
	// Vector to describe line size of the data
	int line_sizes[4] = { video_frame_width * 4, 0, 0, 0 };
	// Scale / reformat data into RGBA data buffer
	sws_scale(scaler_context, frame->data, frame->linesize, 0, frame->height, dest_buffer.data(), line_sizes);
	return true;
}

void Video::stop() {
	// Call stop on threads causing threads to return
	source.request_stop();
}

void Video::start() {
	// Initialize all video props
	if (!set_context()) {
		throw exception("Unable to initiate resources necessary to play video!");
	};
	if (!set_codecs()) {
		throw exception("Unable to set necessary codecs to play video!");
	};

	// Get stop token from stop source
	stop_token stoken = source.get_token();

	// Init stop token callback
	stop_callback callback(stoken, []() {
		cout << "Video threads stop called!..." << endl;
	});

	// Define member func signature to pass to threads
	bool(Video:: * p_buff)(Video*, stop_token) = &Video::populate_buffer;
	void(Video:: * set_f)(Video*, stop_token) = &Video::set_frame;
	// Start writing and reading threads
	// Threads take a stop token and are detached so they can coninue running and
	// can be stopped by class instance in stop method
	thread write(p_buff, this, this, stoken);
	thread read(set_f, this, this, stoken);
	write.detach();
	read.detach();
}

void Video::free_context() {
	// Close file
	avformat_close_input(&av_format_context);
	// Free format, codecs, and scaler contexts
	avformat_free_context(av_format_context);
	avcodec_free_context(&video_codec_context);
	avcodec_free_context(&audio_codec_context);
	sws_freeContext(scaler_context);
}

Video::~Video() {
	source.request_stop();
	free_context();
}