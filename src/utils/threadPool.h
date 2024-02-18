#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class ThreadPool {
public:
	ThreadPool(int maxThread);
	
	~ThreadPool();

	void						run();

	void						addTask(T* task);

private:
	bool						_exit;

	std::vector<std::thread>	_workers;

	std::queue<T*>				_taskQue;

	std::mutex					_taskMux;

	std::condition_variable		_taskCond;
};

template<typename T>
ThreadPool<T>::ThreadPool(int maxThread) {
	_exit = false;
	for (int i = 0; i < maxThread; ++i) {
		_workers.emplace_back(&ThreadPool::run, this);
	}
}

template<typename T>
ThreadPool<T>::~ThreadPool()
{
	_taskMux.lock();
	_exit = true;
	_taskMux.unlock();
	_taskCond.notify_all();
	for (auto& worker : _workers) {
		if (worker.joinable()) {
			worker.join();
		}
	}
}

template<typename T>
void
ThreadPool<T>::run()
{
	while (!_exit) {
		std::unique_lock<std::mutex> lock(_taskMux);
		while (_taskQue.empty() && !_exit) {
			_taskCond.wait(lock);
		}
		if (!_exit) {
			T* task = _taskQue.front();
			_taskQue.pop();
			lock.unlock();
			task->run();
		}
	}
}

template<typename T>
void
ThreadPool<T>::addTask(T* task)
{
	std::unique_lock<std::mutex> lock(_taskMux);
	_taskQue.push(task);
	lock.unlock();
	_taskCond.notify_one();
}