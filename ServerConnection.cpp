#include "ServerConnection.hpp"

#include <iostream>
#include <string>
#include <algorithm>

ServerConnectionHandler::ServerConnectionHandler() {

}

// does all of the winsock magic and makes the socket
void ServerConnectionHandler::Initialize(uint16_t port) 
{
    WSADATA wsa;
    sockaddr_in server;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        throw "WSA Startup failed : " + std::to_string(WSAGetLastError());
    }

    if ((listenerSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        throw "Socket creation failed : " + std::to_string(WSAGetLastError());
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(listenerSocket, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        throw "Socket bind failed : " + std::to_string(WSAGetLastError());
    }

    if (listen(listenerSocket, 3) == SOCKET_ERROR) {
        throw "Socket listen failed : " + std::to_string(WSAGetLastError());
    }

    active = true;
}

void ServerConnectionHandler::Close()
{
    if (!active) return;

    for (ServerConnection* con : connections) {
        con->Close();
    }
    closesocket(listenerSocket);
    WSACleanup();
    active = false;
}

void ServerConnectionHandler::CheckConnections() 
{
    for (auto it = connections.begin(); it != connections.end();) {
        if (!((*it)->IsActive())) {
            it = connections.erase(it);
        }
        else {
            ++it;
        }
    }
}

void ServerConnectionHandler::PrepareNewConnection(ServerConnection* con)
{
    if(con==nullptr) con = new ServerConnection(this);
    if (con->Activate()) {
        connections.push_back(con);
    }
    else {
        delete con;
    }
}


void ServerConnectionHandler::HandleReceive(ServerConnection* con)
{
    // wait for a packet to come. Close connection if returned false.
    bool status = con->Receive();
    if (!status) {
        con->Close();
    }
    else {
        int len = con->GetDataLength();
        //std::cout << "Received packet with " << len << " bytes." << std::endl;

        // execute custom callback
        if (receiveCallback) {
            receiveCallback(con);
        }
    }
}

ServerConnectionHandler::~ServerConnectionHandler()
{
    if (active) Close();
}






ServerConnection::ServerConnection(ServerConnectionHandler* handler) {
    socketHandler = handler;
    dataBuffer = nullptr;
}

bool ServerConnection::Activate()
{
    connectionSocket = accept(socketHandler->GetListenerSocket(), NULL, NULL);
    // accept new connection socket using listener
    if (connectionSocket == INVALID_SOCKET)
    {
        active = false;
    }
    else {
        active = true;
        // create thread for this new connection
        connectionThread = std::thread(&ServerConnection::ThreadLoop, this);
    }
    return active;
}

void ServerConnection::ThreadLoop() {
    try {
        while (IsActive()) {
            socketHandler->HandleReceive(this);
        }
    }
    catch (std::string err) {
        std::cerr << err << std::endl;
        Close();
    }
}

bool ServerConnection::Receive()
{
    if (!active) return false;

    // clear previous buffer, that's for saving space when connection is awaiting for informations
    if (dataBuffer != nullptr) delete dataBuffer;
    dataBuffer = nullptr;

    // wait for new data to arrive
    int waitResult = recv(connectionSocket, nullptr, 0, 0);

    // read the data
    dataBuffer = new char[MC_MAX_PACKET_SIZE];
    int result = recv(connectionSocket, dataBuffer, MC_MAX_PACKET_SIZE, 0);
    if (result > 0) {
        dataLen = result;
        return true;
    }
    else if (result < 0) {
        throw "Socket recv failed : " + std::to_string(WSAGetLastError());
    }
    else {
        dataLen = 0;
        return false;
    }
}

void ServerConnection::Send(char* message, uint32_t size)
{
    if (!active) return;

    int result = send(connectionSocket, (const char*)message, size, 0);
    if (result == SOCKET_ERROR) {
        throw "Socket send failed : " + std::to_string(WSAGetLastError());
    }
}

void ServerConnection::Close()
{
    if (!active) return;

    closesocket(connectionSocket);
    active = false;
    if(dataBuffer!=nullptr)delete dataBuffer;
}

ServerConnection::~ServerConnection()
{
    if (active) Close();
}
