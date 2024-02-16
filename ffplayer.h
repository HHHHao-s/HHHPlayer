#pragma once

#include <string>
#include <memory>

#include "msgqueue.h"
#include "logger.h"
#include "bufferqueue.h"
#include "portaudioplayer.h"
#include "imagescaler.h"
#include "decoder.h"
#include "swsresampler.h"


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
	Destroyed,
};


class FFPlayer
{
public:
	FFPlayer();
	~FFPlayer();
	int create();
	int prepareAsync();
	int setDataSource(const std::string &url);
	int setVideoFrameCallback(std::function<void(std::shared_ptr<Frame>)> cb);
	int tryGetMsg(Msg &msg);
	int blockGetMsg(Msg &msg);
	int start();
	int stop();
	int destroy();
	int pauseOrPlay();

	void decodePacketLoop(int );

	void readPacketLoop();

	//std::shared_ptr<Packet> getPacket();
	std::function<double()> getCurTimeCb();


private:

	void notify(int what);

	std::string url_;
	MsgQueue msg_queue_;
	BufferQueue<std::shared_ptr<Packet>> audio_packet_queue_;
	BufferQueue<std::shared_ptr<Packet>> video_packet_queue_;

	
	std::mutex mtx_;
	std::condition_variable pause_cv_;
	

	std::thread* read_thread_{nullptr};
	std::thread* audio_decode_thread_{ nullptr };
	std::thread* video_decode_thread_{ nullptr };
	
	AVFormatContext* fmt_ctx_{nullptr};
	AVCodec* video_codec_{nullptr};
	AVCodec* audio_codec_{nullptr};
	int video_stream_index_{ -1 };
	int audio_stream_index_{ -1 };

	char errbuf_[128] = {};
	
	FFState	state_{ 
		FFState::Idle,
	};
	Decoder* video_decoder_{ nullptr };
	Decoder* audio_decoder_{ nullptr };

	BufferQueue<std::shared_ptr<Frame>> video_frame_queue_;
	

	SWSResampler* swr_{ nullptr };

	PortAudioPlayer* audio_player_{ nullptr };

	ImageScaler* image_scaler_{ nullptr };

	std::function<void(std::shared_ptr<Frame>)> video_frame_cb_{ nullptr };
};

