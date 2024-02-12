#include "ffplayer.h"

FFPlayer::FFPlayer()
{}

FFPlayer::~FFPlayer()
{
	
	stop();
	if(fmt_ctx_) {
		avformat_close_input(&fmt_ctx_);
	}


}

int FFPlayer::create() {
	int ret = 0;
	
	return ret;
}

void FFPlayer::notify(int what) {
	Msg msg;
	msg.what_ = what;
	msg_queue_.push(msg);
}

int FFPlayer::prepareAsync() {
	int ret = 0;
	if(url_.empty()) {
		LOG_INFO("url is empty");
		return -1;
	}
	if(avformat_open_input(&fmt_ctx_, url_.c_str(), NULL, NULL) < 0) {
		LOG_ERROR("avformat_open_input failed");
		return -1;
	}
	video_stream_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, (const AVCodec**)&video_codec_, 0);
	if(video_stream_index_ < 0) {
		LOG_ERROR("av_find_best_stream failed");
		return -1;
	}
	audio_stream_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1,(const AVCodec**) & audio_codec_, 0);
	if(audio_stream_index_ < 0) {
		LOG_ERROR("av_find_best_stream failed");
		return -1;
	}
	

	audio_decoder_ = new Decoder(fmt_ctx_, AVMEDIA_TYPE_AUDIO, &audio_packet_queue_);
	video_decoder_ = new Decoder(fmt_ctx_, AVMEDIA_TYPE_VIDEO, &video_packet_queue_);

	auto codec_ctx = audio_decoder_->getCodecCtx();

	AVStream* st = fmt_ctx_->streams[audio_stream_index_];
	swr_ = new SWSResampler();
	swr_->setOpts(codec_ctx->ch_layout, codec_ctx->ch_layout, codec_ctx->sample_rate, codec_ctx->sample_rate, codec_ctx->sample_fmt, (AVSampleFormat)AV_SAMPLE_FMT_FLT);

	sdl_player_ = new SDLPlayer();
	sdl_player_->openAudio(codec_ctx);


	notify( MSG_PREPARED);
	state_ = FFState::Prepared;
	return ret;
}

int FFPlayer::start() {
	if(state_ != FFState::Prepared) {
		LOG_ERROR("state is not prepared");
		return -1;
	}
	int ret = 0;
	read_thread_ = new std::thread(&FFPlayer::readPacketLoop, this);
	
	audio_decode_thread_ = new std::thread(std::bind(&FFPlayer::decodePacketLoop, this, 1));
	video_decode_thread_ = new std::thread(std::bind(&FFPlayer::decodePacketLoop, this, 0));

	sdl_player_->playAudio();

	notify( MSG_STARTED);
	state_ = FFState::Playing;
	return ret;
}

int FFPlayer::setDataSource(const std::string& url) {
	url_ = url;
	state_ = FFState::Ready;
	notify(MSG_READY);
	return 0;
}

int FFPlayer::tryGetMsg(Msg& msg) {
	int ret = 0;
	msg = msg_queue_.try_pop();
	
	return ret;

}
int FFPlayer::blockGetMsg(Msg& msg) {
	int ret = 0;
	msg = msg_queue_.wait_and_pop();
	
	return ret;
}

void FFPlayer::readPacketLoop() {
	int ret = 0;
	
	while(1) {
		mtx_.lock();
		if(state_ == FFState::Stopped) {
			mtx_.unlock();
			return;
		}
		mtx_.unlock();

		std::shared_ptr<Packet> pkt_ptr = std::make_shared<Packet>();
		AVPacket* pkt = pkt_ptr->get();
		ret = av_read_frame(fmt_ctx_, pkt);
		if(ret < 0) {
			av_strerror(ret, errbuf_, sizeof(errbuf_));

			LOG_INFO("av_read_frame failed: %s", errbuf_);
			break;
		}
		if(pkt->stream_index == video_stream_index_) {
			pkt_ptr->setVideo();
			video_packet_queue_.push(pkt_ptr);
		} else if(pkt->stream_index == audio_stream_index_) {
			pkt_ptr->setAudio();
			audio_packet_queue_.push(pkt_ptr);
		}
		else {
			LOG_ERROR("unknown stream index");
		}

	}
	mtx_.lock();
	state_ = FFState::EndOfFile;
	mtx_.unlock();
	notify(MSG_EOF);
}




