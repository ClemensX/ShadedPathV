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
}

void GameTime::advanceTime()
{
	now = chrono::steady_clock::now();
}

double GameTime::getTimeAbs()
{
	// hours of day:
	auto dp = floor<chrono::days>(nowAbs);
	chrono::duration<double, std::ratio<60 * 60>> myHourTick2(nowAbs - dp);
	double hh = myHourTick2.count();
	//Log("fraction " << hh << endl);
	return hh;
}