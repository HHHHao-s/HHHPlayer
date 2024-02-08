#include "msgqueue.h"


void MsgQueue::push(const Msg& msg) {
	std::lock_guard<std::mutex> lock(mutex_);
	msg_queue_.push(msg);
	cond_.notify_one();
}

bool MsgQueue::empty() {
	std::lock_guard<std::mutex> lock(mutex_);
	return msg_queue_.empty();
}

Msg MsgQueue::wait_and_pop() {
	std::unique_lock<std::mutex> lock(mutex_);
	cond_.wait(lock, [this] { return !msg_queue_.empty(); });
	Msg msg = std::move(msg_queue_.front());
	msg_queue_.pop();
	return msg;
}

Msg MsgQueue::try_pop() {
	std::lock_guard<std::mutex> lock(mutex_);
	if (msg_queue_.empty()) {
		Msg msg;
		msg.what_ = MSG_NULL;
		return msg;
	}
	Msg msg = std::move(msg_queue_.front());
	msg_queue_.pop();
	return msg;
}

void MsgQueue::clear() {
	std::lock_guard<std::mutex> lock(mutex_);
	while (!msg_queue_.empty()) {
		Msg msg = std::move(msg_queue_.front());
		msg_queue_.pop();
		if (msg.free_) {
			msg.free_(msg.obj_);
		}
	}
}

void MsgQueue::clearSpecific(int what) {
	std::lock_guard<std::mutex> lock(mutex_);
	std::queue<Msg> new_queue;
	while (!msg_queue_.empty()) {
		Msg msg = std::move(msg_queue_.front());
		msg_queue_.pop();
		if (msg.what_ != what) {
			new_queue.push(msg);
		}
		else {
			if (msg.free_) {
				msg.free_(msg.obj_);
			}
		}
	}
	msg_queue_ = std::move(new_queue);
}