#include "portaudioplayer.h"

PortAudioPlayer::PortAudioPlayer()
{
}

PortAudioPlayer::~PortAudioPlayer()
{
	if(stream_ != NULL) {
		PaError err = Pa_CloseStream(stream_);
		if (err != paNoError)
		{
			LOG_ERROR("PortAudio error: %s\n", Pa_GetErrorText(err));
		}
	}
	PaError err = Pa_Terminate();
	if (err != paNoError)
	{
		LOG_ERROR("PortAudio error: %s\n", Pa_GetErrorText(err));
	}
}

int PortAudioPlayer::playCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags)
{

	unsigned long left = framesPerBuffer;

	

	auto f = queue_.pop();
	float *out = (float*)outputBuffer;

	float* leftChannel = (float*)f->frame_->data[0];
	float* rightChannel = (float*)f->frame_->data[1];


	for(int i = 0; i < framesPerBuffer; i++) {
		*out++ = leftChannel[i];
		*out++ = rightChannel[i];
	}

	PaTime now = Pa_GetStreamTime(stream_);
	LOG_INFO("now:%lf out_time:%lf\n", now, timeInfo->outputBufferDacTime);

	audio_clock_ = timeInfo->outputBufferDacTime;


	return paContinue;
}

/* This routine will be called by the PortAudio engine when audio is needed.
 * It may called at interrupt level on some machines so don't do anything
 * that could mess up the system like calling malloc() or free().
*/
static int paCallback(const void* inputBuffer, void* outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void* userData)
{
	PortAudioPlayer* player = (PortAudioPlayer*)userData;
	return player->playCallback(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
}

static void paFinishCallback(void* userData)
{

	LOG_INFO("PortAudio finish callback\n");

}

int PortAudioPlayer::openAudio(AVCodecContext* codec_ctx)
{

	PaError err;

	err = Pa_Initialize();
	if (err != paNoError)
	{
		LOG_ERROR("PortAudio error: %s\n", Pa_GetErrorText(err));
		return -1;
	}

	outputParameters_.device = Pa_GetDefaultOutputDevice();
	if (outputParameters_.device == paNoDevice)
	{
		LOG_ERROR("Error: No default output device.\n");
		return -1;
	}
	outputParameters_.channelCount = codec_ctx->ch_layout.nb_channels;
	outputParameters_.sampleFormat = paFloat32;
	outputParameters_.suggestedLatency = Pa_GetDeviceInfo(outputParameters_.device)->defaultLowOutputLatency;
	outputParameters_.hostApiSpecificStreamInfo = NULL;

	err = Pa_OpenStream(&stream_, NULL, &outputParameters_, codec_ctx->sample_rate,1024, paClipOff, &paCallback, this);
	if(err!=paNoError) {
		LOG_ERROR("PortAudio error: %s\n", Pa_GetErrorText(err));
		return -1;
	}

	err = Pa_SetStreamFinishedCallback(stream_, &paFinishCallback);
	if (err != paNoError) {
		LOG_ERROR("PortAudio error: %s\n", Pa_GetErrorText(err));
		return -1;
	}

	return 0;

}

int PortAudioPlayer::pauseAudio()
{
	PaError err = Pa_StopStream(stream_);
	if (err != paNoError)
	{
		LOG_ERROR("PortAudio error: %s\n", Pa_GetErrorText(err));
		return -1;
	}

	return 0;
}

int PortAudioPlayer::stopAudio()
{

	PaError err = Pa_StopStream(stream_);
	if (err != paNoError)
	{
		LOG_ERROR("PortAudio error: %s\n", Pa_GetErrorText(err));
		return -1;
	}


	return 0;
}

int PortAudioPlayer::playAudio()
{
	PaError err = Pa_StartStream(stream_);
	if (err != paNoError)
	{
		LOG_ERROR("PortAudio error: %s\n", Pa_GetErrorText(err));
		return -1;
	}
	return 0;
}
