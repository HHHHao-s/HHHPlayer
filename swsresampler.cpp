#include "swsresampler.h"


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


