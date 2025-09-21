#include <chrono>

static float GetCurrentMillis()
{
	static long long unsigned int startTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	long long unsigned int currentTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	return (float)(currentTimestamp - startTimestamp);
}