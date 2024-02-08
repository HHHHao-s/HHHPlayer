#pragma once
#include <mutex>
#include <queue>
#include <functional>
#include <future>
#include <thread>
#include <utility>
#include <vector>
#include <condition_variable>


template <typename T>
class BufferQueue
{
public:
	BufferQueue() = default;
	BufferQueue(size_t mx_size);
	~BufferQueue();

	size_t getSize() {
		return queue_.size();
	}

	void push(const T &msg);
	void push(T &&msg);

	T pop();

private:
	size_t mx_{30};
	std::queue<T> queue_;
	std::mutex mutex_;
	std::condition_variable full_cond_;
	std::condition_variable empty_cond_;
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
	full_cond_.wait(lock, [this] { return queue_.size() < mx_; });
	queue_.push(item);
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
	full_cond_.wait(lock, [this] { return queue_.size() < mx_; });
	queue_.emplace(std::move(item));

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
	empty_cond_.wait(lock, [this] { return !queue_.empty(); });
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