#pragma once

#include "ServerConnection.hpp"
#include "Packet.hpp"
#include "MinecraftChunk.hpp"
#include "DumbController.hpp"
#include "Chat.hpp"

struct PlayerPosition {
    double x = 0;
    double y = 0;
    double z = 0;
    float yaw = 0;
    float pitch = 0;
    bool onGround = false;
};

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

    class SetPlayerPositionPacket : public Packet {
    public:
        SetPlayerPositionPacket(PlayerPosition pp) : Packet(0x34) {
            WriteDouble(pp.x); //x
            WriteDouble(pp.y); //y
            WriteDouble(pp.z); //z
            WriteFloat(pp.yaw); //yaw
            WriteFloat(pp.pitch); //pitch
            WriteByte(0); //relative teleportation flags
            WriteVarInt(69); //teleport id, client should resend Teleport Confirm (0x00) with this number, not used lol
        }
    };
}




enum MinecraftConnectionState {
    NONE,
    STATUS,
    LOGIN,
    PLAY,
    DATA
};

class MinecraftServer;

class MinecraftConnection : public ServerConnection {
public:
    MinecraftConnectionState state = NONE;
    bool joined = false;
    MinecraftServer* server;

    int protocolVer = 0;
    string playerName;
    MCP::UUID uuid;
    uint32_t entityId = 0;
    string texture;
    uint8_t oldSkinSettings = 0;
    uint8_t skinSettings = 0;

    PlayerPosition position;
    PlayerPosition oldPosition;
    uint8_t crouchingState = 0;
    uint8_t handAnimState = 0;

    DumbInputType currentControllerZone = None;
    DumbInputType lastControllerZone = None;

    bool god = false;

    uint64_t lastAlive = 0;
    bool lastAliveVerified = true;
public:
    MinecraftConnection(MinecraftServer* server);
};


class Chat;

class MinecraftServer {
private:
    ServerConnectionHandler* socketHandler = nullptr;
    PlayerPosition spawnPoint;
    int nextEntityId = 1;
    int playerCount = 0;
    int requiredProtocol;

    vector<MinecraftConnection*> conInGame;
    vector<MinecraftConnection*> conLeavingGame;
public:
    string secretKey;
public:
    MinecraftServer();
    ~MinecraftServer();
public:
    void Start();
    
    void CreateWorld();
    uint32_t ReserveEntityID();

    int GetPlayerCount() { return playerCount; }
    int GetProtocol() { return requiredProtocol; }
    vector<MinecraftConnection*> GetInGameConnections() { return conInGame; }
    ServerConnectionHandler* GetConnectionHandler() { return socketHandler; }

    void Update();
    void OnPacketReceive(MinecraftConnection* con);

    void Stop();
};

extern MinecraftServer* g_mcServer;

