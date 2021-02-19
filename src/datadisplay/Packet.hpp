#pragma once

#include <stdint.h>
#include <string>
#include <winsock2.h>

class ServerConnection;

namespace MCP {
    
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

        char* ReadByteArray(int length);
        void WriteByteArray(char* array, int length);

        int GetSize();
        int EnsureSpace(int space);
        void SetSizeHeader();
        void Send(SOCKET socket);

        char* GetBuffer() { return buffer; };

        int GetOffset() { return offset; }
        void SetOffset(int o) { offset = o; }

        
    };
}



