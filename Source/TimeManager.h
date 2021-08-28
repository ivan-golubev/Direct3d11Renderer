#pragma once

namespace awesome {

	class TimeManager {
	public:
		TimeManager();
		unsigned long long Tick();
		unsigned long long GetCurrentTimeTicks() const;
		unsigned long long GetCurrentTimeMs() const;
		float GetCurrentTimeSec() const;
	private:
		unsigned long long ticksPerSec{0};
		unsigned long long currentTimeTicks{0};
		unsigned long long startTimeTicks{ 0 };
	};

} // awesome