#include "imagescaler.h"

ImageScaler::ImageScaler()
{
}

ImageScaler::~ImageScaler()
{
}

int ImageScaler::open(int src_width, int src_height, AVPixelFormat src_pix_fmt, int dst_width, int dst_height, AVPixelFormat dst_pix_fmt)
{
	int ret = 0;
	sws_ctx_ = sws_getContext(src_width, src_height, src_pix_fmt, dst_width, dst_height, dst_pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
	if (!sws_ctx_)
	{
		LOG_ERROR("Could not initialize the conversion context\n");
		ret = -1;
	}
	
	src_width_ = src_width;
	src_height_ = src_height;
	src_pix_fmt_ = src_pix_fmt;
	dst_width_ = dst_width;
	dst_height_ = dst_height;
	dst_pix_fmt_ = dst_pix_fmt;

	
	ret = av_image_alloc(dst_data_, dst_linesize_, dst_width, dst_height, dst_pix_fmt, 1);
	dst_bufsize_ = ret;
	return 0;

}

int ImageScaler::scale(const AVFrame* src_frame, AVFrame** dst_frame)
{
	int ret = 0;

	
	if (ret < 0)
	{
		LOG_ERROR("Could not allocate destination image\n");
		ret = -1;
	}
	AVFrame* rgb_frame = av_frame_alloc();
	rgb_frame->format = dst_pix_fmt_;
	rgb_frame->width = dst_width_;
	rgb_frame->height = dst_height_;
	av_frame_get_buffer(rgb_frame, 0);
	av_frame_make_writable(rgb_frame);
	ret = sws_scale(sws_ctx_, (const uint8_t * const *)src_frame->data, src_frame->linesize, 0, src_height_, rgb_frame->data, rgb_frame->linesize);
	if (ret < 0)
	{
		LOG_ERROR("sws_scale error\n");
		ret = -1;
	}
	if (*dst_frame != nullptr) {
		av_frame_free(dst_frame);
	}
	*dst_frame = rgb_frame;
	


	return 0;
}
