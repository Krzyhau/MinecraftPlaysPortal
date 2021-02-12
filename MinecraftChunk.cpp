#include "MinecraftChunk.hpp"

#include "common.hpp"

namespace MCP {

    Chunk::Chunk(int x, int z)
        : x(x)
        , z(z)
    {

    }

    void Chunk::SetBlockID(int x, int y, int z, short id)
    {
        sections[(int)floor(y / 16.0)].SetBlockID(x, y % 16, z, id);
    }

    short Chunk::GetBlockID(int x, int y, int z)
    {
        return sections[(int)floor(y / 16.0)].GetBlockID(x, y % 16, z);
    }

    uint32_t Chunk::GetPrimaryBitMask()
    {
        uint32_t mask = 0;
        for (int i = 15; i >= 0; i--) {
            mask = mask << 1;
            if (sections[i].GetBlockCount() > 0)mask += 1;
        }
        return mask;
    }

    int Chunk::GetSentSectionsCount()
    {
        int count = 0;
        for (int i = 0; i < 16; i++) {
            if (sections[i].GetBlockCount() > 0)count++;
        }
        return count;
    }





    void ChunkSection::SetBlockID(int x, int y, int z, short id)
    {
        assert(x < 16 && y < 16 && z < 16 && x >= 0 && y>=0 && z>=0);

        short prevID = GetBlockID(x, y, z);
        blocks[x][y][z] = id;

        if (prevID == 0 && id != 0)blockCount++;
        if (prevID != 0 && id == 0)blockCount--;
    }

    short ChunkSection::GetBlockID(int x, int y, int z)
    {
        if (x >= 16 || y >= 16 || z >= 16 || x<0 || y<0 || z<0) return 0;
        else return blocks[x][y][z];
    }





    void ChunkWorld::SetBlockID(int x, int y, int z, short id)
    {
        int chunkX = (int)floor(x / 16.0);
        int chunkZ = (int)floor(z / 16.0);
        Chunk * c = GetChunk(chunkX, chunkZ);

        //if there's no chunk, create one
        if (c == nullptr && id > 0) {
            c = new Chunk(chunkX, chunkZ);
            chunks.push_back(c);
        }

        if (c == nullptr) return;

        int blockX = x - chunkX * 16;
        int blockZ = z - chunkZ * 16;

        c->SetBlockID(blockX, y, blockZ, id);

        // if cleaned up whole chunk, remove it from list
        if (c->GetSentSectionsCount() == 0) {
            for (auto it = chunks.begin(); it != chunks.end(); it++) {
                if ((*it)->x == chunkX && (*it)->z == chunkZ) {
                    chunks.erase(it);
                    break;
                }
            }
        }
    }

    short ChunkWorld::GetBlockID(int x, int y, int z)
    {
        int chunkX = (int)floor(x / 16.0);
        int chunkZ = (int)floor(z / 16.0);
        Chunk * c = GetChunk(chunkX, chunkZ);
        if (c != nullptr) {
            return c->GetBlockID(x % 16, y, z % 16);
        }
        return 0;
    }

    Chunk* ChunkWorld::GetChunk(int chunkX, int chunkZ)
    {
        for (Chunk* chunk : chunks) {
            if (chunk->x == chunkX && chunk->z == chunkZ) {
                return chunk;
            }
        }
        return nullptr;
    }





    ChunkPacket::ChunkPacket(Chunk* c) : Packet(0x20)
    {
        WriteInt(c->x); // chunk X coords
        WriteInt(c->z); // chunk Z coords
        bool fullChunk = true;
        WriteByte(fullChunk ? 1 : 0); // full chunk bool
        WriteVarInt(c->GetPrimaryBitMask()); // primary bit mask (what chunks will be sent)
        WriteInt(0x0A000000); // heightmap NBT list. not needed for what I do, so I send empty compound
        if (fullChunk) {
            // biome array of 4x4x4 sections
            WriteVarInt(1024);
            for (int i = 0; i < 1024; i++) {
                WriteVarInt(c->biomeArray[i]);
            }
        }
        // size of upcoming data (one chunk section is 8192 + 2 + 1 + 2)
        WriteVarInt(c->GetSentSectionsCount() * 8197);
        // writing every chunk section
        for (int i = 0; i < 16; i++) {
            ChunkSection& cs = c->sections[i];
            if (cs.GetBlockCount() == 0)continue;

            WriteShort(cs.GetBlockCount()); // block count
            WriteByte(15); // bits per block, using max bytes for direct pallete (15 for 1.16 smh)
            WriteVarInt(1024); // count of longs that will be sent
            for (int y = 0; y < 16; y++) for (int z = 0; z < 16; z++) for (int x = 0; x < 16; x += 4) {
                uint64_t data = 0;
                for (int b = 3; b >= 0; b--) {
                    data = data << 15;
                    data += cs.GetBlockID(x + b, y, z);
                }
                WriteLong(data);
            }
        }
        // block entities. i don't need them, so simply sending empty array
        WriteVarInt(0);
    }

    FullBrightLightChunkPacket::FullBrightLightChunkPacket(Chunk* c) : Packet(0x23)
    {
        WriteVarInt(c->x); // Chunk X
        WriteVarInt(c->z); // Chunk Y
        WriteByte(1); // trust edges?

        uint32_t lightMask = c->GetPrimaryBitMask() << 1;

        WriteVarInt(lightMask); // sky light mask
        WriteVarInt(lightMask); // block light mask
        WriteVarInt(lightMask ^ 0b111111111111111111); // empty sky light mask
        WriteVarInt(lightMask ^ 0b111111111111111111); // empty block light mask

        int sectionCount = c->GetSentSectionsCount();

        // sky light
        for (int i = 0; i < sectionCount; i++) {
            WriteVarInt(2048);
            for (int i = 0; i < 256; i++) {
                WriteLong(0xFFFFFFFFFFFFFFFF);
            }
        }
        
        //block light
        for (int i = 0; i < sectionCount; i++) {
            WriteVarInt(2048);
            for (int i = 0; i < 256; i++) {
                WriteLong(0xFFFFFFFFFFFFFFFF);
            }
        }
    }

}
