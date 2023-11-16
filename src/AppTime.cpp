#include "pch.hpp"
#include "AppTime.h"

Apparatus::Time::Time() :
    _secondsPerCount{0},_deltaTime{-1},_baseTime{0},_pausedTime{0},_prevTime{0},_currTime{0},_stopTime{0}, _stopped{false}
{
    UINT64 countsPerSec{};
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
    _secondsPerCount = 1 / (double)countsPerSec;
}

float Apparatus::Time::TotalTime() const
{
    if (_stopped) {
        return (float)(((_stopTime - _pausedTime) - _baseTime) * _secondsPerCount);
    }
    else {
        return (float)(((_currTime - _pausedTime) - _baseTime) * _secondsPerCount);
    }
    return 0.0f;
}

float Apparatus::Time::DeltaTime() const
{
    return (float)_deltaTime;
}

void Apparatus::Time::Reset()
{
    _currTime = NULL;
    QueryPerformanceCounter((LARGE_INTEGER*)&_currTime);
    _baseTime = _currTime;
    _prevTime = _currTime;
    _stopTime = NULL;
    _stopped = false;
}

void Apparatus::Time::Start()
{
    INT64 startTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
    /* If resuming the timer from a stopped state.*/
    if (_stopped) {
        /* Then accumulate the paused time.*/
        _pausedTime += (startTime - _stopTime);
        _stopTime = NULL;
        _stopped = false;
    }
}

void Apparatus::Time::Stop()
{
    if (_stopped) { return; }
    _currTime = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&_currTime);
    _stopTime = _currTime;
    _stopped = true;
}

void Apparatus::Time::Tick()
{
    if (_stopped) { _deltaTime = NULL; return; }
    QueryPerformanceCounter((LARGE_INTEGER*)&_currTime);
    _deltaTime = (_currTime - _prevTime) * _secondsPerCount;
    _prevTime = _currTime;
    if (_deltaTime > 0) { _deltaTime = NULL; }
}
