#include "WaitableTimer.h"

WaitableTimer::WaitableTimer()
{
}

WaitableTimer::WaitableTimer( HANDLE TimerHandle ) :
    timerHandle( TimerHandle )
{
}

WaitableTimer::~WaitableTimer()
{
    CloseHandle( timerHandle );
}

bool WaitableTimer::SetTimer( DWORD64 duration, DWORD period )
{
    LARGE_INTEGER i;
    i.QuadPart = duration * -10000LL;
    LONG aperiod = period;
    if( !SetWaitableTimer( timerHandle, &i, 0, NULL, NULL, FALSE ) )
        return false;
    return true;
}

DWORD WaitableTimer::Wait( DWORD duration )
{
    DWORD ret = WaitForSingleObject( timerHandle, duration );
    return ret;

}

bool WaitableTimer::CancelTimer()
{
    if( !CancelWaitableTimer( timerHandle ) )
        return false;
    return true;
}

