#include "decoder.h"


int Decoder::getFrame(std::shared_ptr<Frame>& f) {
	if (!frame_queue_.empty()) {
		f = frame_queue_.pop();
		return 0;
	}
	int ret = 0;

	int send_ret = 1;

	do
	{
		std::shared_ptr<Packet> pkt = packet_queue_->pop();
		if (!pkt) {
			// no packet in queue or queue is being waked up
			return -1;
		}
		AVPacket* avpkt = pkt->get();
		AVFrame* frame = av_frame_alloc();
		std::shared_ptr<Frame> ftmp;
		//传入要解码的packet
		ret = avcodec_send_packet(codec_ctx_, avpkt);
		//AVERROR(EAGAIN) 传入失败，表示先要receive frame再重新send packet
		if (ret == AVERROR(EAGAIN))
		{
			send_ret = 0;
			LOG_INFO("avcodec_send_packet = AVERROR(EAGAIN)\n");
		}
		else if (ret < 0)
		{

			av_strerror(ret, errbuf_, sizeof(errbuf_));
			LOG_INFO("avcodec_send_packet = ret < 0 : %s\n", errbuf_);
			return -1;
		}

		while (ret >= 0)
		{
			//调用avcodec_receive_frame会在内部首先调用av_frame_unref来释放frame本来的数据
			//就是这次调用会将上次调用返回的frame数据释放
			ret = avcodec_receive_frame(codec_ctx_, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				send_ret = 0;
				break;
			}

			else if (ret < 0)
			{
				LOG_INFO("avcodec_receive_frame = ret < 0\n");

				return -1;
			}
			ftmp = std::make_shared<Frame>(frame);
			ftmp->is_audio_ = pkt->isAudio();
			ftmp->is_video_ = pkt->isVideo();
			ftmp->duration_ = frame->pkt_duration * av_q2d(time_base_);
			ftmp->pts_ = frame->pts * av_q2d(time_base_);
			frame_queue_.push(ftmp);
			send_ret = 1;
			break;
		}

	} while (!send_ret);

	if (!frame_queue_.empty()) {
		f = frame_queue_.pop();
	}


	return 0;


}

int Decoder::open_codec_context(int* stream_idx,
	AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type)
{
	int ret, stream_index;
	AVStream* st;
	const AVCodec* dec = NULL;

	ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
	if (ret < 0) {
		LOG_INFO("Could not find %s stream in input file '%s'\n",
			av_get_media_type_string(type));
		return ret;
	}
	else {
		stream_index = ret;
		st = fmt_ctx->streams[stream_index];

		/* find decoder for the stream */
		dec = avcodec_find_decoder(st->codecpar->codec_id);
		if (!dec) {
			LOG_INFO("Failed to find %s codec\n",
				av_get_media_type_string(type));
			return AVERROR(EINVAL);
		}

		/* Allocate a codec context for the decoder */
		*dec_ctx = avcodec_alloc_context3(dec);
		if (!*dec_ctx) {
			LOG_INFO("Failed to allocate the %s codec context\n",
				av_get_media_type_string(type));
			return AVERROR(ENOMEM);
		}

		/* Copy codec parameters from input stream to output codec context */
		if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
			LOG_INFO("Failed to copy %s codec parameters to decoder context\n",
				av_get_media_type_string(type));
			return ret;
		}

		/* Init the decoders */
		if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
			fprintf(stderr, "Failed to open %s codec\n",
				av_get_media_type_string(type));
			return ret;
		}
		*stream_idx = stream_index;
	}

	return 0;
}

