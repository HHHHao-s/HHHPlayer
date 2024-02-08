#include "mediaplayer.h"

MediaPlayer::MediaPlayer()
{
}

MediaPlayer::~MediaPlayer()
{
}

int MediaPlayer::create(std::function<int(void*)> msg_loop) {
	int ret = 0;
	player_ = new FFPlayer();
	msg_loop_ = msg_loop;
	ret = player_->create();
	if (ret < 0) {
		delete player_;
		player_ = NULL;
		LOG_ERROR("create player failed\n");
	};
	return ret;
}

int MediaPlayer::setDataSource(const std::string & url) {
	int ret = 0;
	if (player_ == NULL) {
		LOG_ERROR("player is NULL\n");
		return -1;
	}
	url_ = url;
	ret = player_->setDataSource(url);
	return ret;
}

int MediaPlayer::prepareAsync() {
	int ret = 0;
	state_ = State::Preparing;
	
	msg_thread_ = new std::thread(msg_loop_, this);
	ret = player_->prepareAsync();
	if (ret < 0) {
		LOG_ERROR("prepareAsync failed\n");
		return -1;
	};

	return ret;
}

int MediaPlayer::tryGetMsg(Msg &msg) {
	int ret = 0;
	if (player_ == NULL) {
		LOG_ERROR("player is NULL\n");
		return -1;
	}
	ret = player_->tryGetMsg(msg);
	if (ret < 0) {
		LOG_ERROR("tryGetMsg failed\n");
	};
	ret = filterMsg(msg);
	return ret;
}

int MediaPlayer::blockGetMsg(Msg &msg) {
	int ret = 0;
	if (player_ == NULL) {
		LOG_ERROR("player is NULL\n");
		return -1;
	}
	ret = player_->blockGetMsg(msg);
	if (ret < 0) {
		LOG_ERROR("blockGetMsg failed\n");
	};
	ret = filterMsg(msg);
	return ret;
}



int MediaPlayer::filterMsg(Msg &msg) {
	int ret = 0;
	if (player_ == NULL) {
		LOG_ERROR("player is NULL\n");
		return -1;
	}
	switch (msg.what_)
	{
	case MSG_PREPARED:
		doPrepared(msg);
		break;
	default:
		break;
	}
	
	return ret;
}

int MediaPlayer::doPrepared(Msg &msg) {
	int ret = 0;
	state_ = State::Prepared;
	
	return ret;
}

int MediaPlayer::start() {
	int ret = 0;
	if (player_ == NULL) {
		LOG_ERROR("player is NULL\n");
		return -1;
	}
	ret = player_->start();
	if (ret < 0) {
		LOG_ERROR("start failed\n");
	};
	return ret;
}

int MediaPlayer::pause() {
	int ret = 0;
	
	return ret;
}

int MediaPlayer::stop() {
	int ret = 0;
	
	return ret;
}

int MediaPlayer::destroy() {
	int ret = 0;
	
	return ret;
}