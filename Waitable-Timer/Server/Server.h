#pragma once

#include "ClientManagement.h"
#include <vector>
#include "ClientContext.h"

class Server
{
private:
    int port;
    SOCKET ListenSocket;
    unsigned short NumberOfThreads;
    //CRITICAL_SECTION CS_Console;
    CRITICAL_SECTION CS_ClientList;
    WSAEVENT EVENT_Accept;
    HANDLE IOCP_Port;
    std::vector<HANDLE> workersThreadArray;
    std::vector<ClientContext*> clientsArray;


    const unsigned short MAX_CLIENTS = 4;
    const unsigned short THREADS_PER_PROCESSOR = 2;

    unsigned int GetNumberOfProcessors();
    bool InitializeIOCP();
    void DeInitialize();
    bool PreRun();
    void Cleanup();

    //Threads
    HANDLE startWorkerThread();
    DWORD WorkerThread( void* Param );
    static DWORD WINAPI StaticWorkerThreadStart( void* Param );

    void AddNewClient( ClientContext* client );
    void DeleteClient( ClientContext* client );
    bool AssociateWithIOCP( ClientContext* client );

public:
    bool Initialize();
    bool Run();
    Server(int _port);
    ~Server();
};

