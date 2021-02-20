#pragma once

#include "Packet.hpp"

struct DumbControllerData {
    float movementX = 0;
    float movementY = 0;
    float angleX = 0;
    float angleY = 0;
    float digitalAnalogs[5]{0};
    int playerCount = 0;
    int inputCounts[9]{0};
};

class DataReceiver {
private:
    DumbControllerData data;
    bool active;
    SOCKET clientSocket;
public:
    DataReceiver();
    ~DataReceiver();

    void Initialize(std::string serverIP = "127.0.0.1");
    void ReceiveData();

    bool IsActive() { return active; };
    DumbControllerData GetData() { return data; }
    void Disable();
};

extern DataReceiver* g_dataReceiver;

