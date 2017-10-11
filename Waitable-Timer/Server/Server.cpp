#define WIN32_LEAN_AND_MEAN	
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Server.h"
#include <stdio.h>
#include <sstream>
#include <locale>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include "ClientManagement.h"
#include "protocol.h"
#include "ClientManagement.h"




DWORD WINAPI ClientThreadStart( void* _ClientSocket )
{
    ClientManagement c( ( SOCKET ) _ClientSocket );
    c.Run();
    return 0;
}

unsigned int Server::GetNumberOfProcessors()
{

    static int nProcessors = 0;
    if( 0 == nProcessors )
    {
        SYSTEM_INFO si;
        GetSystemInfo( &si );
        nProcessors = si.dwNumberOfProcessors;
    }
    return nProcessors;

}



bool Server::Initialize()
{
    //Calculating number of threads
    NumberOfThreads = GetNumberOfProcessors() * THREADS_PER_PROCESSOR;

    printf( "Number of accessible processors: %d. Number of threads to initialize: %d.\n", GetNumberOfProcessors(), NumberOfThreads );

    //InitializeCriticalSection( &CS_Console );
    InitializeCriticalSection( &CS_ClientList );

    //Initialize IOCP
    WSADATA wsaData;
    int iResult;

    iResult = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
    if( iResult != 0 )
    {
        printf( "WSAStartup failed with error: %d\n", iResult );
        return false;
    }

    if( InitializeIOCP() == false )
    {
        printf( "Initialization of IOCP failed.\n" );
        return false;
    }

    return true;
}

bool Server::InitializeIOCP()
{
    IOCP_Port = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );

    if( IOCP_Port == NULL )
    {
        printf( "Creating IOCP failed - %d.\n", WSAGetLastError() );
        return false;
    }

    for( int i = 0; i < NumberOfThreads; i++ )
    {
        workersThreadArray.push_back( startWorkerThread() );
    }
    return true;
}

void Server::DeInitialize()
{
    //DeleteCriticalSection( &CS_Console );
    DeleteCriticalSection( &CS_ClientList );

    CloseHandle( IOCP_Port );

    WSACleanup();
}

bool Server::PreRun()
{
    int iResult;


    ListenSocket = WSASocket( AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED );

    if( ListenSocket == INVALID_SOCKET )
    {
        printf( "socket failed with error: %ld\n", WSAGetLastError() );
        closesocket( ListenSocket );
        DeInitialize();
        return false;
    }


    struct sockaddr_in ServerAddress;
    ZeroMemory( ( char * ) &ServerAddress, sizeof( ServerAddress ) );
    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_addr.s_addr = INADDR_ANY;
    ServerAddress.sin_port = htons( port );

    //Assign local address and port number
    if( bind( ListenSocket, (struct sockaddr *) &ServerAddress, sizeof( ServerAddress ) ) == SOCKET_ERROR )
    {
        printf( "bind failed with error: %d\n", WSAGetLastError() );
        closesocket( ListenSocket );
        DeInitialize();
        return false;
    }
    if( listen( ListenSocket, SOMAXCONN ) == SOCKET_ERROR )
    {
        printf( "listen failed with error: %d\n", WSAGetLastError() );
        closesocket( ListenSocket );
        DeInitialize();
        return false;
    }


    EVENT_Accept = WSACreateEvent();
    if( EVENT_Accept == WSA_INVALID_EVENT )
    {
        printf( "WSAAccept event initialization error\n" );
        closesocket( ListenSocket );
        DeInitialize();
        return false;
    }

    if( WSAEventSelect( ListenSocket, EVENT_Accept, FD_ACCEPT ) == SOCKET_ERROR )
    {
        printf( "WSAEventSelect() error\n" );
        closesocket( ListenSocket );
        WSACloseEvent( EVENT_Accept );
        DeInitialize();
        return false;
    }
    return true;
}



bool Server::Run()
{
    if( PreRun() == false )
    {
        return false;
    }

    WSANETWORKEVENTS WSAEvents;
    while( TRUE )
    {
        if( WSA_WAIT_TIMEOUT != WSAWaitForMultipleEvents( 1, &EVENT_Accept, FALSE, 100, FALSE ) )
        {
            WSAEnumNetworkEvents( ListenSocket, EVENT_Accept, &WSAEvents );
            if( (WSAEvents.lNetworkEvents & FD_ACCEPT) && (WSAEvents.iErrorCode[FD_ACCEPT_BIT] == 0) )
            {
                sockaddr_in client_con;
                int size = sizeof( client_con );

                SOCKET Socket = accept( ListenSocket, ( sockaddr* ) &client_con, &size );

                if( Socket == INVALID_SOCKET )
                {
                    printf( "Error while accepting client - %d.", WSAGetLastError() );
                }

                char str[INET_ADDRSTRLEN];
                inet_ntop( AF_INET, &(client_con.sin_addr), str, INET_ADDRSTRLEN );
                printf( "[%d] Client connected from: %s.\n", GetCurrentThreadId(), str );

                ClientContext *client = new ClientContext();

                client->Action = ClientContext::ACTION::WRITE;
                client->Socket = Socket;

                AddNewClient( client );

                if( true == AssociateWithIOCP( client ) )
                {

                    client->ClientContext::ACTION::WRITE;
                    WSABUF *wsabuf = client->WSA_buffer;
                    OVERLAPPED *overlapped = client->Overlapped;

                    DWORD dwFlags = 0;
                    DWORD dwBytes = 0;

                    client->SetInitialMessage();

                    int iResult = WSARecv( client->Socket, wsabuf, 1, &dwBytes, &dwFlags, overlapped, NULL );

                    if( (iResult == SOCKET_ERROR) && (WSAGetLastError() != WSA_IO_PENDING) )
                    {
                        printf( "Error with initial post!\n" );
                        continue;
                    }
                    //DWORD bytes_sent = 0, flags = 0;

                    //int iSent = WSASend( client->Socket, wsabuf, 1, &bytes_sent, flags, overlapped, NULL );
                    //if( (SOCKET_ERROR == iSent) && (WSA_IO_PENDING != WSAGetLastError()) )
                    //{
                    //    printf( "[%d] WSASend Error.\n", GetCurrentThreadId() );
                    //    DeleteClient( client );
                    //}
                }
            }
        }
    }
    return true;
}

