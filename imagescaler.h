#pragma once

#include <string>
#include <memory>

#include "msgqueue.h"
#include "logger.h"
#include "bufferqueue.h"
#include "portaudioplayer.h"

extern "C" {

#include <libavformat/avformat.h>
#include <libavutil/mem.h>
#include <SDL.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>
}
class ImageScaler
{
public:
	ImageScaler();
	~ImageScaler();

    int open(int src_width, int src_height, AVPixelFormat src_pix_fmt, int dst_width, int dst_height, AVPixelFormat dst_pix_fmt);

    int scale(const AVFrame* src_frame, AVFrame** dst_frame);

private:
    uint8_t* src_data_[4], * dst_data_[4];
    int src_linesize_[4], dst_linesize_[4];
    int src_width_, src_height_;
    int dst_width_, dst_height_;
    enum AVPixelFormat src_pix_fmt_ = AV_PIX_FMT_YUV420P;
    enum AVPixelFormat dst_pix_fmt_ = AV_PIX_FMT_RGB24;
    int dst_bufsize_;
    struct SwsContext* sws_ctx_;
};
