#pragma once
#include <mutex>
namespace UTIL
{
void lock(std::mutex& mutex, const char* msg);
void unlock(std::mutex& mutex, const char* msg);

}