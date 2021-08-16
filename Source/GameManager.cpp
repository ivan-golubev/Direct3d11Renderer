#include "GameManager.h"
#include <windows.h>
#include <cstdlib>

namespace awesome {

	GameManager::GameManager() {
		LARGE_INTEGER Frequency;
		if (!QueryPerformanceFrequency(&Frequency))
			exit(-1); // Hardware does not support high-res counter
		timerFrequency = Frequency.QuadPart;
	}

	unsigned long long GameManager::CalculateDeltaTimeMs() {
		unsigned long long prevTime = currentTime;
		LARGE_INTEGER perfCount;
		QueryPerformanceCounter(&perfCount);
		currentTime = perfCount.QuadPart;
		unsigned long long dt = (currentTime - prevTime) * 1000ULL / timerFrequency;
		return dt;
	}

	unsigned long long GameManager::GetCurrentTimeTicks() const { 
		return currentTime;
	}

	unsigned long long GameManager::GetCurrentTimeMs() const { 
		return currentTime / timerFrequency;
	}

	float GameManager::GetCurrentTimeSec() const { 
		return static_cast<float> (currentTime / (timerFrequency * 1000.0f));
	}

} // namespace awesome
