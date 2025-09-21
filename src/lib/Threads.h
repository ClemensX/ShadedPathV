#pragma once
/*
 * Threads, ThreadGroups
 */

enum class GlobalResourceSet;
class ShaderBase;


class ThreadGroup {
public:
	// ThreadGroup(0) gets a group with no pre-created threads. Used for QueueSubmit and other 'global' activities.
	// ThreadGroup(32) gets a thread group with 32 worker threads, that will be auto-assigned during asyncSubmit.
	ThreadGroup(size_t numThreads) : activeThreads(0) {
		for (size_t i = 0; i < numThreads; ++i) {
			addThread(ThreadCategory::GlobalUpdate, "WorkerThread_" + std::to_string(i), [this] {
				while (true) {
					std::function<void()> task;
					{
						std::unique_lock<std::mutex> lock(queueMutex);
						condition.wait(lock, [this] { return !tasks.empty() || terminate; });
						if (terminate && tasks.empty()) return;
						task = std::move(tasks.front());
						tasks.pop();
					}
					activeThreads++;
					task();
					activeThreads--;
				}
				});
		}
	}

	~ThreadGroup() {
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			terminate = true;
		}
		condition.notify_all();
		join_all();
	}

	// Method to submit tasks using std::async
	template<class F, class... Args>
	auto asyncSubmit(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
		using returnType = typename std::invoke_result<F, Args...>::type;

		auto task = std::make_shared<std::packaged_task<returnType()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);

		std::future<returnType> res = task->get_future();
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			tasks.emplace([task]() { (*task)(); });
		}
		condition.notify_one();
		return res;
	}

	// Other methods remain unchanged
	template <class Fn, class... Args>
	std::thread::native_handle_type addThread(ThreadCategory category, const std::string& name, Fn&& F, Args&&... A) {
		ThreadInfo info;
		info.name = name;
		info.category = category;
		info.thread = std::thread([this, &info, func = std::forward<Fn>(F), A...]() mutable {
			func(std::forward<Args>(A)...);
			});
		threads.push_back(std::move(info));
		threads.back().id = threads.back().thread.get_id();
		auto native_handle = threads.back().thread.native_handle();
#if defined(_WIN64)
		std::wstring mod_name = Util::string2wstring(name);
		SetThreadDescription((HANDLE)native_handle, mod_name.c_str());
#endif
		return native_handle;
	}

	ThreadInfo& current_thread() {
		auto id = std::this_thread::get_id();
		for (auto& t : threads) {
			if (t.id == id) {
				return t;
			}
		}
		throw std::runtime_error("current_thread() not found");
	}

	void log_current_thread() {
		auto& t = current_thread();
		Log(t << std::endl);
	}

	void join_all() {
		for (auto& t : threads) {
			if (t.thread.joinable()) {
				t.thread.join();
			}
		}
	}

	std::size_t size() const { return threads.size(); }

	std::size_t getActiveThreadCount() const { return activeThreads.load(); }

private:
	std::vector<ThreadInfo> threads;
	std::queue<std::function<void()>> tasks;
	std::mutex queueMutex;
	std::condition_variable condition;
	std::atomic<std::size_t> activeThreads;
	bool terminate = false;
};


class ThreadGroupOld {
public:
	ThreadGroupOld() = default;
	ThreadGroupOld(const ThreadGroupOld&) = delete;
	ThreadGroupOld& operator=(const ThreadGroupOld&) = delete;

	// method to call add_t () with ThreadCategory and name of thread
	template <class Fn, class... Args>
	void* addThread(ThreadCategory category, const std::string& name, Fn&& F, Args&&... A) {
		ThreadInfo info;
		info.name = name;
		info.category = category;
		info.thread = std::thread([this, &info, func = std::forward<Fn>(F), A...]() mutable {
			func(std::forward<Args>(A)...);
			});
		threads.push_back(std::move(info));
		threads.back().id = threads.back().thread.get_id();
		auto native_handle = threads.back().thread.native_handle();
#if defined(_WIN64)
		std::wstring mod_name = Util::string2wstring(name);
		SetThreadDescription((HANDLE)native_handle, mod_name.c_str());
#endif
		return native_handle;
	}

	// find current thread in group and return reference to ThreadInfo
	ThreadInfo& current_thread() {
		auto id = std::this_thread::get_id();
		for (auto& t : threads) {
			if (t.id == id) {
				return t;
			}
		}
		throw std::runtime_error("current_thread() not found");
	}

