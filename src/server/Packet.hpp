#pragma once

#include "common.hpp"

#include "ServerConnection.hpp"
#include "NBT.hpp"

class ServerConnection;

namespace MCP {
    // UUID used by MC
    // https://minecraft.gamepedia.com/Universally_unique_identifier
    union UUID {
        struct { // most/least
            uint64_t most;
            uint64_t least;
        };
        struct { // int-array
            uint32_t parts[4];
        };

        UUID() {
            most = least = 0;
        }
        UUID(uint64_t mostHalf, uint64_t leastHalf) {
            most = mostHalf;
            least = leastHalf;
        }

        UUID(string uuidStr) {
            int strSize = (int)uuidStr.size();
            if (strSize < 32) uuidStr = string('0', 32-strSize) + uuidStr;
            string mostStr = string("0x") + uuidStr.substr(0, 16);
            string leastStr = string("0x") + uuidStr.substr(16, 16);
            most = stoull(mostStr,nullptr,16);
            least = stoull(leastStr,nullptr,16);
        }
    };



    class Packet{
    private:
        bool isWriter;
        char* buffer;
        int offset;

        int headerOffset = 0;
        int originalSize = 0;
    public:
        int id = 0;
    public:
        Packet(int ID);
        Packet(char* data);
        ~Packet();

        uint8_t ReadByte();
        void WriteByte(uint8_t value);
        uint16_t ReadShort();
        void WriteShort(uint16_t value);
        uint32_t ReadInt();
        void WriteInt(uint32_t value);
        uint64_t ReadLong();
        void WriteLong(uint64_t value);
        float ReadFloat();
        void WriteFloat(float value);
        double ReadDouble();
        void WriteDouble(double value);

        UUID ReadUUID();
        void WriteUUID(UUID uuid);

        std::string ReadString();
        void WriteString(std::string value);

        uint32_t ReadVarInt();
        void WriteVarInt(uint32_t var);
        uint64_t ReadVarLong();
        void WriteVarLong(uint64_t var);
    
        NBTTag ReadNBT();
        void WriteNBT(NBTTag tag);

        char* ReadByteArray(int length);
        void WriteByteArray(char* array, int length);

        int GetSize();
        int EnsureSpace(int space);
        void SetSizeHeader();
        void Send(ServerConnection* con);

        char* GetBuffer() { return buffer; };

        int GetOffset() { return offset; }
        void SetOffset(int o) { offset = o; }

        
    };
}
