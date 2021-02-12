#pragma once

#include <winsock2.h>
#include <string>
#include <stdint.h>
#include <vector>
#include <thread>

#include "Packet.hpp"

#define MC_DEFAULT_PORT 25565
#define MC_MAX_PACKET_SIZE 2097151

class ServerConnection;

class ServerConnectionHandler {
private:
    bool active = false;

    SOCKET listenerSocket = INVALID_SOCKET;
    std::vector<ServerConnection*> connections;

    using ReceiveCallback = void(*)(ServerConnection* con);
    ReceiveCallback receiveCallback = nullptr;
public:
    ServerConnectionHandler();
    ~ServerConnectionHandler();

    bool IsActive() { return active; }

public:
    void Initialize(uint16_t port = MC_DEFAULT_PORT);
    void Close();

    void CheckConnections();
    void PrepareNewConnection(ServerConnection* con = nullptr);
    bool HasNewConnection();

    void SetReceiveCallback(ReceiveCallback callback) { receiveCallback = callback; }
    void HandleReceive(ServerConnection* con);

    SOCKET GetListenerSocket() { return listenerSocket; }
    vector<ServerConnection*> GetConnections() { return connections; }
    int ClientCount() { return connections.size(); };
};


class ServerConnection {
private:
    bool active = false;

    ServerConnectionHandler* socketHandler = nullptr;
    SOCKET connectionSocket = INVALID_SOCKET;

    int dataLen = 0;
    char* dataBuffer = nullptr;

    std::thread connectionThread;
public:
    ServerConnection(ServerConnectionHandler* handler);
    ~ServerConnection();

    bool IsActive() { return active; }
public:
    bool Activate();
    void Close();

    void ThreadLoop();

    // receives incoming data and stores it in buffer
    bool Receive();
    // sends data, duh
    void Send(char* message, uint32_t size);

    // getters for buffer
    char* GetDataBuffer() { return dataBuffer; };
    int GetDataLength() { return dataLen; };
};

