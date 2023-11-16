#pragma once
#include <Windows.h>
namespace Apparatus {
	class Time
	{
	public:
		Time();
		float TotalTime()const;
		float DeltaTime()const;
		void Reset();
		void Start();
		void Stop();
		void Tick();
	private:
		double _secondsPerCount;
		double _deltaTime;
		UINT64 _baseTime;
		UINT64 _pausedTime;
		UINT64 _stopTime;
		UINT64 _prevTime;
		UINT64 _currTime;
		bool _stopped;
	};
}


