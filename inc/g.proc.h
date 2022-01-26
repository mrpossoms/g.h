#pragma once

#include <thread>
#include <deque>
#include <mutex>
#include <functional>

namespace g
{
namespace proc
{

template<size_t POOL_SIZE>
struct thread_pool
{

	thread_pool()
	{
		for (unsigned i = 0; i < POOL_SIZE; i++)
		{
			// prevent the worker from looping without a task
			worker_locks[i].lock();
			worker_tasks[i] = nullptr;
			worker_running[i] = true;

			// start the worker thread
			workers[i] = std::thread([this, i]() {
				while(worker_running[i])
				{
					worker_locks[i].lock();

					// execute task
					if (worker_tasks[i])
					{
						worker_tasks[i]();
						worker_tasks[i] = nullptr;
					}

					// reinsert self into idle worker queue
					idle_workers.push_front(i);
				}
			});

			// start as idle
			idle_workers.push_front(i);
		}
	}

	~thread_pool()
	{
		for (unsigned i = 0; i < POOL_SIZE; i++)
		{
			worker_running[i] = false;
			worker_locks[i].unlock();

			if (workers[i].joinable()) { workers[i].join(); }
		}
	}

	bool run(std::function<void(void)> task)
	{
		if (idle_workers.size() == 0) { return false; }

		auto i = idle_workers.back();
		idle_workers.pop_back();
		worker_tasks[i] = task;
		worker_locks[i].unlock();

		return true;
	}


	size_t idle_threads() { return idle_workers.size(); }
private:
	std::thread workers[POOL_SIZE];
	std::function<void(void)> worker_tasks[POOL_SIZE];
	std::mutex worker_locks[POOL_SIZE];
	bool worker_running[POOL_SIZE];

	std::deque<unsigned> idle_workers;
	std::deque<std::function<void(void)>> tasks;
};

}; // namespace proc

}; // namespace g