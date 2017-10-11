#include "ClientManagement.h"

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <string.h>



const std::string ClientManagement::PREFIX = "Global\\";

ClientManagement::ClientManagement()
{
}

ClientManagement::ClientManagement( SOCKET _ClientSocket )
{
    ClientSocket = _ClientSocket;
}



ClientManagement::ClientManagement( ClientManagement const & )
{
}

ClientManagement::ClientManagement( ClientManagement && )
{
}
ClientManagement::~ClientManagement()
{
    if( threadHandle != NULL )
    {
        TerminateThread( threadHandle, 0 );
        CloseHandle( threadHandle );
        closesocket( ClientSocket );
    }
}


void ClientManagement::createTimer( const ClientToServerTimer& msg )
{
    printf( "[%d] Try to create Timer \"%s\"... ", GetCurrentThreadId(), msg.GetName().c_str() );
    std::map<std::string, WaitableTimer*>::iterator it;

    it = localTimers.find( msg.GetName() );
    if( it == localTimers.end() )
    {
        std::string s = msg.GetName();
        LPCSTR arg = s.c_str();
        HANDLE h = CreateWaitableTimerA( NULL, TRUE, arg );
        if( h == NULL )
        {
            printf( "FAILED\n" );
            SendMessage( ClientSocket, "Cannot create system object timer!", ServerToClientTimer::STATUS_ERROR );
        }
        else
        {
            if( ERROR_ALREADY_EXISTS == GetLastError() )
            {
                printf( "FAILED\n" );
                SendMessage( ClientSocket, "There already exists timer object with this name! Try another one!\0", ServerToClientTimer::STATUS_ERROR );
                CloseHandle( h );
            }
            else
            {
                WaitableTimer* w = new WaitableTimer(h);
                localTimers[msg.GetName()] = w;
                printf( "SUCCESSFUL\n" );
                SendMessage( ClientSocket, "Timer object created successfully.", ServerToClientTimer::STATUS_SUCCESS );
            }
        }
    }
    else
    {
        printf( "FAILED\n" );
        SendMessage( ClientSocket, "There already exists timer object with this name! Try another one.", ServerToClientTimer::STATUS_ERROR );
    }

}

void ClientManagement::setTimer( const ClientToServerTimer& msg )
{
    printf( "[%d] Try to create Timer \"%s\"... ", GetCurrentThreadId(), msg.GetName().c_str() );
    std::map<std::string, WaitableTimer*>::iterator it;

    it = localTimers.find( msg.GetName() );
    if( it == localTimers.end() )
    {
        printf( "FAILED\n" );
        SendMessage( ClientSocket, "There is no timer object with this name.", ServerToClientTimer::STATUS_ERROR );
    }
    else
    {
        if( (*it).second->SetTimer( msg.duration, msg.period ) )
        {
            printf( "SUCCESSFUL\n" );
            SendMessage( ClientSocket, "Timer object created successfully.", ServerToClientTimer::STATUS_SUCCESS );
        }
        else
        {
            printf( "FAILED\n" );
            SendMessage( ClientSocket, "Cannot set this timer.", ServerToClientTimer::STATUS_ERROR );
        }
    }
}

void ClientManagement::waitTimer( const ClientToServerTimer& msg )
{
    printf( "[%d] Try to wait on Timer \"%s\"... ", GetCurrentThreadId(), msg.GetName().c_str() );
    std::map<std::string, WaitableTimer*>::iterator it;

    it = localTimers.find( msg.GetName() );
    if( it == localTimers.end() )
    {
        printf( "FAILED\n" );
        SendMessage( ClientSocket, "There is no timer object with this name.", ServerToClientTimer::STATUS_ERROR );
    }
    else
    {
        DWORD resultt = (*it).second->Wait( msg.duration );
        if( resultt == WAIT_OBJECT_0 )
        {
            printf( "SUCCESSFUL\n" );
            SendMessage( ClientSocket, "RING! RING! RING! Successful.", ServerToClientTimer::STATUS_SUCCESS );

        }
        else if( resultt == WAIT_ABANDONED )
        {
            printf( "FAILED\n" );
            SendMessage( ClientSocket, "Waiting was abandoned.", ServerToClientTimer::STATUS_ERROR );

        }
        else if( resultt == WAIT_TIMEOUT )
        {
            printf( "FAILED\n" );
            SendMessage( ClientSocket, "TIMEOUT.", ServerToClientTimer::STATUS_ERROR );

        }
        else
        {
            printf( "FAILED\n" );
            SendMessage( ClientSocket, "Waiting failed.", ServerToClientTimer::STATUS_ERROR );

        }
    }
}

void ClientManagement::getTimer( const ClientToServerTimer& msg )
{
    printf( "[%d] Try to get Timer... ", GetCurrentThreadId() );
    std::map<std::string, WaitableTimer*>::iterator it;

    it = localTimers.find( msg.GetName() );
    if( it != localTimers.end() )
    {
        printf( "FAILED\n" );
        SendMessage( ClientSocket, "You already have this timer.", ServerToClientTimer::STATUS_ERROR );
    }
    else
    {
        HANDLE h = OpenWaitableTimerA( TIMER_ALL_ACCESS, FALSE, msg.GetName().c_str() );
        if( h != NULL )
        {
            WaitableTimer* w = new WaitableTimer(h);
            localTimers[msg.GetName()] = w;
            printf( "SUCCESSFUL\n" );
            SendMessage( ClientSocket, "Timer object loaded successfully.", ServerToClientTimer::STATUS_SUCCESS );
        }
        else
        {
            printf( "FAILED\n" );
            SendMessage( ClientSocket, "Cannot load timer object!", ServerToClientTimer::STATUS_ERROR );
        }
    }

}