void Server::Cleanup()
{
    for( int i = 0; i < NumberOfThreads; ++i )
    {
        PostQueuedCompletionStatus( IOCP_Port, 0, (DWORD) NULL, NULL );

    }
    WaitForMultipleObjects( workersThreadArray.size(), workersThreadArray.data(), TRUE, INFINITE );
    
    for( int i = 0; i < clientsArray.size(); ++i )
    {
        DeleteClient( clientsArray[i] );
    }
}

Server::Server( int _port )
{
    std::locale::global( std::locale( "" ) );

    IOCP_Port = NULL;

    //TCHAR buf[100];
    //sprintf_s( buf, sizeof( buf ), "%d", _port );
    //port = buf;
    port = _port;
}


Server::~Server()
{
    Cleanup();
    closesocket( ListenSocket );
    DeInitialize();
}


void Server::AddNewClient( ClientContext * client )
{
    EnterCriticalSection( &CS_ClientList );
    clientsArray.push_back( client );
    LeaveCriticalSection( &CS_ClientList );
}

void Server::DeleteClient( ClientContext * client )
{
    EnterCriticalSection( &CS_ClientList );
    for( std::vector<ClientContext*>::iterator it = clientsArray.begin(); it != clientsArray.end(); ++it )
    {
        if( (*it) == client )
        {
            printf( "[%d] Client disconnected.\n", GetCurrentThreadId() );
            clientsArray.erase( it );
            delete client;
            break;
        }
    }
    LeaveCriticalSection( &CS_ClientList );
}

bool Server::AssociateWithIOCP( ClientContext * client )
{
    HANDLE h = CreateIoCompletionPort( ( HANDLE ) client->Socket, IOCP_Port, ( DWORD ) client, 0 );
    if( h == NULL )
    {
        printf( "Error with transefering client to completition port\n" );
        DeleteClient( client );
        return false;
    }
    return true;
}


HANDLE Server::startWorkerThread()
{
    DWORD ThreadID;
    return CreateThread( NULL, 0, StaticWorkerThreadStart, (void*) this, 0, &ThreadID );
}


DWORD Server::StaticWorkerThreadStart( void * Param )
{
    Server* This = ( Server* ) Param;
    return This->WorkerThread( 0 );
}

DWORD Server::WorkerThread( void* Param )
{

    void *lpCompletionKey = NULL;
    OVERLAPPED *pOverlapped = NULL;
    ClientContext *client = NULL;
    DWORD NumberOfBytesTransfered = 0;
    int nBytesRecv = 0;
    int nBytesSent = 0;
    DWORD bytes_sent = 0, flags = 0;

    while( TRUE )
    {
        BOOL bReturn = GetQueuedCompletionStatus( IOCP_Port, &NumberOfBytesTransfered, reinterpret_cast< ULONG_PTR* >(&lpCompletionKey), &pOverlapped, INFINITE );

        if( NULL == lpCompletionKey )
        {
            break;
        }

        client = ( ClientContext * ) lpCompletionKey;

        if( (FALSE == bReturn) || ((TRUE == bReturn) && (0 == NumberOfBytesTransfered)) )
        {
            DeleteClient( client );
            continue;
        }

        WSABUF *wsa_buf = client->WSA_buffer;
        OVERLAPPED *overlap = client->Overlapped;
        int size;

        switch( client->Action )
        {
            case ClientContext::ACTION::READ:

                client->sent_bytes += NumberOfBytesTransfered;
                if( client->sent_bytes < client->total_bytes )
                {
                    client->Action = ClientContext::ACTION::READ;

                    wsa_buf->buf += client->sent_bytes;
                    wsa_buf->len = client->total_bytes - client->sent_bytes;

                    flags = 0;

                    nBytesSent = WSASend( client->Socket, wsa_buf, 1, &bytes_sent, flags, overlap, NULL );

                    if( (SOCKET_ERROR == nBytesSent) && (WSA_IO_PENDING != WSAGetLastError()) )
                    {
                        DeleteClient( client );
                    }
                }
                else
                {
                    client->Action = ClientContext::ACTION::WRITE;
                    client->ResetWSABuffer();

                    flags = 0;

                    nBytesRecv = WSARecv( client->Socket, wsa_buf, 1, &bytes_sent, &flags, overlap, NULL );
                    client->MessageProcessed = false;
                    if( (SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()) )
                    {
                        printf( "[%d] WSARecv Error.", GetCurrentThreadId() );

                        DeleteClient( client );
                    }
                }

                break;

            case ClientContext::ACTION::WRITE:

                client->Action = ClientContext::ACTION::READ;
                size = client->ProcessMessage();
                client->total_bytes = size;
                client->sent_bytes = 0;
                wsa_buf->len = size;
                flags = 0;

                nBytesSent = WSASend( client->Socket, wsa_buf, 1, &bytes_sent, flags, overlap, NULL );
                if( (SOCKET_ERROR == nBytesSent) && (WSA_IO_PENDING != WSAGetLastError()) )
                {
                    printf( "[%d] WSASend Error.\n", GetCurrentThreadId() );

                    DeleteClient( client );
                }
                break;

            default:
                break;
        }
    }

    return 0;
}