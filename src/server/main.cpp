
#include "common.hpp"

#include "MinecraftServer.hpp"
#include "NBT.hpp"
#include <fstream>

#include "MinecraftServerResources.hpp"




int main() {
    g_mcServer->Start();
    g_mcServer->Stop();
    return 0;
}
