#pragma once

#include <mutex>
#include <queue>
#include <functional>
#include <future>
#include <thread>
#include <utility>
#include <vector>
#include <condition_variable>
#include <string>

#include "ffplayer.h"
#include "msgqueue.h"
#include "logger.h"

class MediaPlayer
{
public:
	MediaPlayer();
	~MediaPlayer();
	
	int create(std::function<int(void*)> msg_loop);
	int setDataSource(const std::string& url);
	int setVideoFrameCallback(std::function<void(std::shared_ptr<Frame>)> cb);
	int prepareAsync();
	int tryGetMsg(Msg& msg);
	int blockGetMsg(Msg& msg);
	int start();
	int pauseOrPlay();
	int stop();
	int destroy();
	std::function<double()> getCurTimeCb();
	
private:
	std::mutex mutex_;
	FFPlayer* player_{nullptr};
	std::function<int(void*)> msg_loop_; // message loop
	std::thread* msg_thread_{ nullptr };
	std::string url_{};
	
	enum class State {
		Idle,
		Prepared,
		Preparing,
		Ready,
		Playing,
		Eof,
		Error,
		Pause

	}state_{State::Idle};
	// filter those message that no need to send to ui
	int filterMsg(Msg& msg);

	int doPrepared(Msg& msg);
	int doEOF(Msg& msg);
	int doError(Msg& msg);
	int doEOS(Msg& msg);
	int doReady(Msg& msg);
	

};
