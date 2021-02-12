#include "MinecraftServer.hpp"

#include "common.hpp"
#include "Packet.hpp"
#include "NBT.hpp"
#include "MinecraftChunk.hpp"

#include "MinecraftServerResources.hpp"

using namespace std;
using namespace MCP;

MinecraftServer* gMCServer = new MinecraftServer();
ChunkWorld* gWorld = new ChunkWorld();

MinecraftServer::MinecraftServer()
{

}

MinecraftServer::~MinecraftServer()
{
    Stop();
}


void MinecraftServer::Start()
{
    cout << "\"Minecraft Plays Portal 2\" custom bong server!!!!" << endl << endl;

    cout << "Loading dimension codec..." << endl;
    NBTTag codec = MCP::GetDimensionCodecNBT();

    cout << "Creating world... " << endl;
    CreateWorld();

    cout << "Initializing socket handler... " << endl;
    socketHandler.Initialize();

    socketHandler.SetReceiveCallback([](ServerConnection * con) {
        gMCServer->OnPacketReceive((MinecraftConnection*)con); // thats soo bad lol
    });

    cout << "Server is active. Running in a loop..." << endl;
    while (socketHandler.IsActive()) {
        Update();
    }
}

void MinecraftServer::CreateWorld() 
{
    // simple checker pattern
    for (int x = -8; x <= 8; x++) for (int z = -8; z <= 8; z++) {
        short blockID = (x + z) % 2 == 0 ? 1 : 2;
        gWorld->SetBlockID(x, 1, z, blockID);
    }
    
    gWorld->SetBlockID(9, 2, -1, 3355);
    gWorld->SetBlockID(9, 2, 1, 3355);
    gWorld->SetBlockID(9, 3, 0, 3355);
    gWorld->SetBlockID(9, 4, 0, 3355);
    gWorld->SetBlockID(9, 5, 0, 3355);
    gWorld->SetBlockID(9, 6, 0, 3354);
}



