#include "Packet.hpp"

#include "ServerConnection.hpp"

namespace MCP {

    Packet::Packet(int ID)
        : isWriter(true)
        , offset(0)
        , id(ID)
    {
        isWriter = true;
        buffer = new char[MC_MAX_PACKET_SIZE];
        for (int i = 0; i < MC_MAX_PACKET_SIZE; i++) {
            buffer[i] = 0;
        }

        WriteVarInt(ID);
    }

    Packet::Packet(char* data)
        : isWriter(false)
        , offset(0) 
    {
        buffer = data;

        originalSize = ReadVarInt();
        headerOffset = offset;
        id = ReadVarInt();
    }

    Packet::~Packet()
    {
        if(isWriter) delete buffer;
    }

    // byte

    uint8_t Packet::ReadByte()
    {
        uint8_t result = (uint8_t)(buffer[offset]);
        offset += 1;
        return result;
    }
    void Packet::WriteByte(uint8_t value)
    {
        if (offset < MC_MAX_PACKET_SIZE) {
            buffer[offset] = value;
            offset++;
        }
    }

    // short

    uint16_t Packet::ReadShort()
    {
        uint16_t result = ((uint16_t)ReadByte() << 8) + (uint16_t)ReadByte();
        return result;
    }
    void Packet::WriteShort(uint16_t value)
    {
        WriteByte((value & 0xFF00) >> 8);
        WriteByte((value & 0x00FF));
    }

    // int 

    uint32_t Packet::ReadInt()
    {
        uint32_t result = ((uint32_t)ReadShort() << 16) + (uint32_t)ReadShort();
        return result;
    }
    void Packet::WriteInt(uint32_t value)
    {
        WriteShort((value & 0xFFFF0000) >> 16);
        WriteShort((value & 0x0000FFFF));
    }

    // long

    uint64_t Packet::ReadLong()
    {
        uint64_t result = ((uint64_t)ReadInt() << 32) + (uint64_t)ReadInt();
        return result;
    }
    void Packet::WriteLong(uint64_t value)
    {
        WriteInt((value & 0xFFFFFFFF00000000) >> 32);
        WriteInt((value & 0x00000000FFFFFFFF));
    }

    // float
    float Packet::ReadFloat()
    {
        uint32_t intResult = ReadInt();
        float result = *(float*)(&intResult);
        return result;
    }
    void Packet::WriteFloat(float value)
    {
        uint32_t iValue = *(uint32_t*)(&value);
        WriteInt(iValue);
    }

    // double
    double Packet::ReadDouble()
    {
        uint64_t doubleResult = ReadLong();
        double result = *(double*)(&doubleResult);
        return result;
    }
    void Packet::WriteDouble(double value)
    {
        uint64_t lValue = *(uint64_t*)(&value);
        WriteLong(lValue);
    }

    UUID Packet::ReadUUID()
    {
        UUID result(ReadLong(), ReadLong());
        return result;
    }

    void Packet::WriteUUID(UUID uuid)
    {
        WriteLong(uuid.most);
        WriteLong(uuid.least);
    }



    // var int

    uint32_t Packet::ReadVarInt()
    {
        int numRead = 0;
        int result = 0;
        uint8_t read = 0;
        do {
            read = ReadByte();
            int value = (read & 0b01111111);
            result |= (value << (7 * numRead));

            numRead++;
            if (numRead > 5)
                throw "VarInt is too big";
        } while ((read & 0b10000000) != 0);

        return result;
    }
    void Packet::WriteVarInt(uint32_t var)
    {
        WriteVarLong(var);
    }

    // var long
    uint64_t Packet::ReadVarLong()
    {
        int numRead = 0;
        long result = 0;
        uint8_t read;
        do {
            read = ReadByte();
            long value = (read & 0b01111111);
            result |= (value << (7 * numRead));

            numRead++;
            if (numRead > 10) {
                throw "VarLong is too big";
            }
        } while ((read & 0b10000000) != 0);

        return result;
    }
    void Packet::WriteVarLong(uint64_t var)
    {
        do {
            uint8_t temp = (uint8_t)(var & 0b01111111);
            var >>= 7;
            if (var != 0) {
                temp |= 0b10000000;
            }
            WriteByte(temp);
        } while (var != 0);
    }

    NBTTag Packet::ReadNBT()
    {
        NBTTag nbt(&buffer[offset]);
        offset += nbt.size();
        return nbt;
    }
    void Packet::WriteNBT(NBTTag tag)
    {
        int nbtsize = tag.size();
        if (offset + nbtsize < MC_MAX_PACKET_SIZE) {
            tag.WriteToBuffer(&buffer[offset]);
            offset += nbtsize;
        }
    }

    char* Packet::ReadByteArray(int length)
    {
        char* result = &buffer[offset];
        offset += length;
        return result;
    }

    void Packet::WriteByteArray(char* array, int length)
    {
        if (offset+length < MC_MAX_PACKET_SIZE) {
            memcpy(&buffer[offset], array, length);
            offset+=length;
        }
    }

    // string

    std::string Packet::ReadString()
    {
        // VarInt storing a number of UTF-8 characters in a string
        int length = ReadVarInt();

        if (length <= 0)return "";

        int oldOffset = offset;
        int charCount = 0;

        while (charCount < length) {
            uint8_t byte = ReadByte();
            charCount++;
        }

        return std::string(buffer + oldOffset, offset - oldOffset);
    }
    void Packet::WriteString(std::string value)
    {
        int charCount = 0;

        for (unsigned char c : value) {
            charCount++;
        }

        WriteVarInt(charCount);

        if (charCount > 0) {
            for (unsigned char c : value) {
                WriteByte(c);
            }
        }

    }



    int Packet::GetSize() {
        if (isWriter) return offset;
        else return headerOffset + originalSize;
    }

    void Packet::SetSizeHeader()
    {
        if (!isWriter)return;

        // store original buffer somewhere else and create temp buffer
        char* oldbuffer = buffer;
        buffer = new char[MC_MAX_PACKET_SIZE];
    
        // write the packet size (offset) to the beginning of the buffer
        int oldOffset = offset - headerOffset;
        offset = 0;
        WriteVarInt(oldOffset);

        // copy the memory to the original buffer
        memcpy(buffer + offset, oldbuffer + headerOffset, oldOffset);
        delete oldbuffer;

        // store offset in header offset for later use
        headerOffset = offset;

        // fix offset
        offset += oldOffset;
    }

    void Packet::Send(ServerConnection* con) {
        // refresh size header with the size of currently possesed data
        SetSizeHeader(); 
        // send the data
        con->Send(GetBuffer(), GetSize());
    }
}