void ClientManagement::deleteTimer( const ClientToServerTimer& msg )
{
    printf( "[%d] Try to delete Timer \"%s\"... ", GetCurrentThreadId(), msg.GetName().c_str() );
    std::map<std::string, WaitableTimer*>::iterator it;

    it = localTimers.find( msg.GetName() );
    if( it == localTimers.end() )
    {
        printf( "FAILED\n" );
        SendMessage( ClientSocket, "There is no timer object with this name.", ServerToClientTimer::STATUS_ERROR );
    }
    else
    {
        delete (*it).second;
        (*it).second = NULL;
        localTimers.erase( it );
        printf( "SUCCESSFUL\n" );
        SendMessage( ClientSocket, "Timer was deleted successfully.", ServerToClientTimer::STATUS_SUCCESS );
    }
}

void ClientManagement::cancelTimer( const ClientToServerTimer& msg )
{
    printf( "[%d] Try to cancel Timer \"%s\"... ", GetCurrentThreadId(), msg.GetName().c_str() );
    std::map<std::string, WaitableTimer*>::iterator it;

    it = localTimers.find( msg.GetName() );
    if( it == localTimers.end() )
    {
        printf( "FAILED\n" );
        SendMessage( ClientSocket, "There is no timer object with this name.", ServerToClientTimer::STATUS_ERROR );
    }
    else
    {
        if( (*it).second->CancelTimer() )
        {
            printf( "SUCCESSFUL\n" );
            SendMessage( ClientSocket, "Timer was cancelled successfully.", ServerToClientTimer::STATUS_SUCCESS );
        }
        else
        {
            printf( "FAILED\n" );
            SendMessage( ClientSocket, "Cannot cancel this timer!", ServerToClientTimer::STATUS_ERROR );
        }
    }
}

DWORD ClientManagement::Run()
{
    int iResult;

    DWORD threadID = GetCurrentThreadId();

    printf( "[%d] Client's thread started.\n", threadID );
    SendMessage( ClientSocket, "Welcome", ServerToClientTimer::ServerToClientTimer::STATUS_SUCCESS );

    // Receive until the peer shuts down the connection
    do
    {
        ClientToServerTimer message;
        iResult = ReceiveMessage( ClientSocket, message );

        if( iResult > 0 )
        {
            printf( "[%d] Message received. Bytes received: %d\n", threadID, iResult );

            prefixEnrich( message );

            switch( message.RequestedOperation )
            {
                case ClientToServerTimer::Operation::TIMER_CREATE:
                    createTimer( message );
                    break;
                case ClientToServerTimer::Operation::TIMER_SET:
                    setTimer( message );
                    break;
                case ClientToServerTimer::Operation::TIMER_WAIT:
                    waitTimer( message );
                    break;
                case ClientToServerTimer::Operation::TIMER_GET:
                    getTimer( message );
                    break;
                case ClientToServerTimer::Operation::TIMER_DELETE:
                    deleteTimer( message );
                    break;
                case ClientToServerTimer::Operation::TIMER_CANCEL:
                    cancelTimer( message );
                    break;
                default:
                    break;
            }
        }
        else if( iResult == 0 )
            printf( "[%d] Connection closing...\n", GetCurrentThreadId() );
        else
        {
            printf( "recv failed with error: %d\n", WSAGetLastError() );
            closesocket( ClientSocket );
            WSACleanup();
            return EXIT_FAILURE;
        }

    } while( iResult > 0 );

    // shutdown the connection since we're done
    iResult = shutdown( ClientSocket, SD_SEND );
    if( iResult == SOCKET_ERROR )
    {
        printf( "shutdown failed with error: %d\n", WSAGetLastError() );
        closesocket( ClientSocket );
        WSACleanup();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void ClientManagement::prefixEnrich( ClientToServerTimer & msg )
{
    std::string newname = PREFIX + msg.GetName();
    msg.SetName( newname );
}

//std::thread ClientThread::RunThread()
//{
//    return std::thread( &ClientThread::Run, this );
//}!




bool ClientManagement::SendMessage( SOCKET clientSocket, std::string message, ServerToClientTimer::StatusCode code )
{
    ServerToClientTimer reply;
    reply.SetMessage( message );
    reply.Callback = code;

    DWORD threadID = GetCurrentThreadId();

    int size = reply.GetPacketSize();
    char* msgCharPtr = new char[size];

    ServerToClientTimer::SerializeMessage( reply, *msgCharPtr );

    int iSendResult = send( clientSocket, msgCharPtr, size, 0 );
    if( iSendResult == SOCKET_ERROR )
    {
        printf( "[%d] send failed with error: %d\n", threadID, WSAGetLastError() );
        return false;
    }
    printf( "[%d] Message sent. Bytes sent: %d\n", threadID, iSendResult );
    return true;
}

int ClientManagement::ReceiveMessage( SOCKET clientSocket, ClientToServerTimer & message )
{
    int size = message.GetPacketSize();
    char* msgCharPtr = new char[size];

    DWORD threadID = GetCurrentThreadId();

    int iReceiveResult = recv( clientSocket, msgCharPtr, size, 0 );
    if( iReceiveResult == size )
    {
        ClientToServerTimer::DeserializeMessage( message, *msgCharPtr );
    }

    return iReceiveResult;
}

