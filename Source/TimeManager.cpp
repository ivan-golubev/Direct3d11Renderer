#include "TimeManager.h"
#include <windows.h>
#include <cstdlib>
#include <algorithm>

namespace awesome {

	TimeManager::TimeManager() {
		LARGE_INTEGER Frequency;
		if (!QueryPerformanceFrequency(&Frequency))
			exit(-1); // Hardware does not support high-res counter
		ticksPerSec = Frequency.QuadPart;

		LARGE_INTEGER perfCount;
		QueryPerformanceCounter(&perfCount);
		startTimeTicks = currentTimeTicks = perfCount.QuadPart;
	}

	unsigned long long TimeManager::Tick() {
		unsigned long long prevTimeTicks = currentTimeTicks;
		LARGE_INTEGER perfCount;
		QueryPerformanceCounter(&perfCount);
		currentTimeTicks = perfCount.QuadPart;
		unsigned long long deltaMs = (currentTimeTicks - prevTimeTicks) * 1000ULL / ticksPerSec;
		//deltaMs = std::max(deltaMs, 7ULL); /* make sure the first frame is at least 7 ms (144 FPS) */
		return deltaMs;
	}

	unsigned long long TimeManager::GetCurrentTimeTicks() const { 
		return currentTimeTicks - startTimeTicks;
	}

	unsigned long long TimeManager::GetCurrentTimeMs() const { 
		return GetCurrentTimeTicks() * 1000ULL / ticksPerSec;
	}

	float TimeManager::GetCurrentTimeSec() const {
		return static_cast<float> (GetCurrentTimeTicks() / ticksPerSec);
	}

} // namespace awesome
