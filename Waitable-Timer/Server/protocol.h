#pragma once

#include <string>
#include <windows.h>

#define CTS_BUF_LEN 80
#define STC_BUF_LEN 260

namespace
{
    static const unsigned int NAME_LENGTH = 64;
    static const unsigned int TEXT_LENGTH = 256;
}


struct ClientToServerTimer
{

    DWORD duration;
    LONG period;
    BOOL manualReset;


    enum Operation
    {
        TIMER_CREATE,
        TIMER_SET,
        TIMER_WAIT,
        TIMER_GET,
        TIMER_DELETE,
        TIMER_CANCEL, 
        TIMER_WELCOME
    } RequestedOperation;

        std::string GetName() const;
    void SetName( const std::string& text );

    int GetPacketSize();
    static void SerializeMessage( ClientToServerTimer& msgToSerialize, char& serializedMsg );
    static void DeserializeMessage( ClientToServerTimer& msgToSerialize, char& serializedMsg );
private:
    char name[NAME_LENGTH];
};


struct ServerToClientTimer
{
public:

    enum StatusCode
    {
        STATUS_SUCCESS,
        STATUS_ERROR
    }Callback;

    std::string GetMessage() const;
    void SetMessage( const std::string& text );

    int GetPacketSize();
    static void SerializeMessage( ServerToClientTimer& msgToSerialize, char& serializedMsg );
    static void DeserializeMessage( ServerToClientTimer& msgToSerialize, char& serializedMsg );
private:
    char message[TEXT_LENGTH];
};