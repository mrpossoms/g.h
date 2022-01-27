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
	struct task
	{
		std::function<void(void)> work = nullptr;
		std::function<void(void)> on_finish = nullptr;
	};

	struct worker
	{
		bool running;
		std::thread thread;
		std::mutex trigger;
		task current_task;
	};

	thread_pool()
	{
		for (unsigned i = 0; i < POOL_SIZE; i++)
		{
			// prevent the worker from looping without a task
			workers[i].trigger.lock();
			workers[i].running = true;

			// start the worker thread
			workers[i].thread = std::thread([this, i]() {
				auto& me = workers[i];
				while(me.running)
				{
					me.trigger.lock();

					// execute task
					if (me.current_task.work)
					{
						me.current_task.work();
						me.current_task.work = nullptr;
					}

					// if a dispatch was provided execute here
					if (me.current_task.on_finish)
					{
						std::scoped_lock lock(finish_mutex);
						pending_finishes.push_front(me.current_task.on_finish);
					}

					{ // manage new tasks or going dormant
						std::scoped_lock lock(task_mutex);

						// try to grab a new task
						if (pending_tasks.size() > 0)
						{
							me.current_task = pending_tasks.back();
							pending_tasks.pop_back();

							me.trigger.unlock();
						}
						else
						{   // no more work to do.
							// reinsert self into idle worker queue
							std::scoped_lock lock(idle_worker_mutex);
							idle_workers.push_front(i);
						}
					}
				}
			});

			// start as idle
			idle_workers.push_front(i);
		}
	}

	~thread_pool()
	{
		pending_tasks.clear();

		for (unsigned i = 0; i < POOL_SIZE; i++)
		{
			workers[i].running = false;
			workers[i].trigger.unlock();

			if (workers[i].thread.joinable()) { workers[i].thread.join(); }
		}
	}

	void run(std::function<void(void)> task, std::function<void(void)> on_finish=nullptr)
	{
		{ // try to start work immediately if there is a free worker
			std::scoped_lock lock(idle_worker_mutex);
			if (idle_workers.size() > 0)
			{
				auto i = idle_workers.back();
				idle_workers.pop_back();

				workers[i].current_task = { task, on_finish };
				workers[i].trigger.unlock();

				return;
			}
		}

		{ // otherwise, queue the task for the next available
			std::scoped_lock lock(task_mutex);
			pending_tasks.push_front(thread_pool::task{task, on_finish});
		}
	}

	void update()
	{
		std::scoped_lock lock(finish_mutex);
		while (pending_finishes.size() > 0)
		{
			pending_finishes.back()();
			pending_finishes.pop_back();
		}
	}

	size_t idle_threads() { return idle_workers.size(); }
private:
	thread_pool::worker workers[POOL_SIZE];

	std::mutex idle_worker_mutex;
	std::deque<unsigned> idle_workers;

	std::mutex task_mutex;
	std::deque<task> pending_tasks;

	std::mutex finish_mutex;
	std::deque<std::function<void(void)>> pending_finishes;
};

}; // namespace proc

}; // namespace g