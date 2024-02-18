#include "gettime.h"

timePoint UTIL::getTime()
{
	return std::chrono::system_clock().now();
}

int UTIL::getmillisec(timePoint t1, timePoint t2)
{
	std::chrono::system_clock::duration duration;
	if (t1 < t2) {
		duration = t2 - t1;
	}
	else {
		duration = t1 - t2;
	}
	return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();

}
