#pragma once

#include <string>
#include <memory>

#include "msgqueue.h"
#include "logger.h"
#include "bufferqueue.h"
#include "portaudioplayer.h"
#include "imagescaler.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavcodec/bsf.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/mem.h>
#include <SDL.h>
}

class Decoder {
public:
	Decoder(AVFormatContext* fmt_ctx, AVMediaType media_type, BufferQueue<std::shared_ptr<Packet>>* queue) :media_type_(media_type), packet_queue_(queue) {
		if (open_codec_context(&stream_index_, &codec_ctx_, fmt_ctx, media_type) >= 0) {
			;
		}
		else {
			LOG_INFO("open_codec_context failed");
		}
		time_base_ = fmt_ctx->streams[stream_index_]->time_base;
	}
	~Decoder() {
		if (codec_ctx_) {
			avcodec_free_context(&codec_ctx_);
		}
	}

	void setPacketQueue(BufferQueue<std::shared_ptr<Packet>>* queue) {
		packet_queue_ = queue;
	}

	int getFrame(std::shared_ptr<Frame>& frame);

	AVCodecContext* getCodecCtx() {
		return codec_ctx_;
	}

	void wakeUp() {
		frame_queue_.wakeUp();
	}

private:
	AVCodecContext* codec_ctx_{ nullptr };
	AVCodec* codec_{ nullptr };
	AVMediaType media_type_{ AVMEDIA_TYPE_UNKNOWN };
	int stream_index_{ -1 };
	BufferQueue<std::shared_ptr<Packet>>* packet_queue_{ nullptr };
	BufferQueue<std::shared_ptr<Frame>> frame_queue_;

	

	char errbuf_[128] = {};

	int open_codec_context(int* stream_idx,
		AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type);

	AVRational time_base_;

};