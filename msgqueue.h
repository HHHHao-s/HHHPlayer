#pragma once
#include <mutex>
#include <queue>
#include <functional>
#include <future>
#include <thread>
#include <utility>
#include <vector>
#include <condition_variable>

#define MSG_NULL -1
#define MSG_PREPARED 100
#define MSG_STARTED 101
#define MSG_READY 102
#define MSG_EOF 103
#define MSG_ENDOFSTREAM 104
#define MSG_ERROR 105
#define MSG_STOP 106
#define MSG_DESTORY 107

struct Msg {

	int what_{MSG_NULL};
	int arg1_{0};
	int arg2_{ 0 };
	void * obj_{ nullptr };
	size_t len_{ 0 };
	std::function<void(void*)> free_{nullptr};
	Msg() = default;
	Msg(int what, int arg1=0, int arg2=0) : what_(what), arg1_(arg1), arg2_(arg2) {}

	void free() {
		if (free_ != nullptr) {
			free_(obj_);
		}
		what_ = MSG_NULL;
	}
};



class MsgQueue {

public:

	MsgQueue() = default;
	MsgQueue(const MsgQueue&) = delete;
	MsgQueue& operator=(const MsgQueue&) = delete;
	MsgQueue(MsgQueue&&) = delete;
	MsgQueue& operator=(MsgQueue&&) = delete;


	void push(const Msg& msg);
	bool empty();
	Msg wait_and_pop();
	Msg try_pop();
	void clear();
	void clearSpecific(int what);

private:
	std::queue<Msg> msg_queue_;
	std::mutex mutex_;
	std::condition_variable cond_;
};


