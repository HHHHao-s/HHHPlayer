#pragma once
#include <string>
#include "msgqueue.h"
#include "logger.h"

class FFPlayer 
{
public:
	FFPlayer();
	~FFPlayer();
	int create();
	int prepareAsync();
	int setDataSource(const std::string &url);
	int tryGetMsg(Msg &msg);
	int blockGetMsg(Msg &msg);
private:
	std::string url_;
	MsgQueue msg_queue_;

};
