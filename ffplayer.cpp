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
	
	audio_decoder_ = new Decoder(fmt_ctx_, AVMEDIA_TYPE_AUDIO, &audio_packet_queue_);
	video_decoder_ = new Decoder(fmt_ctx_, AVMEDIA_TYPE_VIDEO, &video_packet_queue_);
	audio_decode_thread_ = new std::thread(std::bind(&FFPlayer::decodePacketLoop, this, 1));
	video_decode_thread_ = new std::thread(std::bind(&FFPlayer::decodePacketLoop, this, 0));
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
			LOG_ERROR("av_read_frame failed: %s", errbuf_);
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
				video_frame_queue_.push(f);
			}
		} else {
			if(audio_decoder_->getFrame(f) == 0) {
				audio_frame_queue_.push(f);
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