#pragma once
/*
 * Thread classes, independent from ShadedPath classes
 */

enum class ThreadCategory {
	// Drawing thread
	Draw,
	// global update thread
	GlobalUpdate,
	// submitting draw commands
	DrawQueueSubmit,
    // after queue submit finished rendering we can process the image
    ProcessImage,
	// main thread
	MainThread
};

class ThreadInfo {
public:
	static DWORD thread_osid() {
#if defined(_WIN64)
		DWORD osid = GetCurrentThreadId();
#else
		DWORD osid = std::hash<std::thread::id>()(std::this_thread::get_id());
#endif
		return osid;
	}
	std::string name;
	std::thread thread;
	std::thread::id id;
	ThreadCategory category;

	static std::string to_string(ThreadCategory category) {
		switch (category) {
		case ThreadCategory::Draw:
			return "Draw";
		case ThreadCategory::GlobalUpdate:
			return "GlobalUpdate";
		case ThreadCategory::DrawQueueSubmit:
			return "DrawQueueSubmit";
		case ThreadCategory::MainThread:
			return "MainThread";
		default:
			return "Unknown";
		}
	}

	friend std::ostream& operator<<(std::ostream& os, const ThreadInfo& info) {
		return os << "ThreadInfo: " << info.name.c_str() << " id: " << info.id << " category: " << to_string(info.category).c_str();
	}

};

// limit running thread to max calls per second
// thread will sleep when called too often
// calling rarely will not be changed
class ThreadLimiter {
public:
	ThreadLimiter(float limit) {
		this->limit = limit;
		this->limitMicro = 1000000L / (long long)limit;
		Log("limit: " << limitMicro << std::endl);
		lastCallTime = std::chrono::high_resolution_clock::now();
	}
	void waitForLimit() {
		auto now = std::chrono::high_resolution_clock::now();
		long long length = std::chrono::duration_cast<std::chrono::microseconds>(now - lastCallTime).count();
		if (length < limitMicro) {
			// we are above threshold: sleep for the remaining milliseconds
			unsigned long d = (unsigned long)(limitMicro - length) / 1000; // sleep time in millis
			//Log(" limit length duration " << limitMicro << " " << length << " " << d << endl);
			std::this_thread::sleep_for(std::chrono::milliseconds(d));
		}
		else {
			// sleep at least 2ms to give other update threads a chance
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}
		lastCallTime = now;// chrono::high_resolution_clock::now();
	};
private:
	long long limitMicro;
	float limit;
	std::chrono::time_point<std::chrono::steady_clock> lastCallTime;
};

template<typename T>
class ThreadsafeWaitingQueue {
	std::queue<T> myqueue;
	mutable std::mutex monitorMutex;
	std::condition_variable cond;
	bool in_shutdown = false;
	bool logEnable = false;
	std::string logName = "n/a";

public:
	ThreadsafeWaitingQueue(const ThreadsafeWaitingQueue<T>&) = delete;
	ThreadsafeWaitingQueue& operator=(const ThreadsafeWaitingQueue<T>&) = delete;
	// allow move() of a queue
	ThreadsafeWaitingQueue(ThreadsafeWaitingQueue<T>&& other) {
		std::unique_lock<std::mutex> lock(monitorMutex);
		myqueue = std::move(other.myqueue);
	}
	// Create queue with logging info, waiting threads will be suspended every 3 seconds
	// to check for shutdown mode
	ThreadsafeWaitingQueue() = default;
	virtual ~ThreadsafeWaitingQueue() {};

	// set and enable logging info (to be called before any push/pop operation
	void setLoggingInfo(bool enable, std::string name) {
		std::unique_lock<std::mutex> lock(monitorMutex);
		logEnable = enable;
		logName = name;
	}

