#include "lock.h"
#include <iostream>

void UTIL::lock(std::mutex& mutex, const char* msg)
{
#ifdef DEBUG
	std::thread::id id = std::this_thread::get_id();
	std::cout << id << " locking... " << std::endl;
	std::cout << msg << std::endl;
#endif // DEBUG
	mutex.lock();
}

void UTIL::unlock(std::mutex& mutex, const char* msg)
{
#ifdef DEBUG
	std::thread::id id = std::this_thread::get_id();
	std::cout << id << " unlocking " << std::endl;
	std::cout << msg << std::endl;
#endif //DEBUG
	mutex.unlock();
}