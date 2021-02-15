#include "common.hpp"

#include "MinecraftServer.hpp"
#include "Packet.hpp"
#include "DumbController.hpp"

DumbController* g_dumbController = new DumbController();

DumbControllerZone DumbController::zones[DUMB_CONTROLLER_INPUT_COUNT] = {
    {0,0,0,0,2,"",6}, // none
    {2,2,7,7,2,R"( {"text":"Movement Analog","bold":"true","color":"dark_aqua"} )",1}, // movement
    {2,12,7,17,2,R"( {"text":"View Analog","bold":"true","color":"red"} )",2}, // view
    {9,2,13,4,2,R"( {"text":"Blue Portal Input","bold":"true","color":"aqua"} )",1}, // blue portal
    {9,5,13,7,2,R"( {"text":"Orange Portal Input","bold":"true","color":"gold"} )",2}, // orange portal
    {9,9,13,11,2,R"( {"text":"Jump Input","bold":"true","color":"green"} )",3}, // jump
    {9,12,13,14,2,R"( {"text":"Crouch Input","bold":"true","color":"dark_purple"} )",5}, // crouch
    {9,15,13,17,2,R"( {"text":"Use Input","bold":"true","color":"yellow"} )",4}, // use
    {6,8,7,9,2,R"( {"text":"Save Input","bold":"true","color":"gray"} )",6}, // save
    {6,10,7,11,2,R"( {"text":"Load Input","bold":"true","color":"dark_gray"} )",6} // load
};

string DumbController::inputColors[DUMB_CONTROLLER_INPUT_COUNT] = {
    "white", "dark_aqua", "red", "aqua", "gold", "green", "dark_purple", "yellow", "gray", "dark_gray"
};

DumbController::DumbController()
{
}

void DumbController::ProcessClients(vector<MinecraftConnection*> cons)
{
    playerCount = cons.size();
    vector<MinecraftConnection*> categorizedCons[DUMB_CONTROLLER_INPUT_COUNT];
    
    for (MinecraftConnection* con : cons) {
        bool categoryFound = false;
        PlayerPosition pos = con->position;
        for (int i = 1; i < DUMB_CONTROLLER_INPUT_COUNT; i++) {
            DumbControllerZone zone = zones[i];
            if (pos.x >= zone.minX && pos.x <= zone.maxX
             && pos.z >= zone.minZ && pos.z <= zone.maxZ && abs(pos.y-zone.y)<0.1) {
                categorizedCons[i].push_back(con);
                categoryFound = true;
                break;
            }
        }
        if (!categoryFound) {
            categorizedCons[0].push_back(con);
        }
    }

    for (int i = 0; i < DUMB_CONTROLLER_INPUT_COUNT; i++) {
        vector<MinecraftConnection*> cat = categorizedCons[i];
        inputPlayerCount[i] = cat.size();
        if (i >= Save) {
            float percentage = cat.size() / (float)cons.size();
            bool pressed = (percentage >= 0.5);
            if (i == Save) input.save = pressed;
            if (i == Load) input.load = pressed;
        }
        else if(i != None){
            float midX = (zones[i].minX + zones[i].maxX) * 0.5f;
            float divX = zones[i].maxX - midX;
            float midZ = (zones[i].minZ + zones[i].maxZ) * 0.5f;
            float divZ = zones[i].maxZ - midZ;

            float sumX = 0, sumZ = 0;

            for (MinecraftConnection* con : cat) {
                sumX += con->position.x;
                sumZ += con->position.z;
            }

            float avgX = fminf(fmaxf((((sumX / cat.size()) - midX) / divX),-1.0f),1.0f);
            float avgZ = fminf(fmaxf((((sumZ / cat.size()) - midZ) / divZ),-1.0f),1.0f);

            if (i >= BluePortal) { // "digital" input
                bool pressed = (avgX > 0.5);
                if (i == BluePortal)input.bluePortal = pressed;
                if (i == OrangePortal)input.redPortal = pressed;
                if (i == Jump)input.jump = pressed;
                if (i == Crouch)input.crouch = pressed;
                if (i == Use)input.use = pressed;
            }
            else { // analog input
                if (i == MovementAnalog) {
                    input.movementX = avgZ;
                    input.movementY = avgX;
                }
                if (i == ViewAnalog) {
                    input.angleX = avgZ;
                    input.angleY = avgX;
                }
            }
        }
        for (MinecraftConnection* con : cat) {
            SendDisplayPackets(con, (DumbInputType)i);
        }
    }
}

void DumbController::SendDisplayPackets(MinecraftConnection* con, DumbInputType type)
{
    if (con->currentControllerZone != type) {
        MCP::Packet removeBossBar(0x0C);
        removeBossBar.WriteUUID({ 0,69 });
        removeBossBar.WriteVarInt(1);
        removeBossBar.Send(con);

        if (type != None) {
            DumbControllerZone zone = zones[type];

            MCP::Packet bossBar(0x0C);
            bossBar.WriteUUID({ 0,69 });
            bossBar.WriteVarInt(0);
            bossBar.WriteString(zone.name);
            bossBar.WriteFloat(1);
            bossBar.WriteVarInt(zone.color);
            bossBar.WriteVarInt(0);
            bossBar.WriteByte(0);
            bossBar.Send(con);
        }
        con->currentControllerZone = type;
    }
}