#pragma once

// time measurements and FPS counter
class GameTime
{
public:
	GameTime();
	enum GametimePredefs { GAMEDAY_1_MINUTE = 24 * 60, GAMEDAY_20_SECONDS = 24 * 60 * 3, GAMEDAY_10_SECONDS = 24 * 60 * 6 };
	// set how much faster a game day passes, 1 == real time, 24*60 is a one minute day
	// init needs be called before any other time method
	void initBeforePresentation(LONGLONG gamedayFactor);
private:
	// measurements:
	chrono::steady_clock::time_point now;
	chrono::steady_clock::time_point startTimePoint = chrono::steady_clock::now();
	double gamedayFactor = 0.0f;

	// real system time:
	chrono::system_clock::time_point nowAbs = chrono::system_clock::now();
public:
	// advances time, should be called once for every frame
	// not thread save - be sure to call from synchronized method (usually after presenting)
	void advanceTime();

	// get absolute number of hours (and fractions)
	// NEVER use time values as float instead of double: precision is not enough and you will get same time value for actually different times
	double getTimeAbs();

};

// Store accumulated time info and for individual frames
class ThemedTimer {
	struct TimerEntry {
		long long time;
		bool used = false;
	};
	struct TimerDesc {
		vector<TimerEntry> entries;
		int numSlots; // number of allocated TimerEntry slots
		int pos = 0;  // next input position
		long calls = 0L; // count calls to add()
		long long averageTimeBetweenMicroS; // accumulate time between calls in [microseconds] (1/1000 000 s)
		long long totalTimeMicros = 0; 
		chrono::steady_clock::time_point now;
	};

public:
	//singleton:
	static ThemedTimer* getInstance() {
		static ThemedTimer singleton;
		return &singleton;
	};

	// create topic with name and number of individual slots 
	void create(string name, int slots) {
		TimerDesc td;
		for (int i = 0; i < slots; i++) {
			TimerEntry e;
			td.entries.push_back(e);
		}
		td.numSlots = slots;
		td.pos = 0;
		timerMap[name] = td;
	};

	void add(string name) {
		if (!check_name(name)) return;
		auto& td = timerMap[name];
		chrono::steady_clock::time_point current = chrono::steady_clock::now();
		auto microsSinceLast = chrono::duration_cast<chrono::microseconds>(current - td.now).count();
		td.now = current;
		//Log(" micro since last " << microsSinceLast << endl);
		//Log(" micro since last " << td.totalTimeMicros << endl);
		add(td, microsSinceLast);
	}
		
	int usedSlots(string name) {
		if (!check_name(name)) return -1;
		auto& td = timerMap[name];
		int count = 0;
		for (auto& t : td.entries) {
			if (t.used) count++;
		}
		return count;
	}

	// log all slots 
	void logEntries(string name) {
		if (!check_name(name)) return;
		auto& td = timerMap[name];
		// we start at next insert position, then log whole buffer from here to getnewest entry last
		for (int i = td.pos; i <= td.pos + td.numSlots; i++) {
			int checkPos = i % td.numSlots;
			auto& t = td.entries[checkPos];
			if (t.used) {
				Log("" << t.time << endl);
			}
		}
	}

	// log accumulated info
	void logInfo(string name) {
		if (!check_name(name)) return;
		auto& td = timerMap[name];
		Log("ThemedTimer: " << name.c_str() << endl);
		Log("  #calls: " << td.calls << " average time: " << td.averageTimeBetweenMicroS << " [microseconds] / " << (td.averageTimeBetweenMicroS / 1000) << " [ms]" << endl);
	}

	// log ThemedTimer as FPS
	void logFPS(string name) {
		if (!check_name(name)) return;
		auto& td = timerMap[name];
		double fps = 1000000.0 / (double)td.averageTimeBetweenMicroS;
		double totalSeconds = ((double)td.totalTimeMicros) / 1000000;
		Log("   FPS: " << fps << " over " << totalSeconds << " [s]" << endl);
	}

private:
	// add timer value, first call does not account for average timings
	void add(TimerDesc &td, long long value) {
		td.calls++;
		if (td.calls == 1) {
			td.averageTimeBetweenMicroS = 0;
		}
		else {
			td.averageTimeBetweenMicroS = (td.averageTimeBetweenMicroS * (td.calls - 2) + value) / (td.calls - 1);
			td.totalTimeMicros += value;
		}
		TimerEntry& t = td.entries[td.pos];
		t.time = value;
		t.used = true;
		td.pos++;
		if (td.pos >= td.numSlots) {
			td.pos = 0;
		}
	};

	bool check_name(string name) {
		if (timerMap.count(name) <= 0) {
			// theme not found
			Log("tried to access themed timer that does not exist: " << name.c_str() << endl);
			return false;
		}
		return true;
	}
	unordered_map<string, TimerDesc> timerMap;
	ThemedTimer() {};								// prevent creation outside this class
	ThemedTimer(const ThemedTimer&);				// prevent creation via copy-constructor
	ThemedTimer& operator = (const ThemedTimer&);	// prevent instance copies
};


