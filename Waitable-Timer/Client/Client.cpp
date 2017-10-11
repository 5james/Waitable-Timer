#include "Client.h"

#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>


Client::Client()
{
    ConnectSocket = INVALID_SOCKET;
}


Client::~Client()
{
    if( ConnectSocket != INVALID_SOCKET )
    {
        Cleanup();

    }
}


bool Client::Initialize()
{
    std::locale::global( std::locale( "" ) );

    WSADATA wsaData;
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    int iResult;


    // Initialize Winsock
    iResult = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
    if( iResult != 0 )
    {
        printf( "WSAStartup failed with error: %d\n", iResult );
        return false;
    }

    ZeroMemory( &hints, sizeof( hints ) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo( "127.0.0.1", DEFAULT_PORT, &hints, &result );
    if( iResult != 0 )
    {
        printf( "getaddrinfo failed with error: %d\n", iResult );
        WSACleanup();
        return false;
    }

    // Attempt to connect to an address until one succeeds
    for( ptr = result; ptr != NULL; ptr = ptr->ai_next )
    {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket( ptr->ai_family, ptr->ai_socktype,
                                ptr->ai_protocol );
        if( ConnectSocket == INVALID_SOCKET )
        {
            printf( "socket failed with error: %ld\n", WSAGetLastError() );
            WSACleanup();
            return false;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, ( int ) ptr->ai_addrlen );
        if( iResult == SOCKET_ERROR )
        {
            closesocket( ConnectSocket );
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo( result );

    if( ConnectSocket == INVALID_SOCKET )
    {
        printf( "Unable to connect to server!\n" );
        WSACleanup();
        return false;
    }
    return true;
}

bool Client::receiveMessage()
{
    ServerToClientTimer msg;
    int iResult;
    int size = msg.GetPacketSize();
    char *msgCharPtr = new char[size];


    iResult = recv( ConnectSocket, msgCharPtr, size, 0 );
    if( iResult > 0 )
    {
        ServerToClientTimer::DeserializeMessage( msg, *msgCharPtr );
        printf( "[%s]Response received: %s\n\n",
            (msg.Callback == ServerToClientTimer::StatusCode::STATUS_SUCCESS ? "SUCCESS" : "ERROR"),
                msg.GetMessage().c_str() );
    }
    else if( iResult == 0 )
    {
        printf( "Connection closed\n" );
        return false;
    }
    else
    {
        printf( "recv failed with error: %d\n", WSAGetLastError() );
        return false;
    }

    return true;
}

bool Client::receiveWelcomeMessage()
{
    printf( "Trying to connect... (5 sec)\n" );
    fd_set set;
    struct timeval timeout;
    FD_ZERO( &set ); /* clear the set */
    FD_SET( ConnectSocket, &set ); /* add our file descriptor to the set */
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    int rv = select( ConnectSocket + 1, &set, NULL, NULL, NULL );
    if( rv == SOCKET_ERROR )
    {
        printf( "SERVER FAILURE\n" );
        Cleanup();
        return false;
    }
    else if( rv == 0 )
    {
        printf( "SERVER FAILURE\n" );
        Cleanup();
        return false;
    }
    else
    {
        ServerToClientTimer msg;
        int iResult;
        int size = msg.GetPacketSize();
        char *msgCharPtr = new char[size];


        iResult = recv( ConnectSocket, msgCharPtr, size, 0 );
        if( iResult > 0 )
        {
            ServerToClientTimer::DeserializeMessage( msg, *msgCharPtr );
            printf( "[%s]Response received: %s\n\n",
                (msg.Callback == ServerToClientTimer::StatusCode::STATUS_SUCCESS ? "SUCCESS" : "ERROR"),
                    msg.GetMessage().c_str() );
        }
        else if( iResult <= 0 || msg.Callback == ServerToClientTimer::StatusCode::STATUS_ERROR )
        {
            printf( "Connection closed\n" );
            Cleanup();
            return false;
        }
        else
        {
            printf( "recv failed with error: %d\n", WSAGetLastError() );
            Cleanup();
            return false;
        }

        return true;
    }


}

bool Client::sendMessage( ClientToServerTimer& _msg )
{
    int iResult;
    int size = _msg.GetPacketSize();
    char *msgCharPtr = new char[size];
    ClientToServerTimer::SerializeMessage( _msg, *msgCharPtr );

    iResult = send( ConnectSocket, msgCharPtr, size, 0 );
    if( iResult == SOCKET_ERROR )
    {
        printf( "send failed with error: %d\n", WSAGetLastError() );
        closesocket( ConnectSocket );
        WSACleanup();
        return false;
    }

    printf( "Bytes Sent: %ld\n", iResult );
    return true;

}

void Client::displayMenu()
{
    printf( "----------TIMER MENU----------\n"
            "1. Creating Timer\n"
            "create (TIMER_NAME)\n" "\n"
            "2. Setting Timer\n"
            "set (TIMER_NAME) (DURATION) [PERIOD]\n" "\n"
            "3. Waiting for Timer\n"
            "wait (TIMER_NAME)\n" "\n"
            "4. Getting the Timer\n"
            "get (TIMER_NAME)\n" "\n"
            "5. Deleting the Timer\n"
            "delete (TIMER_NAME)\n" "\n"
            "6. Canceling the Timer - Setting it into inactive mode\n"
            "cancel (TIMER_NAME)\n" "\n"
            "7. Closing the program\n"
            "shutdown\n" "\n"
            "\n"
            "Arguments:- between () mandatory\n"
            "          - between [] optional\n"
            "TIMER_NAME is a string of up to 64 characters\n"
            "DURATION is a number given in ms (LESS THAN 10 sec due to wait limitation)\n"
            "PERIOD is a number given in ms\n"
            "------------------------------\n"
    );
}

bool Client::setTimerName( ClientToServerTimer & _msg, std::istringstream & icommand )
{
    if( icommand.eof() )
    {
        printf( "Timer must have a name!\n\n" );
        return false;
    }
    std::string name;
    icommand >> name;
    if( name.size() == 0 )
    {
        printf( "Timer must have a name!\n\n" );
        return false;
    }

    _msg.SetName( name );
    return true;
}

bool Client::setTimerDuration( ClientToServerTimer & _msg, std::istringstream & icommand )
{
    if( icommand.eof() )
    {
        return false;
    }
    DWORD duration;
    if( !(icommand >> duration) )
    {
        return false;
    }

    _msg.duration = duration;
    return true;
}

bool Client::setTimerPeriod( ClientToServerTimer & _msg, std::istringstream & icommand )
{
    if( icommand.eof() )
    {
        return false;
    }
    DWORD period;
    if( !(icommand >> period) )
    {
        return false;
    }

    _msg.period = period;
    return true;
}

bool Client::Run()
{
    ClientToServerTimer welcomeMessage;
    welcomeMessage.SetName( "Welcome" );
    welcomeMessage.RequestedOperation = ClientToServerTimer::Operation::TIMER_WELCOME;
    sendMessage( welcomeMessage );
    if( !receiveWelcomeMessage() )
        return false;
    displayMenu();
    while( true )
    {
        printf( ">" );

        std::string command;
        std::string task;
        ClientToServerTimer request;

        std::getline( std::cin, command );
        std::istringstream icommand( command );
        icommand >> task;
        std::transform( task.begin(), task.end(), task.begin(), ::tolower );

        if( task == "create" )
        {
            request.RequestedOperation = ClientToServerTimer::Operation::TIMER_CREATE;
            if( setTimerName( request, icommand ) == false )
            {
                printf( "Timer must have a name!\n\n" );
                displayMenu();
                continue;
            }
        }

        else if( task == "set" )
        {
            request.RequestedOperation = ClientToServerTimer::Operation::TIMER_SET;
            if( setTimerName( request, icommand ) == false )
            {
                printf( "Timer must have a name!\n\n" );
                displayMenu();
                continue;
            }
            if( setTimerDuration( request, icommand ) == false )
            {
                printf( "Timer must have duration!\n\n" );
                displayMenu();
                continue;
            }
            if( setTimerPeriod( request, icommand ) == false )
            {
                request.period = 0;
            }
        }

        else if( task == "wait" )
        {
            request.RequestedOperation = ClientToServerTimer::Operation::TIMER_WAIT;
            if( setTimerName( request, icommand ) == false )
            {
                printf( "Timer must have a name!\n\n" );
                displayMenu();
                continue;
            }
            if( setTimerDuration( request, icommand ) == false )
            {
                request.duration = INFINITE;
            }

        }
        else if( task == "get" )
        {
            request.RequestedOperation = ClientToServerTimer::Operation::TIMER_GET;
            if( setTimerName( request, icommand ) == false )
            {
                printf( "Timer must have a name!\n\n" );
                displayMenu();
                continue;
            }
        }
        else if( task == "delete" )
        {
            request.RequestedOperation = ClientToServerTimer::Operation::TIMER_DELETE;
            if( setTimerName( request, icommand ) == false )
            {
                printf( "Timer must have a name!\n\n" );
                displayMenu();
                continue;
            }
        }
        else if( task == "cancel" )
        {
            request.RequestedOperation = ClientToServerTimer::Operation::TIMER_CANCEL;
            if( setTimerName( request, icommand ) == false )
            {
                printf( "Timer must have a name!\n\n" );
                displayMenu();
                continue;
            }
        }
        else if( task == "shutdown" )
        {
            break;
        }
        else
        {
            printf( "Unknown request.\n\n" );
            //displayMenu();
            continue;
        }

        if( sendMessage( request ) == false )
        {
            return false;
        }
        if( receiveMessage() == false )
        {
            return false;
        }
        std::cin.clear();
    }
    return true;
}

int Client::Cleanup()
{
    int iResult;
    //shutdown the connection since no more data will be sent
    iResult = shutdown( ConnectSocket, SD_SEND );
    if( iResult == SOCKET_ERROR )
    {
        printf( "shutdown failed with error: %d\n", WSAGetLastError() );
        closesocket( ConnectSocket );
        WSACleanup();
        return EXIT_FAILURE;
    }


    // cleanup
    closesocket( ConnectSocket );
    WSACleanup();
    return 0;
}

