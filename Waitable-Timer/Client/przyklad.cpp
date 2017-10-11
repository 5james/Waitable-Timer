//Client
#pragma comment(lib,"ws2_32.lib")
#include <Windows.h>
#include <iostream>


int main2()
{
	//Winsock Startup
	WSAData wsaData;
	WORD DllVersion = MAKEWORD( 2, 1 );
	if( WSAStartup( DllVersion, &wsaData ) != 0 ) //If WSAStartup returns anything other than 0, then that means an error has occured in the WinSock Startup.
	{
		MessageBoxA( NULL, "Winsock startup failed", "Error", MB_OK | MB_ICONERROR );
		exit( 1 );
	}

	SOCKADDR_IN addr; //Address to be binded to our Connection socket
	int sizeofaddr = sizeof( addr ); //Need sizeofaddr for the connect function
	addr.sin_addr.s_addr = inet_addr( "127.0.0.1" ); //Address = localhost (this pc)
	addr.sin_port = htons( 8888 ); //Port = 1111
	addr.sin_family = AF_INET; //IPv4 Socket

	SOCKET Connection = socket( AF_INET, SOCK_STREAM, NULL ); //Set Connection socket
	if( connect( Connection, ( SOCKADDR* ) &addr, sizeofaddr ) != 0 ) //If we are unable to connect...
	{
		MessageBoxA( NULL, "Failed to Connect", "Error", MB_OK | MB_ICONERROR );
		return 0; //Failed to Connect
	}
	std::cout << "Connected!" << std::endl;

	char MOTD[256];
	recv( Connection, MOTD, sizeof( MOTD ), NULL ); //Receive Message of the Day buffer into MOTD array
	std::cout << "MOTD:" << MOTD << std::endl;
	while( true )
	{
		Sleep( 10 );
	}
}
