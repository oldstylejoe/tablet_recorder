//Joe Snider
//4/15
//
//a thread safe queue for communication.
// stolen without care from: https://juanchopanzacpp.wordpress.com/2013/02/26/concurrent-queue-c11/


#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>

#ifndef THREAD_QUEUE
#define THREAD_QUEUE

template <typename T>
class threadQueue
{
public:

	T pop()
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		auto item = queue_.front();
		queue_.pop_front();
		return item;
	}

	//use like: while(pop(val)) {//do something}
	bool pop(T& item)
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		if (!queue_.empty()) {
			item = queue_.front();
			queue_.pop_front();
			return true;
		}
		return false;
	}

	void push(const T& item)
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		queue_.push_back(item);
	}

	void push(T&& item)
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		queue_.push_back(std::move(item));
	}

private:
	std::deque<T> queue_;
	std::mutex mutex_;
};

#endif //THREAD_QUEUE
