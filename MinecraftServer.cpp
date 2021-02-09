#include "MinecraftServer.hpp"

#include "common.hpp"
#include "Packet.hpp"
#include "NBT.hpp"

#include "MinecraftServerResources.hpp"

using namespace std;
using namespace MCP;

MinecraftServer* gMCServer = new MinecraftServer();


MinecraftServer::MinecraftServer()
{
}

MinecraftServer::~MinecraftServer()
{
    Stop();
}


void MinecraftServer::Start()
{
    socketHandler.Initialize();

    socketHandler.SetReceiveCallback([](ServerConnection * con) {
        gMCServer->OnPacketReceive((MinecraftConnection*)con); // thats bad lol
    });

    while (socketHandler.IsActive()) {
        Update();
    }
}


void MinecraftServer::Update()
{
    // clean connections that have already ended
    socketHandler.CheckConnections();
    // wait for new connections
    socketHandler.PrepareNewConnection(new MinecraftConnection(&socketHandler));
}

void MinecraftServer::OnPacketReceive(MinecraftConnection* con) {

    // data buffer can contain multiple packets. read all of them
    int totalLength = 0;
    while (totalLength < con->GetDataLength() ) {
        // read from ofsetted buffer
        char* buffer = &con->GetDataBuffer()[totalLength];
        MCP::Packet inPacket(buffer);
        if (inPacket.GetSize() == 0) break;
        totalLength += inPacket.GetSize();
        

        cout << "Packet ID " << inPacket.id << "(length " << inPacket.GetSize() << ", real length " << con->GetDataLength() << ")" << endl;

        if (con->state == NONE && inPacket.id == 0x00) { // handshake
            MCP::HandshakeInPacket handshake(buffer);
            if (handshake.state == 1) con->state = STATUS;
            else if (handshake.state == 2) con->state = LOGIN;
            cout << dec << "Handshake -> " << "protocol=" << handshake.protocol;
            cout << "   addr=" << handshake.address << ":" << handshake.port;
            cout << "   state=" << handshake.state << endl;

            con->protocolVer = handshake.protocol;
        }
        else if (con->state == STATUS) {
            if (inPacket.id == 0x00) { // request
                // send response
                MCP::Packet response(0x00);
                response.WriteString(MCP::GetServerInfo(con));
                response.Send(con);
            }
            if (inPacket.id == 0x01) { // ping
                uint64_t payload = inPacket.ReadLong();
                // send pong
                MCP::Packet pong(0x01);
                pong.WriteLong(payload);
                pong.Send(con);

                con->Close(); // sometimes client doesnt end connection, fuck them
            }
        }
        else if (con->state == LOGIN) {
            if (inPacket.id == 0x00) { // login start
                string playerName = inPacket.ReadString();
                con->playerName = playerName;
                
                
                // just to make UUID unique for a player, will replace it later
                MCP::LoginSuccessOutPacket loginSuccess(
                    MCP::UUID(playerName.length(), (int)(playerName[0])),
                    playerName
                );
                loginSuccess.Send(con);

                con->state = PLAY;


                // Send crazy ass join game packet
                MCP::Packet joinGame(0x24);
                joinGame.WriteInt(0); // Entity ID
                joinGame.WriteByte(0); // is hardcore?
                joinGame.WriteByte(2); // gamemode, 2 = adventure
                joinGame.WriteByte(-1); // previous gamemode???
                joinGame.WriteVarInt(1); // size of worlds existing
                joinGame.WriteString("minecraft:overworld"); // list of all (1) worlds existing
                joinGame.WriteNBT(*MCP::GetDimensionCodecNBT()); // dimension codec
                joinGame.WriteNBT(*MCP::GetDimensionTypeNBT()); // dimension type
                joinGame.WriteString("minecraft:overworld"); //spawned world name
                joinGame.WriteLong(0); //first 8 bytes of hashed seed (idk)
                joinGame.WriteVarInt(0); // max players, ignored
                joinGame.WriteVarInt(2); // render distance
                joinGame.WriteByte(0); // reduced debug info
                joinGame.WriteByte(1); // instant respawn
                joinGame.WriteByte(0); // debug world
                joinGame.WriteByte(0); // flat world

                joinGame.Send(con); // amen.

                for (int x = -4; x <= 4; x++) for (int y = -4; y <= 4; y++) {
                    MCP::Packet chunk(0x20);
                    chunk.WriteInt(x); // Chunk X
                    chunk.WriteInt(y); // Chunk Y
                    chunk.WriteByte(1); // Full chunk bool
                    chunk.WriteVarInt(1);  // primary bit mask
                    chunk.WriteInt(0x0A000000); // heightmap NBT list. not needed for what I do, so I send empty compound
                    chunk.WriteVarInt(1024);  // biomes array length
                    for (int i = 0; i < 1024; i++) {
                        chunk.WriteVarInt(0);
                    }
                    chunk.WriteVarInt(2057);  // data size
                    //chunk
                    chunk.WriteShort(256);  // num of non-air blocks
                    chunk.WriteByte(4); // bites for single block, 4 is min
                    chunk.WriteVarInt(3); // num of blocks in lookup table
                    chunk.WriteVarInt(0); // block 0 (air)
                    chunk.WriteVarInt(1); // block 1 (stone)
                    chunk.WriteVarInt(2); // block 2 (dirt)
                    chunk.WriteVarInt(256); // number of longs in upcoming data
                    for (int i = 0; i < 16; i++) {
                        if (i % 2 == 0) {
                            chunk.WriteLong(0x1212121212121212);
                        }
                        else {
                            chunk.WriteLong(0x2121212121212121);
                        }
                    }
                    for (int i = 0; i < 240; i++) {
                        chunk.WriteLong(0);
                    }

                    chunk.WriteVarInt(0);  // block entities length

                    chunk.Send(con);


                    MCP::Packet lightUpdate(0x23);
                    lightUpdate.WriteVarInt(x); // Chunk X
                    lightUpdate.WriteVarInt(y); // Chunk Y
                    lightUpdate.WriteByte(0); // trust edges?
                    lightUpdate.WriteVarInt(0b000000000000000010); // sky light mask
                    lightUpdate.WriteVarInt(0b000000000000000010); // block light mask
                    lightUpdate.WriteVarInt(0b111111111111111101); // empty sky light mask
                    lightUpdate.WriteVarInt(0b111111111111111101); // empty block light mask

                    lightUpdate.WriteVarInt(2048);
                    for (int i = 0; i < 256; i++) {
                        lightUpdate.WriteLong(0xFFFFFFFFFFFFFFFF);
                    }
                    lightUpdate.WriteVarInt(2048);
                    for (int i = 0; i < 256; i++) {
                        lightUpdate.WriteLong(0xFFFFFFFFFFFFFFFF);
                    }

                    lightUpdate.Send(con);
                }



                //send "Player Position And Look" packet
                MCP::Packet ppal(0x34);
                ppal.WriteDouble(8); //x
                ppal.WriteDouble(5); //y
                ppal.WriteDouble(8); //z
                ppal.WriteFloat(0); //yaw
                ppal.WriteFloat(0); //pitch
                ppal.WriteByte(0); //relative teleportation flags
                ppal.WriteVarInt(69); //teleport id, client should resend Teleport Confirm (0x00) with this number
                ppal.Send(con);

                /*MCP::Packet disconnect(0x00);
                disconnect.WriteString(R"( {"text":"Kurwa wypierdalaj.","bold":"true"} )");
                disconnect.Send(con);*/
                
            }
        }
        else if (con->state == PLAY) {

        }
    }
}

void MinecraftServer::Stop() {
    socketHandler.Close();
}

