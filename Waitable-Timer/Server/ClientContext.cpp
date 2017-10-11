#include "ClientContext.h"
#include <stdio.h>
#include <string.h>

const std::string ClientContext::PREFIX = "Global\\";

int ClientContext::ProcessMessage()
{
    if( MessageProcessed == true )
    {
        return -1;
    }
    ClientToServerTimer received;
    int size = received.GetPacketSize();
    ClientToServerTimer::DeserializeMessage( received, *Buffer );

    ServerToClientTimer reply;
    switch( received.RequestedOperation )
    {
        case ClientToServerTimer::Operation::TIMER_CREATE:
            reply = createTimer( received );
            break;
        case ClientToServerTimer::Operation::TIMER_SET:
            reply = setTimer( received );
            break;
        case ClientToServerTimer::Operation::TIMER_WAIT:
            reply = waitTimer( received );
            break;
        case ClientToServerTimer::Operation::TIMER_GET:
            reply = getTimer( received );
            break;
        case ClientToServerTimer::Operation::TIMER_DELETE:
            reply = deleteTimer( received );
            break;
        case ClientToServerTimer::Operation::TIMER_CANCEL:
            reply = cancelTimer( received );
            break;
        case ClientToServerTimer::Operation::TIMER_WELCOME:
            reply = welcomeMessage( received );
            break;
        default:
            return -1;
            break;
    }
    ResetWSABuffer();
    ServerToClientTimer::SerializeMessage( reply, *Buffer );

    MessageProcessed = true;
    return size;
}

ServerToClientTimer ClientContext::createTimer( const ClientToServerTimer & msg )
{
    printf( "[%d] Try to create Timer \"%s\"... ", GetCurrentThreadId(), msg.GetName().c_str() );
    std::map<std::string, WaitableTimer*>::iterator it;
    ServerToClientTimer ret;

    it = localTimers.find( msg.GetName() );
    if( it == localTimers.end() )
    {
        std::string s = msg.GetName();
        LPCSTR arg = s.c_str();
        HANDLE h = CreateWaitableTimerA( NULL, TRUE, arg );
        if( h == NULL )
        {
            printf( "FAILED\n" );
            ret.Callback = ServerToClientTimer::StatusCode::STATUS_ERROR;
            ret.SetMessage( "Cannot create system object timer!" );
        }
        else
        {
            if( ERROR_ALREADY_EXISTS == GetLastError() )
            {
                printf( "FAILED\n" );
                ret.Callback = ServerToClientTimer::StatusCode::STATUS_ERROR;
                ret.SetMessage( "There already exists timer object with this name! Try another one!\0" );
                CloseHandle( h );
            }
            else
            {
                WaitableTimer* w = new WaitableTimer( h );
                localTimers[msg.GetName()] = w;
                printf( "SUCCESSFUL\n" );
                ret.Callback = ServerToClientTimer::StatusCode::STATUS_SUCCESS;
                ret.SetMessage( "Timer object created successfully." );
            }
        }
    }
    else
    {
        printf( "FAILED\n" );
        ret.Callback = ServerToClientTimer::StatusCode::STATUS_ERROR;
        ret.SetMessage( "There already exists timer object with this name! Try another one." );
    }
    return ret;
}

ServerToClientTimer ClientContext::setTimer( const ClientToServerTimer & msg )
{
    printf( "[%d] Try to create Timer \"%s\"... ", GetCurrentThreadId(), msg.GetName().c_str() );
    std::map<std::string, WaitableTimer*>::iterator it;
    ServerToClientTimer ret;

    it = localTimers.find( msg.GetName() );
    if( it == localTimers.end() )
    {
        printf( "FAILED\n" );
        ret.SetMessage( "There is no timer object with this name." );
        ret.Callback = ServerToClientTimer::StatusCode::STATUS_ERROR;
    }
    else
    {
        if( (*it).second->SetTimer( msg.duration, msg.period ) )
        {
            printf( "SUCCESSFUL\n" );
            ret.SetMessage( "Timer object set successfully." );
            ret.Callback = ServerToClientTimer::StatusCode::STATUS_SUCCESS;
        }
        else
        {
            printf( "FAILED\n" );
            ret.SetMessage( "Cannot set this timer." );
            ret.Callback = ServerToClientTimer::StatusCode::STATUS_ERROR;
        }
    }
        return ret;
}

