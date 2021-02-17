
#include "common.hpp"

#include "MinecraftServer.hpp"
#include "NBT.hpp"
#include <fstream>

#include "MinecraftServerResources.hpp"




int main() {
    gMCServer->Start();
    gMCServer->Stop();
    return 0;
}
