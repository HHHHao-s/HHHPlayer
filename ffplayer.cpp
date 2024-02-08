#include "ffplayer.h"

FFPlayer::FFPlayer()
{}

FFPlayer::~FFPlayer()
{}

int FFPlayer::create() {
	int ret = 0;
	return ret;
}

int FFPlayer::prepareAsync() {
	int ret = 0;
	return ret;
}

int FFPlayer::setDataSource(const std::string& url) {
	url_ = url;
	return 0;
}

int FFPlayer::tryGetMsg(Msg& msg) {
	int ret = 0;
	msg = msg_queue_.try_pop();
	
	return ret;

}
int FFPlayer::blockGetMsg(Msg& msg) {
	int ret = 0;
	msg = msg_queue_.wait_and_pop();
	
	return ret;
}