ServerToClientTimer ClientContext::waitTimer( const ClientToServerTimer & msg )
{
    printf( "[%d] Try to wait on Timer \"%s\"... ", GetCurrentThreadId(), msg.GetName().c_str() );
    std::map<std::string, WaitableTimer*>::iterator it;
    ServerToClientTimer ret;


    it = localTimers.find( msg.GetName() );
    if( it == localTimers.end() )
    {
        printf( "FAILED\n" );
        ret.SetMessage( "There is no timer object with this name." );
        ret.Callback = ServerToClientTimer::StatusCode::STATUS_ERROR;
    }
    else
    {
        DWORD resultt = (*it).second->Wait( msg.duration );
        if( resultt == WAIT_OBJECT_0 )
        {
            printf( "SUCCESSFUL\n" );
            ret.SetMessage( "RING! RING! RING! Successful." );
            ret.Callback = ServerToClientTimer::StatusCode::STATUS_SUCCESS;
        }
        else if( resultt == WAIT_ABANDONED )
        {
            printf( "FAILED\n" );
            ret.SetMessage( "Waiting was abandoned." );
            ret.Callback = ServerToClientTimer::StatusCode::STATUS_ERROR;
        }
        else if( resultt == WAIT_TIMEOUT )
        {
            printf( "FAILED\n" );
            ret.SetMessage( "TIMEOUT." );
            ret.Callback = ServerToClientTimer::StatusCode::STATUS_ERROR;
        }
        else
        {
            printf( "FAILED\n" );
            ret.SetMessage( "Waiting failed." );
            ret.Callback = ServerToClientTimer::StatusCode::STATUS_ERROR;
        }
    }
    return ret;
}

ServerToClientTimer ClientContext::getTimer( const ClientToServerTimer & msg )
{
    printf( "[%d] Try to get Timer... ", GetCurrentThreadId() );
    std::map<std::string, WaitableTimer*>::iterator it;
    ServerToClientTimer ret;

    it = localTimers.find( msg.GetName() );
    if( it != localTimers.end() )
    {
        printf( "FAILED\n" );
        ret.SetMessage( "You already have this timer." );
        ret.Callback = ServerToClientTimer::StatusCode::STATUS_ERROR;
    }
    else
    {
        HANDLE h = OpenWaitableTimerA( TIMER_ALL_ACCESS, FALSE, msg.GetName().c_str() );
        if( h != NULL )
        {
            WaitableTimer* w = new WaitableTimer( h );
            localTimers[msg.GetName()] = w;
            printf( "SUCCESSFUL\n" );
            ret.SetMessage("Timer object loaded successfully." );
            ret.Callback = ServerToClientTimer::StatusCode::STATUS_SUCCESS;
        }
        else
        {
            printf( "FAILED\n" );
            ret.SetMessage( "Cannot load timer object!" );
            ret.Callback = ServerToClientTimer::StatusCode::STATUS_ERROR;
        }
    }
    return ret;
}

ServerToClientTimer ClientContext::deleteTimer( const ClientToServerTimer & msg )
{
    printf( "[%d] Try to delete Timer \"%s\"... ", GetCurrentThreadId(), msg.GetName().c_str() );
    std::map<std::string, WaitableTimer*>::iterator it;
    ServerToClientTimer ret;

    it = localTimers.find( msg.GetName() );
    if( it == localTimers.end() )
    {
        printf( "FAILED\n" );
        ret.SetMessage( "There is no timer object with this name." );
        ret.Callback = ServerToClientTimer::StatusCode::STATUS_ERROR;
    }
    else
    {
        delete (*it).second;
        (*it).second = NULL;
        localTimers.erase( it );
        printf( "SUCCESSFUL\n" );
        ret.SetMessage( "Timer was deleted successfully." );
        ret.Callback = ServerToClientTimer::StatusCode::STATUS_SUCCESS;
    }
    return ret;
}

ServerToClientTimer ClientContext::cancelTimer( const ClientToServerTimer & msg )
{
    printf( "[%d] Try to cancel Timer \"%s\"... ", GetCurrentThreadId(), msg.GetName().c_str() );
    std::map<std::string, WaitableTimer*>::iterator it;
    ServerToClientTimer ret;

    it = localTimers.find( msg.GetName() );
    if( it == localTimers.end() )
    {
        printf( "FAILED\n" );
        ret.SetMessage( "There is no timer object with this name." );
        ret.Callback = ServerToClientTimer::StatusCode::STATUS_SUCCESS;
    }
    else
    {
        if( (*it).second->CancelTimer() )
        {
            printf( "SUCCESSFUL\n" );
            ret.SetMessage( "Timer was canceled successfully." );
            ret.Callback = ServerToClientTimer::StatusCode::STATUS_SUCCESS;
        }
        else
        {
            printf( "FAILED\n" );
            ret.SetMessage( "Cannot cancel this timer!" );
            ret.Callback = ServerToClientTimer::StatusCode::STATUS_SUCCESS;
        }
    }
    return ret;
}

ServerToClientTimer ClientContext::welcomeMessage( const ClientToServerTimer & msg )
{
    ServerToClientTimer reply;
    reply.SetMessage( "Welcome" );
    reply.Callback = ServerToClientTimer::StatusCode::STATUS_SUCCESS;
    return reply;
}

void ClientContext::prefixEnrich( ClientToServerTimer & msg )
{
    std::string newname = PREFIX + msg.GetName();
    msg.SetName( newname );
}

void ClientContext::SetInitialMessage()
{
    ServerToClientTimer reply;
    reply.Callback = ServerToClientTimer::StatusCode::STATUS_SUCCESS;
    reply.SetMessage( "Welcome" );
    ServerToClientTimer::SerializeMessage( reply, *Buffer );
}
