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
	now = chrono::steady_clock::now();
}

void GameTime::advanceTime()
{
	// get time
	auto old = now;
	now = chrono::steady_clock::now();
	nanoTime = now.time_since_epoch().count();
	chrono::system_clock::time_point nowAbs = chrono::system_clock::now();

	// hours of day:
	auto dp = floor<chrono::days>(nowAbs);
	// chrono::hours == ratio<60 *60>
	chrono::duration<double, std::ratio<60 * 60>> myHourTick2(nowAbs - dp);
	timeSystemClock = myHourTick2.count();
	//Log("fraction " << hh << endl);
	timeGameClock = fmod(timeSystemClock * gamedayFactor, 24.0);

	chrono::duration<double, std::ratio<60 * 60>> myHourTick3(now - this->startTimePoint);
	realtime = myHourTick3.count();

	gametime = realtime * gamedayFactor;
	// seconds is standard - we don't need ratio
	chrono::duration<double> mySecondsTick(now - this->startTimePoint);
	gametimeSeconds = mySecondsTick.count() * gamedayFactor;

	timeDelta = chrono::duration_cast<chrono::duration<double>>(now - old).count();
	gameTimeDelta = timeDelta * gamedayFactor;
}

double GameTime::getTimeSystemClock()
{
	return timeSystemClock;
}

double GameTime::getTimeGameClock()
{
	return timeGameClock;
}

double GameTime::getTime()
{
	return gametime;
}

double GameTime::getTimeSeconds()
{
	return gametimeSeconds;
}

double GameTime::getTimeDelta()
{
	return gameTimeDelta;
}

double GameTime::getRealTimeDelta()
{
	return timeDelta;
}

long long GameTime::getNanoTime()
{
	return nanoTime;
}

ThemedTimer ThemedTimer::singleton;