	// wait until item available, if nothing is returned queue is in shutdown
	std::optional<T> pop() {
		std::unique_lock<std::mutex> lock(monitorMutex);
		while (myqueue.empty()) {
			cond.wait_for(lock, std::chrono::milliseconds(3000));
			if (myqueue.empty()) {
				LogCondF(logEnable, logName + " timeout wait suspended\n");
			}
			else {
				LogCondF(logEnable, logName + " pop\n");
			}
			if (in_shutdown) {
				LogCondF(LOG_QUEUE, "RenderQueue shutdown in pop\n");
				cond.notify_all();
				return std::nullopt;
			}
		}
		assert(myqueue.empty() == false);
		T tmp = myqueue.front();
		myqueue.pop();
		cond.notify_one();
		return tmp;
	}

	// push item and notify one waiting thread
	void push(const T &item) {
		std::unique_lock<std::mutex> lock(monitorMutex);
		if (in_shutdown) {
			//throw "RenderQueue shutdown in push";
			LogCondF(logEnable, logName + " shutdown in push\n");
			return;
		}
		myqueue.push(item);
		LogCondF(logEnable, logName + " length " << myqueue.size() << std::endl);
		cond.notify_one();
	}

	size_t size() {
		std::unique_lock<std::mutex> lock(monitorMutex);
		return myqueue.size();
	}

	void shutdown() {
		std::unique_lock<std::mutex> lock(monitorMutex);
		in_shutdown = true;
		cond.notify_all();
	}
};



// producer thread creates datatransfer data to be picked up by another thread
// and wait until the other thread has completed working with the data
template<typename T>
class SynchronizedDataConsumption {
	T data;
	mutable std::mutex monitorMutex;
	std::condition_variable condSignalConsumer;
	std::condition_variable condSignalProducer;
	bool in_shutdown = false;
	bool logEnable = false;
	std::string logName = "n/a";

public:
	SynchronizedDataConsumption(const SynchronizedDataConsumption<T>&) = delete;
	SynchronizedDataConsumption& operator=(const SynchronizedDataConsumption<T>&) = delete;
	// allow move() of a queue
	SynchronizedDataConsumption(SynchronizedDataConsumption<T>&& other) {
		std::unique_lock<std::mutex> lock(monitorMutex);
		data = std::move(other.data);
	}
	// Create queue with logging info, waiting threads will be suspended every 3 seconds
	// to check for shutdown mode
	SynchronizedDataConsumption() = default;
	virtual ~SynchronizedDataConsumption() {};

	// set and enable logging info (to be called before any push/pop operation
	void setLoggingInfo(bool enable, std::string name) {
		std::unique_lock<std::mutex> lock(monitorMutex);
		logEnable = enable;
		logName = name;
	}

	// wait until item available, if nothing is returned queue is in shutdown
	std::optional<T> pop() {
		std::unique_lock<std::mutex> lock(monitorMutex);
		while (data == nullptr) {
			condSignalConsumer.wait_for(lock, std::chrono::milliseconds(3000));
			if (data == nullptr) {
				LogCondF(logEnable, logName + " timeout wait suspended\n");
			}
			else {
				LogCondF(logEnable, logName + " pop\n");
			}
			if (in_shutdown) {
				LogCondF(LOG_QUEUE, "RenderQueue shutdown in pop\n");
				condSignalConsumer.notify_all();
				return std::nullopt;
			}
		}
		assert(data != nullptr);
		condSignalConsumer.notify_one();
		return data;
	}

	// push item and notify one waiting thread
	void push(const T& item) {
		std::unique_lock<std::mutex> lock(monitorMutex);
		if (in_shutdown) {
			//throw "RenderQueue shutdown in push";
			LogCondF(logEnable, logName + " shutdown in push\n");
			return;
		}
		data = item;
		LogCondF(logEnable, logName + " length 1" << std::endl);
		condSignalConsumer.notify_one();
	}

	size_t size() {
		std::unique_lock<std::mutex> lock(monitorMutex);
		return data == nullptr ? 0 : 1;
	}

	void shutdown() {
		std::unique_lock<std::mutex> lock(monitorMutex);
		in_shutdown = true;
		condSignalConsumer.notify_all();
	}
};

