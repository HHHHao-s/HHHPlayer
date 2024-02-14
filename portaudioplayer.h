#pragma once

#include <string>
#include <memory>

#include "msgqueue.h"
#include "logger.h"
#include "bufferqueue.h"

extern "C" {
#include <portaudio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavcodec/bsf.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/mem.h>
}

class PortAudioPlayer
{
public:
    PortAudioPlayer();
    ~PortAudioPlayer();
    int playCallback(const void* inputBuffer, void* outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags);
    int openAudio(AVCodecContext* codec_ctx);

    int pauseAudio();
    int stopAudio();
    int playAudio();

    void enqueue(std::shared_ptr<Frame> frame) {
		queue_.push(frame);
    }

    double getClock() {
        
		return audio_clock_;
	}
private:
    BufferQueue<std::shared_ptr<Frame>>queue_;
    PaStream *stream_;
    PaStreamParameters outputParameters_;
    uint8_t buffer_[(1 << 13)];
    size_t buffer_size_{ 0 };
    size_t pos_{ 0 };
    std::atomic<double> audio_clock_{ 0 };

};




