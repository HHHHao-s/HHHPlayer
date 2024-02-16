#pragma once
#include <mutex>
#include <queue>
#include <functional>
#include <future>
#include <thread>
#include <utility>
#include <vector>
#include <condition_variable>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavcodec/bsf.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/mem.h>
#include <SDL.h>
}
#include "logger.h"
class Packet {
public:


	Packet() :pkt(av_packet_alloc()) {
		if (!pkt) {
			LOG_ERROR("av_packet_alloc failed");
		}

	}
	~Packet() {
		if (pkt) {
			av_packet_free(&pkt);
		}
	}
	AVPacket* get() {
		return pkt;
	}
	void setVideo() {
		is_video_ = 1;
		is_audio_ = 0;
	}
	void setAudio() {
		is_audio_ = 1;
		is_video_ = 0;
	}

	int isVideo() {
		return is_video_;
	}

	int isAudio() {
		return is_audio_;
	}
private:
	AVPacket* pkt;
	int is_video_{ 0 };
	int is_audio_{ 0 };
};// packet

struct Frame {
	int is_video_{ 0 };
	int is_audio_{ 0 };
	double pts_{ 0.0 };
	double duration_{ 0.0 };
	AVFrame* frame_{ nullptr };
	Frame() {
		
	}
	Frame(AVFrame* frame) :frame_(frame) {

	}
	~Frame() {
		if (frame_) {
			av_frame_free(&frame_);
		}
	}
	/*Frame(const Frame&) = delete;
	Frame& operator=(const Frame&) = delete;
	Frame(Frame&& other) = delete;
	Frame& operator=(Frame&& other) = delete;*/


};

struct RawMessage {

	uint8_t* raw_{ nullptr };
	size_t size_{ 0 };

	~RawMessage() {
		if (raw_) {
			delete[] raw_;
		}
	}

};

template <typename T>
class BufferQueue
{
public:
	BufferQueue() = default;
	BufferQueue(size_t mx_size);
	~BufferQueue();



	void abort() {
		mutex_.lock();
		while(!queue_.empty() )
			queue_.pop();
		mutex_.unlock();
		wakeUp();

	}

	size_t getSize() {
		return queue_.size();
	}

	bool empty() {
		return queue_.empty();
	}

	void push(const T &msg);
	void push(T &&msg);

	T pop();

	T tryPop();

	void wakeUp() {
		wake_up_ = true;
		empty_cond_.notify_all();
		full_cond_.notify_all();
	}

private:
	size_t mx_{512};
	std::queue<T> queue_;
	std::mutex mutex_;
	std::condition_variable full_cond_;
	std::condition_variable empty_cond_;
	bool abort_{ false };
	bool wake_up_{ false };
};

template <typename T>
BufferQueue<T>::BufferQueue(size_t mx_size)
{
	mx_ = mx_size;
}
template <typename T>
BufferQueue<T>::~BufferQueue()
{
}

template <typename T>
void BufferQueue<T>::push(const T& item)
{
	std::unique_lock<std::mutex> lock(mutex_);
	full_cond_.wait(lock, [this] { return queue_.size() < mx_ || wake_up_; });
	if (wake_up_) {
		return;
	}
	queue_.emplace(item);

	if (queue_.size() == 1) {
		lock.unlock();
		empty_cond_.notify_one();
	}
	else {
		lock.unlock();
	}


}

template <typename T>
void BufferQueue<T>::push(T&& item)
{
	std::unique_lock<std::mutex> lock(mutex_);
	full_cond_.wait(lock, [this] { return queue_.size() < mx_ || wake_up_; });
	if (wake_up_) {
		return;
	}
	queue_.emplace(move(item));

	if (queue_.size() == 1) {
		lock.unlock();
		empty_cond_.notify_one();
	}
	else {
		lock.unlock();
	}


}

template <typename T>
T BufferQueue<T>::pop()
{
	std::unique_lock<std::mutex> lock(mutex_);
	empty_cond_.wait(lock, [this] { return !queue_.empty() || wake_up_; });
	if (wake_up_) {
		return T();
	}
	T item = std::move(queue_.front());
	queue_.pop();

	if (queue_.size() == mx_ - 1) {
		lock.unlock();
		full_cond_.notify_one();
	}
	else {
		lock.unlock();
	}

	return item;
}

template<typename T>
T BufferQueue<T>::tryPop()
{
	std::unique_lock<std::mutex> lock(mutex_);
	if (!queue_.empty())
	{

		T item = std::move(queue_.front());
		queue_.pop();

		if (queue_.size() == mx_ - 1) {
			lock.unlock();
			full_cond_.notify_one();
		}
		else {
			lock.unlock();
		}
		return item;
	}
	return T();
}
