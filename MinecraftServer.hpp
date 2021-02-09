#pragma once

#include "ServerConnection.hpp"
#include "Packet.hpp"

namespace MCP {
    class HandshakeInPacket : public Packet {
    public:
        uint32_t protocol = 0;
        std::string address;
        uint16_t port = 0;
        uint32_t state = 0;
    public:
        HandshakeInPacket(char* data) : Packet(data) {
            protocol = ReadVarInt();
            address = ReadString();
            port = ReadShort();
            state = ReadVarInt();
        }
    };

    class LoginSuccessOutPacket : public Packet {
    public:
        LoginSuccessOutPacket(UUID uuid, std::string name) : Packet(0x02) {
            WriteUUID(uuid);
            WriteString(name);
        }
    };
}




enum MinecraftConnectionState {
    NONE,
    STATUS,
    LOGIN,
    PLAY
};

class MinecraftConnection : public ServerConnection {
public:
    MinecraftConnectionState state = NONE;
    int protocolVer = 0;
    std::string playerName;
public:
    MinecraftConnection(ServerConnectionHandler* handler) : ServerConnection(handler) {};
};


class MinecraftServer {
private:
    ServerConnectionHandler socketHandler;
public:
    MinecraftServer();
    ~MinecraftServer();
public:
    void Start();
    
    void Update();
    void OnPacketReceive(MinecraftConnection* con);

    void Stop();
};

extern MinecraftServer* gMCServer;

