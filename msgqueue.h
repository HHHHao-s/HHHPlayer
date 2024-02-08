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

struct Msg {

	int what_{MSG_NULL};
	int arg1_;
	int arg2_;
	void * obj_{ nullptr };
	size_t len_;
	std::function<void(void*)> free_{nullptr};

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
