#include "protocol.h"

//-------------------------------------------------------
//----------------ClientToServerTimer--------------------

int ClientToServerTimer::GetPacketSize()
{
	return sizeof( Operation ) + sizeof( duration ) + sizeof( period ) + sizeof( manualReset ) + sizeof( name );
}

void ClientToServerTimer::SerializeMessage( ClientToServerTimer& msgToSerialize, char& serializedMsg )
{
	DWORD* p1 = ( DWORD* ) (&serializedMsg);
	*p1 = msgToSerialize.duration;
	p1++;

	LONG* p2 = ( LONG* ) p1;
	*p2 = msgToSerialize.period;
	p2++;

	BOOL* p3 = ( BOOL* ) p2;
	*p3 = msgToSerialize.manualReset;
	p3++;

	Operation* p4 = ( Operation* ) p3;
	*p4 = msgToSerialize.RequestedOperation;
	p4++;

	char* p5 = ( char* ) p4;
	int i = 0;
	while( i < NAME_LENGTH )
	{
		*p5 = msgToSerialize.name[i];
		p5++;
		i++;
	}
}

void ClientToServerTimer::DeserializeMessage( ClientToServerTimer& msgToSerialize, char& serializedMsg )
{
	DWORD* p1 = ( DWORD* ) (&serializedMsg);
	msgToSerialize.duration = *p1;
	p1++;

	LONG* p2 = ( LONG* ) p1;
	msgToSerialize.period = *p2;
	p2++;

	BOOL* p3 = ( BOOL* ) p2;
	msgToSerialize.manualReset = *p3;
	p3++;

	Operation* p4 = ( Operation* ) p3;
	msgToSerialize.RequestedOperation = *p4;
	p4++;

	char* p5 = ( char* ) p4;
	int i = 0;
	while( i < NAME_LENGTH )
	{
		msgToSerialize.name[i] = *p5;
		p5++;
		i++;
	}
}

void ClientToServerTimer::SetName( const std::string& text )
{
	std::string str = text;
	str.resize( NAME_LENGTH - 1 );
	strncpy_s( name, str.c_str(), NAME_LENGTH );
}

std::string ClientToServerTimer::GetName() const
{
	return std::string( name );
}


//-------------------------------------------------------
//----------------ServerToClientTimer--------------------


std::string ServerToClientTimer::GetMessage() const
{
	return std::string( message );
}

void ServerToClientTimer::SetMessage( const std::string& text )
{
	std::string str = text;
	str.resize( TEXT_LENGTH - 1 );
	strncpy_s( message, str.c_str(), TEXT_LENGTH );
}

int ServerToClientTimer::GetPacketSize()
{
	return sizeof( Callback ) + sizeof( message );
}

void ServerToClientTimer::SerializeMessage( ServerToClientTimer & msgToSerialize, char & serializedMsg )
{
	StatusCode* p1 = ( StatusCode* ) (&serializedMsg);
	*p1 = msgToSerialize.Callback;
	p1++;

	char* p2 = ( char* ) p1;
	int i = 0;
	while( i < TEXT_LENGTH )
	{
		*p2 = msgToSerialize.message[i];
		p2++;
		i++;
	}
}

void ServerToClientTimer::DeserializeMessage( ServerToClientTimer & msgToSerialize, char & serializedMsg )
{

	StatusCode* p1 = ( StatusCode* ) (&serializedMsg);
	msgToSerialize.Callback = *p1;
	p1++;

	char* p5 = ( char* ) p1;
	int i = 0;
	while( i < TEXT_LENGTH )
	{
		msgToSerialize.message[i] = *p5;
		p5++;
		i++;
	}
}
