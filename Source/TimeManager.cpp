#include "TimeManager.h"
#include <windows.h>
#include <cstdlib>

namespace awesome {

	TimeManager::TimeManager() {
		LARGE_INTEGER Frequency;
		if (!QueryPerformanceFrequency(&Frequency))
			exit(-1); // Hardware does not support high-res counter
		timerFrequency = Frequency.QuadPart;
	}

	unsigned long long TimeManager::Tick() {
		unsigned long long prevTime = currentTime;
		LARGE_INTEGER perfCount;
		QueryPerformanceCounter(&perfCount);
		currentTime = perfCount.QuadPart;
		unsigned long long dt = (currentTime - prevTime) * 1000ULL / timerFrequency;
		return dt;
	}

	unsigned long long TimeManager::GetCurrentTimeTicks() const { 
		return currentTime;
	}

	unsigned long long TimeManager::GetCurrentTimeMs() const { 
		return currentTime / timerFrequency;
	}

	float TimeManager::GetCurrentTimeSec() const { 
		return static_cast<float> (currentTime / (timerFrequency * 1000.0f));
	}

} // namespace awesome
