#include "MinecraftServer.hpp"

#include "common.hpp"
#include "Packet.hpp"
#include "NBT.hpp"
#include "MinecraftChunk.hpp"
#include "DumbController.hpp"

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
    /*for (int x = -8; x <= 8; x++) for (int z = -8; z <= 8; z++) {
        short blockID = (x + z) % 2 == 0 ? 1 : 2;
        gWorld->SetBlockID(x, 1, z, blockID);
    }*/
    
    // general shape of the controller
    for (int x = 0; x < 15; x++) for(int z = 0; z < 19;z++){
        if (x == 0 || x == 14 || z == 0 || z == 18) { //border
            gWorld->SetBlockID(x, 1, z, 9449); // gray concrete
        }
        else {
            gWorld->SetBlockID(x, 1, z, 10838); // smooth quartz slab (bottom not waterlogged)
        }
        gWorld->SetBlockID(x, 0, z, 9450); // light gray concrete
    }

    // left and right joystick
    for (int i = 0; i <= 1; i++) {
        for (int x = 0; x < 5; x++)for (int z = 0; z < 5; z++) {
            int bX = 2 + x;
            int bZ = 2 + z + i*10;
            int bId = i==0 ? 9353 : 9293; // base block (shulker boxes, blue and orange)
            if (x % 4 == 0 && z % 4 == 0) bId = 9388;  // corners (pink glazed terracotta)
            else if (z == 0) bId -= 5; // north edge
            else if (x == 4) bId -= 4; // east edge
            else if (z == 4) bId -= 3; // south edge
            else if (x == 0) bId -= 2; // west edge
            else if (x == 2 && z == 2) bId -= 1; //center ring
            gWorld->SetBlockID(bX, 1, bZ, bId);
        }
    }

    gWorld->SetBlockID(6, 1, 8, 9287); // save button
    gWorld->SetBlockID(6, 1, 10, 9377); // load button

    //digital inputs
    for (int i = 0; i < 5; i++) {
        for (int x = 0; x < 4; x++)for (int z = 0; z < 2; z++) {
            int bX = 9 + x;
            int bZ = 2 + z + i * 3 + (i > 1 ? 1 : 0);
            int bId;
            if (x == 0) {
                bId = 9369; // red shulker facing west
            }
            else if (x == 3){
                bId = 9313; // lime shulker facing east
            }
            else switch (i) {
            case 0: bId = 9461; break; // light blue concrete powder
            case 1: bId = 9459; break; // orange concrete powder
            case 2: bId = 7539; break; // slime block
            case 3: bId = 1357; break; // piston facing west not extended
            case 4: bId = 3356; break; // crafting table
            }
            gWorld->SetBlockID(bX, 1, bZ, bId);
        }
    }
    gWorld->SetBlockID(-16, 255, -16, 1);
    gWorld->SetBlockID(-16, 255, 0, 1);
    gWorld->SetBlockID(0, 255, -16, 1);
}

uint32_t MinecraftServer::ReserveEntityID() {
    int reservedId = nextEntityId;
    nextEntityId++;
    return reservedId;
}

