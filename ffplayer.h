#pragma once

#include <string>
#include <memory>

#include "msgqueue.h"
#include "logger.h"
#include "bufferqueue.h"
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



enum class FFState
{
	Idle,
	Prepared,
	Ready,
	Playing,
	EndOfFile,
	EndOfStream,
	Stopped,
	Pause,

};

class Packet {
public:
	
	
	Packet():pkt(av_packet_alloc()) {
		if (!pkt) {
			LOG_ERROR("av_packet_alloc failed");
		}
		
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

	int isVideo() {
		return is_video_;
	}

	int isAudio() {
		return is_audio_;
	}
private:
	AVPacket* pkt;
	int is_video_{ 0 };
	int is_audio_{ 0 };
};// packet

struct Frame {
	int is_video_{ 0 };
	int is_audio_{ 0 };
	double pts_{ 0.0 };
	double duration_{ 0.0 };
	AVFrame* frame_{ nullptr };
	Frame() {
		frame_ = av_frame_alloc();
	}
	Frame(AVFrame *frame) :frame_(frame) {
		
	}
	~Frame() {
		if (frame_) {
			av_frame_free(&frame_);
		}
	}
	/*Frame(const Frame&) = delete;
	Frame& operator=(const Frame&) = delete;
	Frame(Frame&& other) = delete;
	Frame& operator=(Frame&& other) = delete;*/

	

};

class SDLPlayer {
public:

private:

	SDL_AudioSpec desired_spec_;

};

class SWSResampler {
public:
	SWSResampler() {
		swr_ctx_ = swr_alloc();
		if (!swr_ctx_) {
			LOG_ERROR("swr_alloc failed");
		}
	}
	~SWSResampler() {

		if (dst_data_)
			av_freep(&dst_data_[0]);
		av_freep(&dst_data_);

		swr_free(&swr_ctx_);
	}
	int setOpts(AVChannelLayout src_ch_layout, AVChannelLayout dst_ch_layout, int src_rate, int dst_rate, enum AVSampleFormat src_sample_fmt, enum AVSampleFormat dst_sample_fmt);
	// the old data can't be used if convert success
	// return the size of the new data
	int convert(AVFrame* frame, uint8_t*** data);
private:
	SwrContext* swr_ctx_{ nullptr };
	AVChannelLayout src_ch_layout_;
	AVChannelLayout dst_ch_layout_;
	int src_rate_;
	int dst_rate_;
	enum AVSampleFormat src_sample_fmt_;
	enum AVSampleFormat dst_sample_fmt_;
	int src_nb_samples_ = 1024, dst_nb_samples_, max_dst_nb_samples_;
	int src_nb_channels_ = 0, dst_nb_channels_ = 0;
	int src_linesize_, dst_linesize_;
	uint8_t** dst_data_ = NULL;
};

class Decoder {
public:
	Decoder(AVFormatContext * fmt_ctx,AVMediaType media_type, BufferQueue<std::shared_ptr<Packet>>* queue):media_type_(media_type), packet_queue_(queue){
		if (open_codec_context(&stream_index_, &codec_ctx_, fmt_ctx, media_type) >= 0) {
			;
		}
		else {
			LOG_INFO("open_codec_context failed");
		}
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

private:
	AVCodecContext* codec_ctx_{nullptr};
	AVCodec* codec_{ nullptr };
	AVMediaType media_type_{ AVMEDIA_TYPE_UNKNOWN };
	int stream_index_{ -1 };
	BufferQueue<std::shared_ptr<Packet>>* packet_queue_{ nullptr };
	BufferQueue<std::shared_ptr<Frame>> frame_queue_;

	FFState *state_{ nullptr };

	char errbuf_[128] = {};

	int open_codec_context(int* stream_idx,
		AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type);
	
};



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
	int stop();
	//int destory();

	void decodePacketLoop(int );

	void readPacketLoop();

	//std::shared_ptr<Packet> getPacket();



private:

	void notify(int what);

	std::string url_;
	MsgQueue msg_queue_;
	BufferQueue<std::shared_ptr<Packet>> audio_packet_queue_;
	BufferQueue<std::shared_ptr<Packet>> video_packet_queue_;

	
	std::mutex mtx_;
	std::thread* read_thread_{nullptr};
	std::thread* audio_decode_thread_{ nullptr };
	std::thread* video_decode_thread_{ nullptr };
	
	AVFormatContext* fmt_ctx_{nullptr};
	AVCodec* video_codec_{nullptr};
	AVCodec* audio_codec_{nullptr};
	int video_stream_index_{ -1 };
	int audio_stream_index_{ -1 };

	char errbuf_[128] = {};
	
	FFState	state_{ FFState::Idle};
	Decoder* video_decoder_{ nullptr };
	Decoder* audio_decoder_{ nullptr };

	BufferQueue<std::shared_ptr<Frame>> video_frame_queue_;
	BufferQueue<std::shared_ptr<Frame>> audio_frame_queue_;

	SWSResampler* swr_{ nullptr };
};

