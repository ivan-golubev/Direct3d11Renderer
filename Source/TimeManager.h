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
		unsigned long long timerFrequency{0};
		unsigned long long currentTime{0}; // in ticks
	};

} // awesome