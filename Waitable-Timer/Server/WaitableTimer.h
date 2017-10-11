#pragma once

#include <windows.h>

class WaitableTimer
{
    HANDLE timerHandle;
public:
    WaitableTimer();
    WaitableTimer( HANDLE TimerHandle );
    ~WaitableTimer();
    bool SetTimer( DWORD64 duration, DWORD period );
    DWORD Wait( DWORD duration );
    bool CancelTimer();
};

