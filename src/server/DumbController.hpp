#pragma once

#include "MinecraftServer.hpp"

class MinecraftConnection;

enum DumbInputType {
    None,

    MovementAnalog,
    ViewAnalog,

    BluePortal,
    OrangePortal,
    Jump,Crouch,Use,

    Save, Load
};

#define DUMB_CONTROLLER_INPUT_COUNT 10


struct DumbControllerInputs {
    float movementX = 0;
    float movementY = 0;

    float angleX = 0;
    float angleY = 0;

    bool bluePortal = 0;
    bool redPortal = 0;
    bool jump = 0;
    bool crouch = 0;
    bool use = 0;

    bool save = 0;
    bool load = 0;
};

struct DumbControllerZone {
    float minX;
    float minZ;
    float maxX;
    float maxZ;
    float y;
    string name;
    int color;
};

class DumbController
{
private:
    int playerCount = 0;
    vector<MinecraftConnection*> categorizedConnections[DUMB_CONTROLLER_INPUT_COUNT];
    DumbControllerInputs input;

    float digitalAnalogs[5] {0}; // peak of comedy.
public:
    DumbController();
    
    void ProcessClients(vector<MinecraftConnection*> cons);
    void SendDisplayPackets(MinecraftConnection* con, DumbInputType type);

    void SendInputPackets(MinecraftConnection* con, bool detailed);

    static DumbControllerZone zones[DUMB_CONTROLLER_INPUT_COUNT];
    static string inputColors[DUMB_CONTROLLER_INPUT_COUNT];
};

extern DumbController* g_dumbController;