int FFPlayer::stop() {
	if(state_ == FFState::Stopped) {
		return 0;
	}
	if(state_ != FFState::Playing) {
		LOG_INFO("try to stop ffplayer that its' state is not playing");
		return -1;
	}
	
	int ret = 0;
	mtx_.lock();
	state_ = FFState::Stopped;
	mtx_.unlock();
	
	
	read_thread_->join();
	audio_decode_thread_->join();
	video_decode_thread_->join();
	delete audio_decode_thread_;
	delete video_decode_thread_;
	delete audio_decoder_;
	delete video_decoder_;
	delete read_thread_;
	audio_decode_thread_ = nullptr;
	video_decode_thread_ = nullptr;
	audio_decoder_ = nullptr;
	video_decoder_ = nullptr;
	read_thread_ = nullptr;

	delete swr_;
	delete sdl_player_;
	swr_ = nullptr;
	sdl_player_ = nullptr;


	notify(MSG_STOP);
	return ret;
}

int Decoder::getFrame(std::shared_ptr<Frame>&f) {
	if (!frame_queue_.empty()) {
		f = frame_queue_.pop();
		return 0;
	}
	int ret = 0;

	int send_ret = 1;
	
	do
	{
		std::shared_ptr<Packet> pkt = packet_queue_->pop();
		AVPacket* avpkt = pkt->get();
		AVFrame* frame = av_frame_alloc();
		std::shared_ptr<Frame> ftmp;
		//传入要解码的packet
		ret = avcodec_send_packet(codec_ctx_, avpkt);
		//AVERROR(EAGAIN) 传入失败，表示先要receive frame再重新send packet
		if (ret == AVERROR(EAGAIN))
		{
			send_ret = 0;
			LOG_INFO( "avcodec_send_packet = AVERROR(EAGAIN)\n");
		}
		else if (ret < 0)
		{

			av_strerror(ret, errbuf_, sizeof(errbuf_));
			LOG_INFO("avcodec_send_packet = ret < 0 : %s\n", errbuf_);
			return -1;
		}

		while (ret >= 0)
		{
			//调用avcodec_receive_frame会在内部首先调用av_frame_unref来释放frame本来的数据
			//就是这次调用会将上次调用返回的frame数据释放
			ret = avcodec_receive_frame(codec_ctx_, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				send_ret = 0;
				break;
			}
				
			else if (ret < 0)
			{
				LOG_INFO( "avcodec_receive_frame = ret < 0\n");
				
				return -1;
			}
			ftmp = std::make_shared<Frame>(frame);
			ftmp->is_audio_ = pkt->isAudio();
			ftmp->is_video_ = pkt->isVideo();
			ftmp->duration_ = frame->pkt_duration * av_q2d(frame->time_base);
			ftmp->pts_ = frame->pts * av_q2d(frame->time_base);
			frame_queue_.push(ftmp);
			send_ret = 1;
			break;
		}

	} while (!send_ret);
	
	if (!frame_queue_.empty()) {
		f = frame_queue_.pop();
	}


	return 0;


}

