#include "DataReceiver.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <iostream>

using namespace std;

DataReceiver* g_dataReceiver = new DataReceiver();

DataReceiver::DataReceiver()
{
    active = false;
    clientSocket = INVALID_SOCKET;
}


DataReceiver::~DataReceiver()
{

}

void DataReceiver::Initialize()
{
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        throw "WSA Startup failed : " + std::to_string(WSAGetLastError());
    }

    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        throw "Socket creation failed : " + std::to_string(WSAGetLastError());
    }

    addrinfo hints, * result;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo("127.0.0.1", "25565", &hints, &result) != 0) {
        throw "GetAddrInfo failed : " + std::to_string(WSAGetLastError());
    }

    if ((clientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == INVALID_SOCKET) {
        throw "Socket creation failed : " + std::to_string(WSAGetLastError());
    }

    active = true;

    if (connect(clientSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        throw "Socket connection failed : " + std::to_string(WSAGetLastError());
    }

    freeaddrinfo(result);
    
    // send handshake packet
    MCP::Packet handshake(0x68);
    handshake.Send(clientSocket);
}

void DataReceiver::ReceiveData()
{
    // send request packet (detailed)
    MCP::Packet request(0x69);
    request.WriteByte(0x01);
    request.Send(clientSocket);

    // wait for new data to arrive
    int waitResult = recv(clientSocket, nullptr, 0, 0);

    // receive the data
    char* dataBuffer = new char[1024];
    int dataLen = recv(clientSocket, dataBuffer, 1024, 0);
    if (dataLen < 0) {
        throw "Socket recv failed : " + std::to_string(WSAGetLastError());
    }
    if (dataLen == 0)return;

    // read the data
    MCP::Packet pIn(dataBuffer);

    data.movementX = pIn.ReadFloat();
    data.movementY = pIn.ReadFloat();
    data.angleX = pIn.ReadFloat();
    data.angleY = pIn.ReadFloat();
    for (int i = 0; i < 5; i++) {
        data.digitalAnalogs[i] = pIn.ReadFloat();
    }
    data.playerCount = pIn.ReadInt();
    for (int i = 0; i < 9; i++) {
        data.inputCounts[i] = pIn.ReadInt();
    }

    /*cout << "movX:" << data.movementX << ",";
    cout << "movY:" << data.movementY << ",";
    cout << "angX:" << data.angleX << ",";
    cout << "angY:" << data.angleY << ",";
    cout << "digital:";
    for (int i = 0; i < 5; i++) {
        cout << data.digitalAnalogs[i] << ",";
    }
    cout << endl;*/

    delete[] dataBuffer;
}

void DataReceiver::Disable()
{
    if (!active) return;

    active = false;
    closesocket(clientSocket);
    WSACleanup();
}
