#pragma once

#include "Packet.hpp"

namespace MCP {

    class ChunkSection {
    private:
        short blocks[16][16][16]{0};
        short blockCount = 0;
    public:
        void SetBlockID(int x, int y, int z, short id);
        short GetBlockID(int x, int y, int z);
        short GetBlockCount() { return blockCount; }
    };

    class Chunk {
    public:
        int x; // x location of chunk divided by 16
        int z; // z location of chunk divided by 16
        ChunkSection sections[16];
        short biomeArray[1024]{0};

        Chunk(int x, int z);
        void SetBlockID(int x, int y, int z, short id);
        short GetBlockID(int x, int y, int z);
        uint32_t GetPrimaryBitMask();
        int GetSentSectionsCount();
    };

    class ChunkWorld {
    public:
        vector<Chunk*> chunks;

        void SetBlockID(int x, int y, int z, short id);
        short GetBlockID(int x, int y, int z);

        Chunk* GetChunk(int chunkX, int chunkZ);
    };

    class ChunkPacket : public Packet {
    public:
        ChunkPacket(Chunk* c);
    };

    class FullBrightLightChunkPacket : public Packet {
    public:
        FullBrightLightChunkPacket(Chunk* c);
    };
}


