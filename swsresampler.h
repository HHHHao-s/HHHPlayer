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