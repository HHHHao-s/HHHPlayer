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
	msg_queue_.push(Msg( MSG_PREPARED));

	return ret;
}

int FFPlayer::start() {
	int ret = 0;
	read_thread_ = new std::thread(&FFPlayer::readPacketLoop, this);
	msg_queue_.push(Msg( MSG_STARTED));
	return ret;
}

int FFPlayer::setDataSource(const std::string& url) {
	url_ = url;
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
	std::shared_ptr<Packet> pkt_ptr= std::make_shared<Packet>();
	AVPacket* pkt = pkt_ptr->get();
	while(1) {
		ret = av_read_frame(fmt_ctx_, pkt);
		if(ret < 0) {
			LOG_ERROR("av_read_frame failed");
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
}

std::shared_ptr<Packet> FFPlayer::getPacket() {
	return packet_queue_.pop();
}