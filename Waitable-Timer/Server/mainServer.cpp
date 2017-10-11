#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "protocol.h"
#include "Server.h"

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
//#pragma comment (lib, "Mswsock.lib")
//#pragma comment (lib, "AdvApi32.lib")


int __cdecl main()
{
    try
    {
        Server s( 1357 );
        if( !s.Initialize() )
            return EXIT_FAILURE;
        s.Run();
    }
    catch( ... )
    {
        return EXIT_FAILURE;
    }
    return 0;
}