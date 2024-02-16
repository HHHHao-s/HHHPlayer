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

	if (avformat_find_stream_info(fmt_ctx_, NULL) < 0) {
		LOG_ERROR("avformat_find_stream_info failed");
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

	auto audio_codec_ctx = audio_decoder_->getCodecCtx();

	AVStream* st = fmt_ctx_->streams[audio_stream_index_];
	swr_ = new SWSResampler();
	swr_->setOpts(audio_codec_ctx->ch_layout, audio_codec_ctx->ch_layout, audio_codec_ctx->sample_rate, audio_codec_ctx->sample_rate, audio_codec_ctx->sample_fmt, (AVSampleFormat)AV_SAMPLE_FMT_FLT);

	audio_player_ = new PortAudioPlayer();
	audio_player_->openAudio(audio_codec_ctx);


	
	auto video_codec_ctx = video_decoder_->getCodecCtx();

	image_scaler_ = new ImageScaler();
	image_scaler_->open(video_codec_ctx->width, video_codec_ctx->height, video_codec_ctx->pix_fmt, video_codec_ctx->width, video_codec_ctx->height, AV_PIX_FMT_RGB24);

	


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

	audio_player_->playAudio();

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

int FFPlayer::setVideoFrameCallback(std::function<void(std::shared_ptr<Frame>)> cb)
{
	video_frame_cb_ = cb;
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
		std::unique_lock<std::mutex> lck(mtx_);
		if (state_ == FFState::Pause) {
			pause_cv_.wait(lck, [this] {return state_ == FFState::Playing || state_ == FFState::Stopped; });
		}
		if (state_ == FFState::Stopped) {
			lck.unlock();
			return;
		}
		lck.unlock();

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

std::function<double()> FFPlayer::getCurTimeCb()
{
	return std::bind(&PortAudioPlayer::getClock, audio_player_);
}




int FFPlayer::stop() {
	if(state_ == FFState::Stopped) {
		return 0;
	}
	if(state_ != FFState::Playing && state_!=FFState::Pause) {
		LOG_INFO("try to stop ffplayer that its' state is not playing");
		return -1;
	}
	
	int ret = 0;
	mtx_.lock();
	state_ = FFState::Stopped;
	mtx_.unlock();
	pause_cv_.notify_all();
	video_packet_queue_.wakeUp();
	audio_packet_queue_.wakeUp();

	video_decoder_->wakeUp();
	audio_decoder_->wakeUp();

	audio_player_->stopAudio();
	
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

	delete audio_player_;
	audio_player_ = nullptr;

	delete swr_;
	swr_ = nullptr;
	

	delete image_scaler_;
	image_scaler_ = nullptr;

	video_frame_cb_ = nullptr;

	state_ = FFState::Stopped;
	notify(MSG_STOP);
	return ret;
}

int FFPlayer::destroy()
{

	if (state_ != FFState::Stopped) {
		stop();
	}
	state_ = FFState::Destroyed;
	notify(MSG_DESTORY);
	return 0;
}

int FFPlayer::pauseOrPlay()
{
	mtx_.lock();
	if (state_ == FFState::Playing) {
		state_ = FFState::Pause;
		mtx_.unlock();
		audio_player_->pauseAudio();
		return 0;
	}
	else if (state_ == FFState::Pause) {
		state_ = FFState::Playing;
		mtx_.unlock();
		pause_cv_.notify_all();
		audio_player_->resumeAudio();
		return 0;
	}
	else {
		LOG_ERROR("state is not playing or pause");
		mtx_.unlock();
		return -1;
	}
	return 0;
}



void FFPlayer::decodePacketLoop(int is_audio) {
	int ret = 0;
	std::shared_ptr<Frame> f;

	
	while(1) {
		std::unique_lock<std::mutex> lck(mtx_);
		if (state_ == FFState::Pause) {
			pause_cv_.wait(lck, [this] {return state_ == FFState::Playing || state_ == FFState::Stopped; });
		}
		if(state_ == FFState::Stopped) {
			lck.unlock();
			return;
		}
		lck.unlock();
		if (!is_audio) {
			if(video_decoder_->getFrame(f) == 0) {
				//video_frame_queue_.push(f);
				LOG_INFO("get video frame");

				//auto scale_frame = std::make_shared<Frame>();
				AVFrame* scale_frame = nullptr;

				image_scaler_->scale(f->frame_, &scale_frame);
				auto pscale_frame = std::make_shared<Frame>(scale_frame);
				pscale_frame->is_video_ = 1;
				pscale_frame->duration_ = f->duration_;
				pscale_frame->pts_ = f->pts_;

				video_frame_cb_(pscale_frame);

				//std::this_thread::sleep_for(std::chrono::milliseconds(5));



			}
		} else {
			if(audio_decoder_->getFrame(f) == 0) {
				LOG_INFO("get audio frame");
				//audio_frame_queue_.push(f);
				/*uint8_t** data;
				int size = swr_->convert(f->frame_, &data);
				
				auto msg = std::make_shared<RawMessage>();

				msg->raw_ = new uint8_t[size];
				memcpy(msg->raw_, data[0], size);
				msg->size_ = size;

				audio_frame_queue_.push(msg);

				LOG_INFO("queue audio frame");*/
				audio_player_->enqueue(f);
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
		
	}

}

