#pragma once

#include <winsock2.h>
#include "protocol.h"

class Client
{
private:
    const char* DEFAULT_PORT = "1357";
    SOCKET ConnectSocket;

    bool receiveMessage();
    bool receiveWelcomeMessage();
    bool sendMessage( ClientToServerTimer& _msg );
    void displayMenu();
    bool setTimerName( ClientToServerTimer& _msg, std::istringstream& icommand );
    bool setTimerDuration( ClientToServerTimer& _msg, std::istringstream& icommand );
    bool setTimerPeriod( ClientToServerTimer& _msg, std::istringstream& icommand );

public:

    bool Initialize();
    bool Run();
    int Cleanup();
    Client();
    ~Client();
};

