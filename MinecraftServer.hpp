#pragma once

#include "ServerConnection.hpp"
#include "Packet.hpp"
#include "MinecraftChunk.hpp"
#include "DumbController.hpp"

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

enum ChatMessageType {
    Message,
    Join,
    Leave,
    ServerInfo
};

struct ChatMessage {
    MinecraftConnection* sender;
    ChatMessageType type;
    string message;
};

struct PlayerPosition {
    double x = 0;
    double y = 0;
    double z = 0;
    float yaw = 0;
    float pitch = 0;
    bool onGround = false;
};

class MinecraftConnection : public ServerConnection {
public:
    MinecraftConnectionState state = NONE;
    bool joined = false;

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

    uint64_t lastAlive = 0;
    bool lastAliveVerified = true;


public:
    MinecraftConnection(ServerConnectionHandler* handler) : ServerConnection(handler) {};
};


class MinecraftServer {
private:
    ServerConnectionHandler socketHandler;
    vector<ChatMessage> chatMessages;
    int nextEntityId = 1;
public:
    MinecraftServer();
    ~MinecraftServer();
public:
    void Start();
    
    void CreateWorld();
    uint32_t ReserveEntityID();

    void Update();
    void OnPacketReceive(MinecraftConnection* con);

    void Stop();
};

extern MinecraftServer* gMCServer;