	// log current thread info
	void log_current_thread() {
		auto& t = current_thread();
		Log(t << std::endl);
	}


	// wait for all threads to finish
	void join_all() {
		for (auto& t : threads) {
			if (t.thread.joinable()) {
				t.thread.join();
			}
		}
	}

	~ThreadGroupOld() {
		join_all();
	}

	std::size_t size() const { return threads.size(); }

private:
	std::vector<ThreadInfo> threads;
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


// queue for syncing render and queue submit threads:
class RenderQueue {
public:
	// wait until next frame has finished rendering
	QueueSubmitResources* pop() {
		std::unique_lock<std::mutex> lock(monitorMutex);
		while (myqueue.empty()) {
			cond.wait_for(lock, std::chrono::milliseconds(3000));
			LogCondF(LOG_QUEUE, "RenderQueue wait suspended\n");
			if (in_shutdown) {
				LogCondF(LOG_QUEUE, "RenderQueue shutdown in pop\n");
				cond.notify_all();
				return nullptr;
			}
		}
		assert(myqueue.empty() == false);
		QueueSubmitResources* frame = myqueue.front();
		myqueue.pop();
		cond.notify_one();
		return frame;
	}

	// push finished frame
	void push(QueueSubmitResources* frame) {
		std::unique_lock<std::mutex> lock(monitorMutex);
		if (in_shutdown) {
			//throw "RenderQueue shutdown in push";
			LogCondF(LOG_QUEUE, "RenderQueue shutdown in push\n");
			return;
		}
		myqueue.push(frame);
		LogCondF(LOG_QUEUE, "RenderQueue length " << myqueue.size() << std::endl);
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
	std::queue<QueueSubmitResources*> myqueue;
	std::mutex monitorMutex;
	std::condition_variable cond;
	bool in_shutdown{ false };
};

// allow update thread to wait for render threads having adopted resource switches
class RenderThreadNotification {
	ThreadsafeWaitingQueue<int> queue;
	// render threads do not wait() but check this atomic for > 0
	// then notify update thread
	// update thread sets this to number of render threads initially
	// and decreases for every pop() render thread
	std::atomic<int> outstandingAdoptions;
	GlobalResourceSet currentResourceSet;
	ShaderBase* currentShaderInstance = nullptr;
	// keep track of notifications: 1 means not done
	std::array<int, 10> notificationList;
public:
	// update thread waiting method:
	// no need to sync because we only have 1 update thread
	void waitForRenderThreads(size_t num, GlobalResourceSet resourceSet, ShaderBase* shaderInstance) {
		if (num > notificationList.size()) Error("too many render threads");
		for (size_t i = 0; i < num; i++) {
			notificationList[i] = 1;
		}
		currentResourceSet = resourceSet;
		currentShaderInstance = shaderInstance;
		outstandingAdoptions = num;
		while (true) {
			std::optional<size_t> opt = queue.pop();
			if (opt.has_value()) {
				internalResourceSwitchComplete(opt.value());
			}
			if (outstandingAdoptions == 0) {
				break;
			}
		}
	}

	// render threads: fast check to see if there is work to do (just checking atomic value)
	bool resourceSwitchAvailable(int renderThreadNum, GlobalResourceSet& resSet, ShaderBase*& shaderInstance) {
		if (outstandingAdoptions == 0) {
			return false;
		}
		if (notificationList[renderThreadNum] == 1) {
			shaderInstance = currentShaderInstance;
			resSet = currentResourceSet;
			return true;
		}
		return false;
	}

	// render threads: signal completed resource switch
	void resourceSwitchComplete(int renderThreadNum) {
		// double check that we actually wait for this thread:
		if (notificationList[renderThreadNum] == 1 && outstandingAdoptions > 0) {
			Log("resourceSwitchComplete(" << renderThreadNum << ")" << std::endl);
			queue.push(renderThreadNum);
		} else {
			Log("WARNING: unexpected call to resourceSwitchComplete(" << renderThreadNum << ")" << std::endl);
		}
	}
private:
	void internalResourceSwitchComplete(int renderThreadNum) {
		Log("internalResourceSwitchComplete(" << renderThreadNum << ")" << std::endl);
		notificationList[renderThreadNum] = 0;
		outstandingAdoptions--;
		queue.push(renderThreadNum);
	}
};