void FFPlayer::decodePacketLoop(int is_audio) {
	int ret = 0;
	std::shared_ptr<Frame> f;

	
	while(1) {
		mtx_.lock();
		if(state_ == FFState::Stopped) {
			mtx_.unlock();
			return;
		}
		mtx_.unlock();
		if (!is_audio) {
			if(video_decoder_->getFrame(f) == 0) {
				//video_frame_queue_.push(f);
				LOG_INFO("drop video frame");
			}
		} else {
			if(audio_decoder_->getFrame(f) == 0) {
				//audio_frame_queue_.push(f);
				uint8_t** data;
				int size = swr_->convert(f->frame_, &data);
				sdl_player_->queueAudio(data[0], size);

				LOG_INFO("queue audio frame");

			}
		}
		
	}

}

int Decoder::open_codec_context(int* stream_idx,
	AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type)
{
	int ret, stream_index;
	AVStream* st;
	const AVCodec* dec = NULL;

	ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
	if (ret < 0) {
		LOG_INFO("Could not find %s stream in input file '%s'\n",
			av_get_media_type_string(type));
		return ret;
	}
	else {
		stream_index = ret;
		st = fmt_ctx->streams[stream_index];

		/* find decoder for the stream */
		dec = avcodec_find_decoder(st->codecpar->codec_id);
		if (!dec) {
			LOG_INFO( "Failed to find %s codec\n",
				av_get_media_type_string(type));
			return AVERROR(EINVAL);
		}

		/* Allocate a codec context for the decoder */
		*dec_ctx = avcodec_alloc_context3(dec);
		if (!*dec_ctx) {
			LOG_INFO( "Failed to allocate the %s codec context\n",
				av_get_media_type_string(type));
			return AVERROR(ENOMEM);
		}

		/* Copy codec parameters from input stream to output codec context */
		if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
			LOG_INFO( "Failed to copy %s codec parameters to decoder context\n",
				av_get_media_type_string(type));
			return ret;
		}

		/* Init the decoders */
		if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
			fprintf(stderr, "Failed to open %s codec\n",
				av_get_media_type_string(type));
			return ret;
		}
		*stream_idx = stream_index;
	}

	return 0;
}

int SWSResampler::setOpts(AVChannelLayout src_ch_layout, AVChannelLayout dst_ch_layout, int src_rate, int dst_rate, enum AVSampleFormat src_sample_fmt, enum AVSampleFormat dst_sample_fmt) {
	int ret = 0;
	src_ch_layout_ = src_ch_layout;
	dst_ch_layout_ = dst_ch_layout;
	src_rate_ = src_rate;
	dst_rate_ = dst_rate;
	src_sample_fmt_ = src_sample_fmt;
	dst_sample_fmt_ = dst_sample_fmt;


	av_opt_set_chlayout(swr_ctx_, "in_chlayout", &src_ch_layout, 0);
	av_opt_set_int(swr_ctx_, "in_sample_rate", src_rate, 0);
	av_opt_set_sample_fmt(swr_ctx_, "in_sample_fmt", src_sample_fmt, 0);

	av_opt_set_chlayout(swr_ctx_, "out_chlayout", &dst_ch_layout, 0);
	av_opt_set_int(swr_ctx_, "out_sample_rate", dst_rate, 0);
	av_opt_set_sample_fmt(swr_ctx_, "out_sample_fmt", dst_sample_fmt, 0);
	if ((ret = swr_init(swr_ctx_)) < 0) {
		LOG_ERROR("swr_init failed");
		return ret;
	}
	///* allocate source and destination samples buffers */

	//src_nb_channels_ = src_ch_layout.nb_channels;
	//ret = av_samples_alloc_array_and_samples(&src_data_, &src_linesize_, src_nb_channels_,
	//	src_nb_samples_, src_sample_fmt, 0);
	//if (ret < 0) {
	//	LOG_ERROR("Could not allocate source samples\n");
	//	return ret;
	//}

	/* compute the number of converted samples: buffering is avoided
	 * ensuring that the output buffer will contain at least all the
	 * converted input samples */
	max_dst_nb_samples_ = dst_nb_samples_ =
		av_rescale_rnd(src_nb_samples_, dst_rate, src_rate, AV_ROUND_UP) + 1024; // make enough space for some frames

	/* buffer is going to be directly written to a rawaudio file, no alignment */
	dst_nb_channels_ = dst_ch_layout.nb_channels;
	ret = av_samples_alloc_array_and_samples(&dst_data_, &dst_linesize_, dst_nb_channels_,
		dst_nb_samples_, dst_sample_fmt, 0);
	if (ret < 0) {
		LOG_ERROR("Could not allocate destination samples\n");
		return ret;
	}

	return 0;

}