void MinecraftServer::Update()
{
    uint64_t now = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    // quickly getting all connections that are currently in game and that left the game
    vector<MinecraftConnection*> conLeavingGame;
    vector<MinecraftConnection*> conInGame;
    for (ServerConnection* con : socketHandler.GetConnections()) {
        MinecraftConnection* mccon = (MinecraftConnection*)con;
        if (mccon->state != PLAY) continue;
        if (!con->IsActive()) {
            // this connection is about to be removed, add them to the list for later handling
            conLeavingGame.push_back(mccon);
        }
        else {
            // this connection is still in game, just put it in the vector lmfao, what else can i fucking say
            conInGame.push_back(mccon);
        }
    }


    // clean connections that have already ended
    socketHandler.CheckConnections();
    // wait for new connections
    if (socketHandler.HasNewConnection()) {
        socketHandler.PrepareNewConnection(new MinecraftConnection(&socketHandler));
    }


    // do all sorts of stuff to "update" the state of the server here

    // handle leaving players
    if (conLeavingGame.size() > 0) {
        MCP::Packet deleteEntities(0x36);
        deleteEntities.WriteVarInt(conLeavingGame.size());

        MCP::Packet leavingPlayersInfo(0x32);
        leavingPlayersInfo.WriteVarInt(4);
        leavingPlayersInfo.WriteVarInt(conLeavingGame.size());

        for (MinecraftConnection* leavingCon : conLeavingGame) {
            // add leaving message
            chatMessages.push_back({ leavingCon, Leave });
            cout << leavingCon->playerName << " left the server." << endl;

            // send remove player packet for player info
            leavingPlayersInfo.WriteUUID(leavingCon->uuid);

            // remove entity for each player
            deleteEntities.WriteVarInt(leavingCon->entityId);
        }

        // remove entity for each player contd.
        for (MinecraftConnection* gamingCon : conInGame) {
            deleteEntities.Send(gamingCon);
            leavingPlayersInfo.Send(gamingCon);
        }
    }

    


    // converting all chat messages into strings
    vector<string> chatMsgBuffer;
    for (ChatMessage& message : chatMessages) {
        string chatMsg = "[{\"text\":\"\"},";

        if (message.type == Join) {
            chatMsg += "{\"text\":\"" + message.sender->playerName + "\",\"color\":\"yellow\",\"bold\":true},";
            chatMsg += "{\"text\":\" joined.\",\"color\":\"yellow\"}]";
        }
        else if (message.type == Leave) {
            chatMsg += "{\"text\":\"" + message.sender->playerName + "\",\"color\":\"yellow\",\"bold\":true},";
            chatMsg += "{\"text\":\" left.\",\"color\":\"yellow\"}]";
        }
        else if (message.type == Message) {
            chatMsg += "{\"text\":\"" + message.sender->playerName + ": \",\"bold\":true,\"color\":\"";
            chatMsg += DumbController::inputColors[message.sender->currentControllerZone];
            chatMsg += "\"},{\"text\":\"" + message.message + "\"}]";
        }

        chatMsgBuffer.push_back(chatMsg);
    }
    chatMessages.clear();

    for (MinecraftConnection* con : conInGame) {
        // handle Keep Alive packet
        if (con->lastAlive + 10000 < now) {
            if (con->lastAliveVerified) {
                con->lastAlive = now;
                con->lastAliveVerified = false;
                MCP::Packet keepAlive(0x1F);
                keepAlive.WriteLong(now);
                keepAlive.Send(con);
            }
            else {
                MCP::Packet disconnect(0x00);
                disconnect.WriteString(R"( {"text":"Timed out.","bold":"true"} )");
                disconnect.Send(con);
                cout << con->playerName << " timed out." << endl;
                con->Close();
            }
        }

        // handle chat
        for (string& message : chatMsgBuffer) {
            MCP::Packet testMessage(0x0E);
            testMessage.WriteString(message);
            testMessage.WriteByte(0);
            testMessage.WriteUUID({ 0,0 });
            testMessage.Send(con);
        }

        // handle joining
        if (!con->joined) {
            MCP::Packet newPlayerInfo(0x32);
            newPlayerInfo.WriteVarInt(0); // action (0 = add player)
            newPlayerInfo.WriteVarInt(1); // num of players (only 1)
            newPlayerInfo.WriteUUID(con->uuid);
            newPlayerInfo.WriteString(con->playerName);
            newPlayerInfo.WriteVarInt(1); // only one param
            newPlayerInfo.WriteString("textures");
            newPlayerInfo.WriteString(con->texture);
            newPlayerInfo.WriteByte(0); // not signed
            newPlayerInfo.WriteVarInt(2);
            newPlayerInfo.WriteVarInt(1001);
            newPlayerInfo.WriteByte(0);

            MCP::Packet playerInfos(0x32);
            playerInfos.WriteVarInt(0); // action (0 = add player)
            playerInfos.WriteVarInt(conInGame.size()); // num of players (adding them all, including itself)
            for (MinecraftConnection* con2 : conInGame) {
                //sending the newcomer info of all players on server
                playerInfos.WriteUUID(con2->uuid);
                playerInfos.WriteString(con2->playerName);
                playerInfos.WriteVarInt(1); // only one param
                playerInfos.WriteString("textures");
                playerInfos.WriteString(con2->texture);
                playerInfos.WriteByte(0); // not signed
                playerInfos.WriteVarInt(2);
                playerInfos.WriteVarInt(1001);
                playerInfos.WriteByte(0);
                
                //but also sending to all players info of the newcomer
                if (con == con2)continue;
                newPlayerInfo.Send(con2);
            }
            playerInfos.Send(con);

            // exactly the same thing I've done above, but for entities
            MCP::Packet newPlayerEntity(0x04);
            newPlayerEntity.WriteVarInt(con->entityId);
            newPlayerEntity.WriteUUID(con->uuid);
            newPlayerEntity.WriteDouble(con->position.x);
            newPlayerEntity.WriteDouble(con->position.y);
            newPlayerEntity.WriteDouble(con->position.z);
            newPlayerEntity.WriteByte(0);
            newPlayerEntity.WriteByte(0);

            for (MinecraftConnection* con2 : conInGame) {
                if (con == con2)continue;
                //creating an entity of all players on server for newcomer
                MCP::Packet oldPlayerEntity(0x04);
                oldPlayerEntity.WriteVarInt(con2->entityId);
                oldPlayerEntity.WriteUUID(con2->uuid);
                oldPlayerEntity.WriteDouble(con2->position.x);
                oldPlayerEntity.WriteDouble(con2->position.y);
                oldPlayerEntity.WriteDouble(con2->position.z);
                oldPlayerEntity.WriteByte(0); // writing angle of 0,0, fuck Mojang and their dumb angle convention
                oldPlayerEntity.WriteByte(0);
                oldPlayerEntity.Send(con);

                // and sending their skins to the newcomer
                MCP::Packet entMetadata(0x44);
                entMetadata.WriteVarInt(con2->entityId);
                entMetadata.WriteByte(16);
                entMetadata.WriteVarInt(0);
                entMetadata.WriteVarInt(con2->skinSettings);
                entMetadata.WriteByte(0xff);
                entMetadata.Send(con);

                // but also creating a newcomer entity for all players
                newPlayerEntity.Send(con2);
            }

            con->joined = true;
        }

        // update position of this entity for other players
        int8_t yaw = ((int)(con->position.yaw * 0.711111111f)) % 256;
        int8_t pitch = ((int)(con->position.pitch * 0.711111111f)) % 256;

        // there are packets for "relative" position update that are smaller
        // but i'm too lazy to make them work properly so I just do teleport lmfao
        MCP::Packet entityTeleport(0x56);
        entityTeleport.WriteVarInt(con->entityId);
        entityTeleport.WriteDouble(con->position.x);
        entityTeleport.WriteDouble(con->position.y);
        entityTeleport.WriteDouble(con->position.z);
        entityTeleport.WriteByte(yaw);
        entityTeleport.WriteByte(pitch);
        entityTeleport.WriteByte(con->position.onGround);

        MCP::Packet entityHeadLook(0x3A);
        entityHeadLook.WriteVarInt(con->entityId);
        entityHeadLook.WriteByte(yaw);

        for (MinecraftConnection* con2 : conInGame) {
            if (con == con2) continue;
            entityTeleport.Send(con2);
            entityHeadLook.Send(con2);
        }
        con->oldPosition = con->position;


        // handle animations
        if (con->handAnimState > 0) {
            MCP::Packet animation(0x05);
            animation.WriteVarInt(con->entityId);
            if (con->handAnimState == 1) animation.WriteByte(0);
            if (con->handAnimState == 2) animation.WriteByte(3);
            for (MinecraftConnection* con2 : conInGame) {
                if (con == con2) continue;
                animation.Send(con2);
            }
            con->handAnimState = 0;
        }
        if ((con->crouchingState & 0b10) > 0) {
            MCP::Packet entMetadata(0x44);
            entMetadata.WriteVarInt(con->entityId);

            entMetadata.WriteByte(6); // Pose
            entMetadata.WriteVarInt(18); // pose type
            entMetadata.WriteVarInt((con->crouchingState & 1) > 0 ? 5 : 0); // value
            entMetadata.WriteByte(0xff); // end array

            for (MinecraftConnection* con2 : conInGame) {
                if (con == con2) continue;
                entMetadata.Send(con2);
            }
            con->crouchingState = (con->crouchingState & 1);
        }

        // handle skins
        if (con->skinSettings != con->oldSkinSettings) {
            MCP::Packet entMetadata(0x44);
            entMetadata.WriteVarInt(con->entityId);

            entMetadata.WriteByte(16);
            entMetadata.WriteVarInt(0);
            entMetadata.WriteVarInt(con->skinSettings);
            entMetadata.WriteByte(0xff);

            for (MinecraftConnection* con2 : conInGame) {
                entMetadata.Send(con2);
            }
            con->oldSkinSettings = con->skinSettings;
        }
        

        //cout << "x:" << con->position.x << ", y:" << con->position.y << ", z:" << con->position.z << ", yaw:" << con->position.yaw << ", pitch:" << con->position.pitch << ", onGround:" << con->position.onGround << endl;
    }

    //clear chat buffer
    chatMsgBuffer.clear();

    //update controller
    g_dumbController->ProcessClients(conInGame);

    //quickly go over controller data connections to disable inactive ones
    for (ServerConnection* con : socketHandler.GetConnections()) {
        MinecraftConnection* mccon = (MinecraftConnection*)con;
        if (!con->IsActive() || mccon->state != DATA) continue;
        if (mccon->lastAliveVerified) {
            mccon->lastAlive = now;
            mccon->lastAliveVerified = false;
        }
        else if (mccon->lastAlive + 5000 < now) {
            mccon->Close();
        }
    }

    //sleep to make server work in 20 ticks per second
    uint64_t nower = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    int sleepDelay = (int)std::fmax(0, 50 - (nower - now));
    std::this_thread::sleep_for(chrono::milliseconds(sleepDelay));
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
        

        //cout << "Packet ID " << inPacket.id << "(length " << inPacket.GetSize() << ", real length " << con->GetDataLength() << ")" << endl;

        if (con->state == NONE) { // handshake
            if (inPacket.id == 0x00) { // default minecraft handshake
                MCP::HandshakeInPacket handshake(buffer);
                if (handshake.state == 1) con->state = STATUS;
                else if (handshake.state == 2) con->state = LOGIN;
                /*cout << dec << "Handshake -> " << "protocol=" << handshake.protocol;
                cout << "   addr=" << handshake.address << ":" << handshake.port;
                cout << "   state=" << handshake.state << endl;*/

                con->protocolVer = handshake.protocol;
            }
            if (inPacket.id == 0x68) { // special handshake for data receivers
                con->state = DATA;
            }
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
                

                cout << playerName << "(" << con->ipAddress << ") attempts to join the server." << endl;

                // getting minecraft skin for given nickname
                string uuidStr = MCP::GetPlayerUUID(playerName);
                //con->texture = MCP::GetPlayerSkin(uuidStr);
                
                if (uuidStr.size() > 0) {
                    con->uuid = MCP::UUID(uuidStr);
                }
                else {
                    // do this weird shit if non-existing, just to make UUID unique for a player

                    uint64_t uniqueId1 = 0, uniqueId2 = playerName.length();
                    uniqueId2 = uniqueId2 << 32;
                    for (char& c : playerName) {
                        uniqueId1 *= 100;
                        uniqueId1 += c;
                        uniqueId2 += c;
                    }

                    // ???????????
                    uniqueId1 = (uniqueId1 ^ 0x1000) | 0x3000;

                    con->uuid = MCP::UUID(uniqueId1, uniqueId2);
                }
                

                // fuck verification, its login time
                MCP::LoginSuccessOutPacket loginSuccess(con->uuid, playerName);
                loginSuccess.Send(con);

                con->entityId = ReserveEntityID();

                // Send crazy ass join game packet
                MCP::Packet joinGame(0x24);
                joinGame.WriteInt(con->entityId); // Entity ID
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
                joinGame.WriteVarInt(16); // render distance
                joinGame.WriteByte(0); // reduced debug info
                joinGame.WriteByte(1); // instant respawn
                joinGame.WriteByte(0); // debug world
                joinGame.WriteByte(0); // flat world

                joinGame.Send(con); // amen.

                // send all chunks from created world
                for (Chunk* chunk : gWorld->chunks) {
                    MCP::ChunkPacket chunkPacket(chunk);
                    chunkPacket.Send(con);

                    MCP::FullBrightLightChunkPacket fullBright(chunk);
                    fullBright.Send(con);
                }
                con->position = { 4.5,5,4.5,0,0,false };

                //send "Player Position And Look" packet
                MCP::Packet ppal(0x34);
                ppal.WriteDouble(con->position.x); //x
                ppal.WriteDouble(con->position.y); //y
                ppal.WriteDouble(con->position.z); //z
                ppal.WriteFloat(con->position.yaw); //yaw
                ppal.WriteFloat(con->position.pitch); //pitch
                ppal.WriteByte(0); //relative teleportation flags
                ppal.WriteVarInt(69); //teleport id, client should resend Teleport Confirm (0x00) with this number
                ppal.Send(con);

                // send joining message
                chatMessages.push_back({ con, Join });
                cout << playerName << " joined the server successfully." << endl;

                con->state = PLAY;


                cout << "Aktywne polaczenia: " << socketHandler.GetConnections().size() << endl;
            }
        }
        else if (con->state == PLAY) {
            if (inPacket.id == 0x10) { // keep alive (serverbound) (confirming the keep alive packet)
                uint64_t keepAliveId = inPacket.ReadLong();
                //cout << "Received Keep Alive with param " << keepAliveId << endl;
                if (keepAliveId == con->lastAlive) {
                    con->lastAliveVerified = true;
                }
            }
            if (inPacket.id == 0x03) { // chat message (serverbound)
                string message = inPacket.ReadString();
                chatMessages.push_back({con, Message, message});
            }



            // serverbound movement packets
            PlayerPosition newPosition = con->position;
            if (inPacket.id >= 0x12 && inPacket.id <= 0x15) {
                if (inPacket.id == 0x12 || inPacket.id == 0x13) {
                    newPosition.x = inPacket.ReadDouble();
                    newPosition.y = inPacket.ReadDouble();
                    newPosition.z = inPacket.ReadDouble();
                }
                if (inPacket.id == 0x13 || inPacket.id == 0x14) {
                    newPosition.yaw = inPacket.ReadFloat();
                    newPosition.pitch = inPacket.ReadFloat();
                }
                newPosition.onGround = inPacket.ReadByte()>0;
            }
            con->position = newPosition;

            // entity action
            if (inPacket.id == 0x1C) {
                int entityId = inPacket.ReadVarInt();
                int actionId = inPacket.ReadVarInt();
                int jumpBoost = inPacket.ReadVarInt();
                if (actionId == 0)con->crouchingState = 0b11;
                if (actionId == 1)con->crouchingState = 0b10;
            }

            // animation
            if (inPacket.id == 0x2C) {
                int hand = inPacket.ReadVarInt();
                con->handAnimState = hand + 1;
            }

            // client settings
            if (inPacket.id == 0x05) {
                string locale = inPacket.ReadString();
                uint8_t viewDistance = inPacket.ReadByte();
                int chatMode = inPacket.ReadVarInt();
                bool colors = inPacket.ReadByte();
                uint8_t skinParts = inPacket.ReadByte();
                int mainHand = inPacket.ReadVarInt();

                con->skinSettings = skinParts;
            }
        }
        else if (con->state == DATA) {
            con->lastAliveVerified = true;
            if (inPacket.id == 0x69) {
                bool detailed = inPacket.ReadByte() > 0;
                g_dumbController->SendInputPackets(con, detailed);
            }
        }
    }
}

void MinecraftServer::Stop() {
    socketHandler.Close();
}

