#include "ffplayer.h"

FFPlayer::FFPlayer()
{}

FFPlayer::~FFPlayer()
{
	if(fmt_ctx_) {
		avformat_close_input(&fmt_ctx_);
	}
	delete read_thread_;


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
	int ret = 0;
	read_thread_ = new std::thread(&FFPlayer::readPacketLoop, this);
	decode_thread_ = new std::thread(&FFPlayer::decodePacketLoop, this);
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
		} else if(pkt->stream_index == audio_stream_index_) {
			pkt_ptr->setAudio();
		}
		else {
			LOG_ERROR("unknown stream index");
		}
		packet_queue_.push(pkt_ptr);
	}
	mtx_.lock();
	state_ = FFState::EndOfFile;
	mtx_.unlock();
	notify(MSG_EOF);
}

std::shared_ptr<Packet> FFPlayer::getPacket() {
	return packet_queue_.pop();
}

void FFPlayer::decodePacketLoop() {
	int ret = 0;
	FILE *fp = fopen(ROOT_DIR "/data/out.h264", "wb");
	FILE *fp_audio = fopen(ROOT_DIR "/data/out.aac", "wb");
	while (1) {
		mtx_.lock();
		if (state_ == FFState::Stopped) {
			mtx_.unlock();
			fclose(fp);
			fclose(fp_audio);
			return;
		}
		mtx_.unlock();

		if(packet_queue_.empty()) {
			mtx_.lock();
			if(state_ == FFState::EndOfFile) {
				state_ = FFState::EndOfStream;
				notify(MSG_ENDOFSTREAM);
				mtx_.unlock();
				break;
			}
			mtx_.unlock();
		}
		
		std::shared_ptr<Packet> pkt_ptr = getPacket();
		AVPacket* pkt = pkt_ptr->get();
		if (pkt_ptr->isAudio()) {
			fwrite(pkt->data, 1, pkt->size, fp_audio);
		}
		else if (pkt_ptr->isVideo()) {
			fwrite(pkt->data, 1, pkt->size, fp);
		}
	}
	fclose(fp);
	fclose(fp_audio);
}

int FFPlayer::stop() {
	int ret = 0;
	mtx_.lock();
	state_ = FFState::Stopped;
	mtx_.unlock();
	
	packet_queue_.abort();
	read_thread_->join();
	decode_thread_->join();
	read_thread_ = nullptr;
	decode_thread_ = nullptr;
	notify(MSG_STOP);
	return ret;
}

