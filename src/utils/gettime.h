#pragma once
#include <chrono>
using timePoint = std::chrono::system_clock::time_point;
namespace UTIL {
	timePoint getTime();
	int getmillisec(timePoint t1, timePoint t2);
}