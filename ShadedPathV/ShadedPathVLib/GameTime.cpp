#include "pch.h"

GameTime::GameTime()
{
	// get tick per second:
	//ratio period = chrono::steady_clock::period(); // time in seconds between ticks
	//double ticks_per_sec = (double)period.num / (double) period.den;
	//Log("Realtime Tick per Second: " << ticks_per_sec << endl);
}

void GameTime::init(LONGLONG gamedayFactor)
{
	this->gamedayFactor = (double)gamedayFactor;
	//time_t tt = chrono::system_clock::to_time_t(nowAbs);
	//auto t = ctime(&tt);
	//Log("time: " << t << endl);
	chrono::duration d_epoch = nowAbs.time_since_epoch();
	chrono::duration<double, std::ratio<60*60>> myHourTick(d_epoch);
	auto h = myHourTick.count();
	Log("fraction " << h << endl);

	// hours of day:
	auto dp = floor<chrono::days>(nowAbs);
	chrono::duration<double, std::ratio<60 * 60>> myHourTick2(nowAbs - dp);
	double hh = myHourTick2.count();
	Log("fraction " << hh << endl);

	//Log("days " << dp. << endl);
	chrono::hh_mm_ss time{ floor<chrono::milliseconds>(nowAbs - dp) };
	auto h2 = time.hours();
	Log("hours " << h2 << endl);
	auto h3 = floor<chrono::hours>(nowAbs - dp);
	Log("hours " << h3 << endl);
}

void GameTime::advanceTime()
{
	now = chrono::steady_clock::now();
}