void MinecraftServer::Update()
{
    // clean connections that have already ended
    socketHandler.CheckConnections();
    // wait for new connections
    if (socketHandler.HasNewConnection()) {
        socketHandler.PrepareNewConnection(new MinecraftConnection(&socketHandler));
    }

    uint64_t now = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();

    for (ServerConnection* con : socketHandler.GetConnections()) {
        if (!con->IsActive())continue;
        MinecraftConnection* mccon = (MinecraftConnection*)con;
        if (mccon->state != PLAY) continue;

        // handle Keep Alive packet
        if (mccon->lastAlive + 10000 < now) {
            if (mccon->lastAliveVerified) {
                mccon->lastAlive = now;
                mccon->lastAliveVerified = false;
                MCP::Packet keepAlive(0x1F);
                keepAlive.WriteLong(now);
                keepAlive.Send(mccon);
            }
            else {
                MCP::Packet disconnect(0x00);
                disconnect.WriteString(R"( {"text":"Timed out.","bold":"true"} )");
                disconnect.Send(mccon);
                mccon->Close();
            }
        }

        // handle chat
        for (ChatMessage& message : chatMessages) {
            string chatMsg = "[{\"text\":\"\"},";

            if (message.sender == "server.join") {
                chatMsg += "{\"text\":\"" + message.message + "\",\"color\":\"yellow\",\"bold\":true},";
                chatMsg += "{\"text\":\" joined.\",\"color\":\"yellow\"}]";
            }
            else {
                chatMsg += "{\"text\":\"" + message.sender + ": \",\"bold\":true},";
                chatMsg += "{\"text\":\"" + message.message + "\"}]";
            }
            

            MCP::Packet testMessage(0x0E);
            testMessage.WriteString(chatMsg);
            testMessage.WriteByte(0);
            testMessage.WriteUUID({ 0,0 });
            testMessage.Send(mccon);
        }
    }

    //clean chat buffer
    chatMessages.clear();

    //sleep, just dont take up all processing power when not needed lmfao
    std::this_thread::sleep_for(1ms);
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
                

                // getting minecraft skin for given nickname

                // step 1: load uuid from playerdb.co (eg. https://playerdb.co/api/player/minecraft/Krzyhu)

                // step 2: load texture string from mojang servers
                // (eg. https://sessionserver.mojang.com/session/minecraft/profile/16632b42ebd24df0a71ae3745ea14a5f)

                string s = HttpGetRequest("playerdb.co", "/api/player/minecraft/Krzyhu");
                cout << "Kurwa: " << s << endl;

                // just to make UUID unique for a player, will replace it later maybe idk
                uint64_t uniqueId1 = 0, uniqueId2 = playerName.length();
                uniqueId2 = uniqueId2 << 32;
                for (char& c : playerName) {
                    uniqueId1 *= 100;
                    uniqueId1 += c;
                    uniqueId2 += c;
                }
                con->uuid = MCP::UUID(uniqueId1, uniqueId2);

                // fuck verification, its login time
                MCP::LoginSuccessOutPacket loginSuccess(con->uuid, playerName);
                loginSuccess.Send(con);

                con->state = PLAY;

                // Send crazy ass join game packet
                MCP::Packet joinGame(0x24);
                joinGame.WriteInt(0); // Entity ID
                joinGame.WriteByte(0); // is hardcore?
                joinGame.WriteByte(2); // gamemode, 2 = adventure
                joinGame.WriteByte(-1); // previous gamemode??? why tf would you need that??
                joinGame.WriteVarInt(1); // size of worlds existing
                joinGame.WriteString("minecraft:the_end"); // list of all (1) worlds existing
                joinGame.WriteNBT(MCP::GetDimensionCodecNBT()); // dimension codec
                joinGame.WriteNBT(*MCP::GetDimensionTypeNBT()); // dimension type
                joinGame.WriteString("minecraft:the_end"); //spawned world name
                joinGame.WriteLong(0); //first 8 bytes of hashed seed (idk lol)
                joinGame.WriteVarInt(0); // max players, ignored
                joinGame.WriteVarInt(2); // render distance
                joinGame.WriteByte(0); // reduced debug info
                joinGame.WriteByte(1); // instant respawn
                joinGame.WriteByte(0); // debug world
                joinGame.WriteByte(0); // flat world

                joinGame.Send(con); // amen.


                // send joining message
                chatMessages.push_back({ "server.join", playerName });


                // send all chunks from created world
                for (Chunk* chunk : gWorld->chunks) {
                    MCP::ChunkPacket chunkPacket(chunk);
                    chunkPacket.Send(con);

                    MCP::FullBrightLightChunkPacket fullBright(chunk);
                    fullBright.Send(con);
                }

                //send "Player Position And Look" packet
                MCP::Packet ppal(0x34);
                ppal.WriteDouble(0); //x
                ppal.WriteDouble(5); //y
                ppal.WriteDouble(0); //z
                ppal.WriteFloat(0); //yaw
                ppal.WriteFloat(0); //pitch
                ppal.WriteByte(0); //relative teleportation flags
                ppal.WriteVarInt(69); //teleport id, client should resend Teleport Confirm (0x00) with this number
                ppal.Send(con);
                
            }
        }
        else if (con->state == PLAY) {
            if (inPacket.id == 0x10) { // keep alive (serverbound) (confirming the keep alive packet)
                uint64_t keepAliveId = inPacket.ReadLong();
                cout << "Received Keep Alive with param " << keepAliveId << endl;
                if (keepAliveId == con->lastAlive) {
                    con->lastAliveVerified = true;
                }
            }
            if (inPacket.id == 0x03) { // chat message (serverbound)
                string message = inPacket.ReadString();
                chatMessages.push_back({con->playerName, message});
            }
        }
    }
}

void MinecraftServer::Stop() {
    socketHandler.Close();
}

