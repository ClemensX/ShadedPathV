#pragma once
/*
 * Threads, ThreadGroups
 */

 // currently size() is not guarded against xapp->getMaxThreadCount(), but fails on using comand vectors if index too high
class ThreadGroup {
public:
	ThreadGroup() = default;
	ThreadGroup(const ThreadGroup&) = delete;
	ThreadGroup& operator=(const ThreadGroup&) = delete;

	// store and start Thread
	template <class Fn, class... Args>
	void* add_t(Fn&& F, Args&&... A) {
		threads.emplace_back(std::thread(F, A...));
		return threads.back().native_handle();
	}

	// wait for all threads to finish
	void join_all() {
		for (auto& t : threads) {
			if (t.joinable()) {
				t.join();
			}
		}
	}

	~ThreadGroup() {
		join_all();
	}

	std::size_t size() const { return threads.size(); }
private:
	std::vector<std::thread> threads;
};

class ThreadInfo {
public:
	static DWORD thread_osid() {
		DWORD osid = GetCurrentThreadId();
		return osid;
	}
};

/*enum FrameState { Free, Drawing, Executing };

// hold all infos needed to syncronize render and command threads
class ThreadState {
private:
	mutex stateMutex;
	condition_variable drawSlotAvailable;
	FrameState frameState[DXManager::FrameCount];
public:
	void init() {
		for (int i = 0; i < DXManager::FrameCount; i++) {
			frameState[i] = Free;
		}
	};

	// wait until draw slot available and return slot index (0..2)
	// slots are returned in order: 0,1,2,0,1,2,...
	int waitForNextDrawSlot(int old_slot) {
		// lock needed for contition_variables
		unique_lock<mutex> myLock(stateMutex);
		// wait for next free slot
		int next_slot = old_slot + 1;
		if (next_slot >= DXManager::FrameCount) {
			next_slot = 0;
		}
		//Log("wait for next draw slot: " << next_slot << endl);
		drawSlotAvailable.wait(myLock, [this, next_slot]() {return frameState[next_slot] == Free; });

		// now we run exclusively - freely set any state
		//Log(" got next draw slot: " << next_slot << endl);
		assert(frameState[next_slot] == Free);
		frameState[next_slot] = Drawing;

		// leave critical section
		myLock.unlock();
		return next_slot;
	};

	// free draw slot. called right after swapChain->Present()
	void freeDrawSlot(int i) {
		// lock needed for contition_variables
		unique_lock<mutex> myLock(stateMutex);
		// now we run exclusively - freely set any state
		// free given slot:
		assert(frameState[i] == Drawing);
		frameState[i] = Free;

		// leave critical section
		myLock.unlock();
		//Log(" freed draw slot: " << i << endl);
		// notify waiting threads that new slot is available
		drawSlotAvailable.notify_one();
	};
};
*/

// limit running thread to max calls per second
// thread will sleep when called too often
// calling rarely will not be changed
class ThreadLimiter {
public:
	ThreadLimiter(float limit) {
		this->limit = limit;
		this->limitMicro = 1000000L / (long long)limit;
		Log("limit: " << limitMicro << endl);
		lastCallTime = chrono::high_resolution_clock::now();
	}
	void waitForLimit() {
		auto now = chrono::high_resolution_clock::now();
		long long length = chrono::duration_cast<chrono::microseconds>(now - lastCallTime).count();
		if (length < limitMicro) {
			// we are above threshold: sleep for the remaining milliseconds
			unsigned long d = (unsigned long)(limitMicro - length) / 1000; // sleep time in millis
			//Log(" limit length duration " << limitMicro << " " << length << " " << d << endl);
			Sleep(d);
		}
		else {
			// sleep at least 10ms to give other update threads a chance
			Sleep(10);
		}
		lastCallTime = chrono::high_resolution_clock::now();
	};
private:
	long long limitMicro;
	float limit;
	chrono::time_point<chrono::steady_clock> lastCallTime;
};

template<typename T>
class ThreadsafeWaitingQueue {
	queue<T> myqueue;
	mutable mutex monitorMutex;
	condition_variable cond;
	bool in_shutdown = false;
	bool logEnable = false;
	string logName = "n/a";

public:
	ThreadsafeWaitingQueue(const ThreadsafeWaitingQueue<T>&) = delete;
	ThreadsafeWaitingQueue& operator=(const ThreadsafeWaitingQueue<T>&) = delete;
	// allow move() of a queue
	ThreadsafeWaitingQueue(ThreadsafeWaitingQueue<T>&& other) {
		unique_lock<mutex> lock(monitorMutex);
		myqueue = std::move(other.myqueue);
	}
	// Create queue with logging info, waiting threads will be suspended every 3 seconds
	// to check for shutdown mode
	ThreadsafeWaitingQueue() = default;
	virtual ~ThreadsafeWaitingQueue() {};

	// set and enable logging info (to be called before any push/pop operation
	void setLoggingInfo(bool enable, string name) {
		unique_lock<mutex> lock(monitorMutex);
		logEnable = enable;
		logName = name;
	}

	// wait until item available, if nothing is returned queue is in shutdown
	optional<T> pop() {
		unique_lock<mutex> lock(monitorMutex);
		while (myqueue.empty()) {
			cond.wait_for(lock, chrono::milliseconds(3000));
			LogCondF(logEnable, logName + " wait suspended\n");
			if (in_shutdown) {
				LogCondF(LOG_QUEUE, "RenderQueue shutdown in pop\n");
				cond.notify_all();
				return nullopt;
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
		unique_lock<mutex> lock(monitorMutex);
		if (in_shutdown) {
			//throw "RenderQueue shutdown in push";
			LogCondF(logEnable, logName + " shutdown in push\n");
			return;
		}
		myqueue.push(item);
		LogCondF(logEnable, logName + " length " << myqueue.size() << endl);
		cond.notify_one();
	}

	void shutdown() {
		unique_lock<mutex> lock(monitorMutex);
		in_shutdown = true;
		cond.notify_all();
	}
};

class ThreadResources;

// queue for syncing render and queue submit threads:
class RenderQueue {
public:
	// wait until next frame has finished rendering
	ThreadResources* pop() {
		unique_lock<mutex> lock(monitorMutex);
		while (myqueue.empty()) {
			cond.wait_for(lock, chrono::milliseconds(3000));
			LogCondF(LOG_QUEUE, "RenderQueue wait suspended\n");
			if (in_shutdown) {
				LogCondF(LOG_QUEUE, "RenderQueue shutdown in pop\n");
				cond.notify_all();
				return nullptr;
			}
		}
		assert(myqueue.empty() == false);
		ThreadResources* frame = myqueue.front();
		myqueue.pop();
		cond.notify_one();
		return frame;
	}

	// push finished frame
	void push(ThreadResources* frame) {
		unique_lock<mutex> lock(monitorMutex);
		if (in_shutdown) {
			//throw "RenderQueue shutdown in push";
			LogCondF(LOG_QUEUE, "RenderQueue shutdown in push\n");
			return;
		}
		myqueue.push(frame);
		LogCondF(LOG_QUEUE, "RenderQueue length " << myqueue.size() << endl);
		cond.notify_one();
	}

	void shutdown() {
		in_shutdown = true;
		cond.notify_all();
	}

	size_t size() {
		return myqueue.size();
	}

private:
	queue<ThreadResources*> myqueue;
	mutex monitorMutex;
	condition_variable cond;
	bool in_shutdown{ false };
};
