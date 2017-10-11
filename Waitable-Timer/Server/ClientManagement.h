#pragma once

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <map>
#include "protocol.h"
#include "WaitableTimer.h"
//#include <thread>

static DWORD WINAPI InitializeThread( LPVOID parameter );

class ClientManagement
{
private:
    SOCKET ClientSocket;
    std::map<std::string, WaitableTimer*> localTimers;
    HANDLE threadHandle;

    void createTimer( const ClientToServerTimer& msg );
    void setTimer( const ClientToServerTimer& msg );
    void waitTimer( const ClientToServerTimer& msg );
    void getTimer( const ClientToServerTimer& msg );
    void deleteTimer( const ClientToServerTimer& msg );
    void cancelTimer( const ClientToServerTimer& msg );
    void prefixEnrich( ClientToServerTimer& msg );
public:
    static bool SendMessage( SOCKET clientSocket, std::string message, enum ServerToClientTimer::StatusCode code );
    static int ReceiveMessage( SOCKET clientSocket, ClientToServerTimer & message );
    DWORD Run();
    ClientManagement();
    ClientManagement( SOCKET _ClientSocket );
    ClientManagement( ClientManagement const& );
    ClientManagement( ClientManagement && );
    HANDLE startMyThread()
    {
        DWORD nThreadID;
        threadHandle = CreateThread( NULL, 0, InitializeThread, reinterpret_cast< LPVOID >(this), 0, &nThreadID );
        if( threadHandle == NULL )
        {
            throw EXIT_FAILURE;
        }
        return threadHandle;
    }
    ~ClientManagement();

    const static std::string PREFIX;
};

static DWORD WINAPI InitializeThread( LPVOID parameter )
{
    ClientManagement* t = reinterpret_cast< ClientManagement* >(parameter);
    t->Run();
    return 0;
}