#pragma once

#include <string>
#include <memory>

#include "msgqueue.h"
#include "logger.h"
#include "bufferqueue.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavcodec/bsf.h"
}

class Packet {
public:
	
	
	Packet():pkt(av_packet_alloc()) {
		if (!pkt) {
			LOG_ERROR("av_packet_alloc failed");
		}
		av_init_packet(pkt);
	}
	~Packet() {
		if (pkt) {
			av_packet_free(&pkt);
		}
	}
	AVPacket* get() {
		return pkt;
	}
	void setVideo() {
		is_video_ = 1;
		is_audio_ = 0;
	}
	void setAudio() {
		is_audio_ = 1;
		is_video_ = 0;
	}
private:
	AVPacket* pkt;
	int is_video_{ 0 };
	int is_audio_{ 0 };
};// packet

class FFPlayer 
{
public:
	FFPlayer();
	~FFPlayer();
	int create();
	int prepareAsync();
	int setDataSource(const std::string &url);
	int tryGetMsg(Msg &msg);
	int blockGetMsg(Msg &msg);
	int start();

	void readPacketLoop();

	std::shared_ptr<Packet> getPacket();

private:
	std::string url_;
	MsgQueue msg_queue_;
	BufferQueue<std::shared_ptr<Packet>> packet_queue_;
	std::thread* read_thread_{nullptr};
	
	AVFormatContext* fmt_ctx_{nullptr};
	AVCodec* video_codec_{nullptr};
	AVCodec* audio_codec_{nullptr};
	int video_stream_index_ = -1;
	int audio_stream_index_ = -1;
	

};