int SWSResampler::convert(AVFrame* frame, uint8_t*** data) {
	int ret = 0;
	/* compute destination number of samples */
	int64_t delay = swr_get_delay(swr_ctx_, src_rate_);
	dst_nb_samples_ = av_rescale_rnd(delay +
		src_nb_samples_, dst_rate_, src_rate_, AV_ROUND_UP);

	if (dst_nb_samples_ > max_dst_nb_samples_) {
		av_freep(&dst_data_[0]);
		ret = av_samples_alloc(dst_data_, &dst_linesize_, dst_nb_channels_,
			dst_nb_samples_, dst_sample_fmt_, 1);
		if (ret < 0)
			return ret;
		max_dst_nb_samples_ = dst_nb_samples_;
	}
	/* convert to destination format */
	uint8_t** src_data_ = (uint8_t**)frame->data;
	int src_nb_samples_ = frame->nb_samples;
	ret = swr_convert(swr_ctx_, dst_data_, dst_nb_samples_, (const uint8_t**)src_data_, src_nb_samples_);
	if (ret < 0) {
		fprintf(stderr, "Error while converting\n");
		return ret;
	}
	int dst_bufsize = av_samples_get_buffer_size(&dst_linesize_, dst_nb_channels_,
		ret, dst_sample_fmt_, 1);
	if (dst_bufsize < 0) {
		fprintf(stderr, "Could not get sample buffer size\n");
		return AVERROR(ENOMEM);
	}
	*data = dst_data_;

	return dst_bufsize;
}

int SDLPlayer::openAudio(AVCodecContext* codec_ctx)
{
	desired_spec_.freq = codec_ctx->sample_rate;
	switch (codec_ctx->sample_fmt) {
		case AV_SAMPLE_FMT_U8:
			desired_spec_.format = AUDIO_U8;
			break;
		case AV_SAMPLE_FMT_S16:
			desired_spec_.format = AUDIO_S16SYS;
			break;
		case AV_SAMPLE_FMT_S32:
			desired_spec_.format = AUDIO_S32SYS;
			break;
		case AV_SAMPLE_FMT_FLT:
			desired_spec_.format = AUDIO_F32SYS;
			break;
		case AV_SAMPLE_FMT_FLTP:
			desired_spec_.format = AUDIO_F32SYS;
			break;
		default:
			LOG_ERROR("unsupported sample format");
			return -1;

	}
	desired_spec_.channels = codec_ctx->ch_layout.nb_channels;
	desired_spec_.silence = 0;
	desired_spec_.samples = codec_ctx->frame_size;
	desired_spec_.callback = NULL;

	dev_ = SDL_OpenAudioDevice(NULL, 0, &desired_spec_, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);
	if (dev_ < 2) {
		LOG_ERROR("SDL_OpenAudioDevice failed");
		return -1;
	}


	// zero: play, non-zero: pause
	SDL_PauseAudioDevice(dev_, 1);

	return 0;
}

int SDLPlayer::queueAudio(uint8_t* data, int size)
{
	return SDL_QueueAudio(dev_, data, size);
}

int SDLPlayer::pauseAudio()
{
	SDL_PauseAudioDevice(dev_, 1);
	return 0;
}

int SDLPlayer::stopAudio()
{
	SDL_CloseAudioDevice(dev_);
	dev_=-1;

	return 0;
}

int SDLPlayer::playAudio()
{
	SDL_PauseAudioDevice(dev_, 0);
	return 0;
}
