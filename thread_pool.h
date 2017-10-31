#pragma once
#include <functional>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <thread>

class Barrier {
public:
	Barrier(int size): size(size) {}

	void done() {
		bool unlock = false;
		{
			std::unique_lock <std::mutex> lock(mutex);
			assert(size > 0);
			size--;
			unlock = size <= 0;

		}

		if(unlock) {
			cond.notify_one();
		}
	}

	void wait() {
		std::unique_lock<std::mutex> lock(mutex);
		cond.wait(lock, [&](){return size <= 0;});
	}

	void operator=(const Barrier &b) {
		size = b.size;
	}

	int getSize() const {
		return size;
	}

private:
	int size;
	std::mutex mutex;
	std::condition_variable cond;
};

class BarrierReleaser {
public:
	BarrierReleaser(Barrier& barrier): barrier(barrier) {}
	~BarrierReleaser() {
		barrier.done();
	}

private:
	Barrier& barrier;
};

class ThreadPool {
public:
	typedef std::function<void()> JobFn;

	ThreadPool(int n) {
		for(int i = 0; i < n; i++) {
			threads.push_back(std::thread([i, this]() {
				for(;;) {
					JobFn task;

					{
						std::unique_lock<std::mutex> lock(mutex);
						cond.wait(lock, [&](){return !queue.empty() || quit;});
						if(quit) {
							break;
						}

						task = queue.front();
						queue.pop();
					}

					task();
				}
			}));
		}
	}

	~ThreadPool() {
		quit = true;
		cond.notify_all();

		for(auto &t: threads) {
			t.join();
		}
	}

	void add(JobFn fn) {
		{
			std::unique_lock<std::mutex> lock(mutex);
			queue.push(fn);
		}

		cond.notify_one();
	}

	void add(JobFn fn, Barrier &barrier) {
		add([fn, &barrier]() {
			BarrierReleaser releaser(barrier);
			fn();
		});
	}

private:
	std::vector<std::thread> threads;
	std::queue<std::function<void()>> queue;
	std::mutex mutex;
	std::condition_variable cond;
	bool quit = false;
};