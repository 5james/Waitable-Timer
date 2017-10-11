#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "protocol.h"
#include "Client.h"

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
//#pragma comment (lib, "Mswsock.lib")
//#pragma comment (lib, "AdvApi32.lib")


int __cdecl main( int argc, char **argv )
{
    Client client;
    if( client.Initialize() == false )
        return EXIT_FAILURE;
    if( client.Run() == false )
        return EXIT_FAILURE;
    client.Cleanup();
   return 0;
}
