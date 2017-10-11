#pragma once
#include <winsock2.h>
#include <map>
#include "protocol.h"
#include "WaitableTimer.h"

#define MAX_BUF_LEN 512

struct ClientContext 
{
private:

    std::map<std::string, WaitableTimer*> localTimers;

    ServerToClientTimer createTimer( const ClientToServerTimer& msg );
    ServerToClientTimer setTimer( const ClientToServerTimer& msg );
    ServerToClientTimer waitTimer( const ClientToServerTimer& msg );
    ServerToClientTimer getTimer( const ClientToServerTimer& msg );
    ServerToClientTimer deleteTimer( const ClientToServerTimer& msg );
    ServerToClientTimer cancelTimer( const ClientToServerTimer& msg );
    ServerToClientTimer welcomeMessage( const ClientToServerTimer& msg );
    void prefixEnrich( ClientToServerTimer& msg );

public:
    bool MessageProcessed = false;
    int ProcessMessage();

    void SetInitialMessage();
    
    const static std::string PREFIX;

    OVERLAPPED *Overlapped;
    WSABUF *WSA_buffer;

    int total_bytes;
    int sent_bytes;

    SOCKET Socket; 
    enum ACTION
    {
        READ,
        WRITE
    }Action;

    char Buffer[MAX_BUF_LEN];

    void ZeroBuffer()
    {
        ZeroMemory( Buffer, MAX_BUF_LEN );
    }
    
    void ResetWSABuffer()
    {
        ZeroBuffer();
        WSA_buffer->buf = Buffer;
        WSA_buffer->len = MAX_BUF_LEN;
    }

    ClientContext()
    {
        Overlapped = new OVERLAPPED;
        WSA_buffer = new WSABUF;

        ZeroMemory( Overlapped, sizeof( OVERLAPPED ) );

        Socket = SOCKET_ERROR;

        ZeroMemory( Buffer, MAX_BUF_LEN );

        WSA_buffer->buf = Buffer;
        WSA_buffer->len = MAX_BUF_LEN;

        Action = ACTION::READ;
        total_bytes = 0;
        sent_bytes = 0;
    }

    ~ClientContext()
    {
        while( !HasOverlappedIoCompleted( Overlapped ) )
        {
            Sleep( 0 );
        }

        closesocket( Socket );

        delete Overlapped;
        delete WSA_buffer;
    }
};
