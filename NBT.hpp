#pragma once

#include "common.hpp"

namespace MCP {
    enum NBTType {
        TAG_End,
        TAG_Byte, TAG_Short, TAG_Int, TAG_Long,
        TAG_Float, TAG_Double,
        TAG_ByteArray,
        TAG_String,
        TAG_List,
        TAG_Compound,
        TAG_IntArray, TAG_LongArray
    };

    class NBTList;

    class NBTTag {
    private:
        bool written;

        NBTType type;
        string name;
        char* payload;
        int headerSize;
    public:
        NBTTag();
        NBTTag(string name);
        NBTTag(char* data, bool readOnly = true);
        NBTTag(NBTType id, string name, char* payload);

        static NBTTag* Byte(string name, uint8_t value);
        static NBTTag* Short(string name, uint16_t value);
        static NBTTag* Int(string name, uint32_t value);
        static NBTTag* Long(string name, uint64_t value);
        static NBTTag* Float(string name, float value);
        static NBTTag* Double(string name, double value);
        static NBTTag* String(string name, string value);
        static NBTTag* List(string name, NBTList* value);
        static NBTTag* List(string name, NBTType type, vector<NBTTag*> list);
        static NBTTag* CompoundEnd();

        ~NBTTag();

        NBTType GetType() { return type; }

        uint8_t GetByte(bool force = false) const;
        uint16_t GetShort(bool force = false) const;
        uint32_t GetInt(bool force = false) const;
        uint64_t GetLong(bool force = false) const;
        float GetFloat(bool force = false) const;
        double GetDouble(bool force = false) const;
        string GetString(bool force = false) const;

        NBTList GetByteArray(bool force = false) const;
        NBTList GetIntArray(bool force = false) const;
        NBTList GetLongArray(bool force = false) const;
        NBTList GetList(bool force = false) const;
        NBTList GetCompound(bool force = false) const;

        void Clear();

        void SetByte(uint8_t value);
        void SetShort(uint16_t value);
        void SetInt(uint32_t value);
        void SetLong(uint64_t value);
        void SetFloat(float value);
        void SetDouble(double value);
        void SetString(string value);

        void SetList(NBTList* list);

        int size(bool noHeader = false) const;
        string ToString(int tab=0, bool showHeader=true);
        int WriteToBuffer(char* buffer, bool noHeader = false);
    };

    class NBTList {
    private:
        bool compound;
        bool commonType;
        NBTType listType = TAG_List;

        vector<NBTTag*> list;
    public:
        NBTList(char* data, bool compound); // compound and list
        NBTList(NBTType type, char* data); // other lists
        
        // initializer for all lists
        NBTList(NBTType type);
        NBTList(NBTType type, vector<NBTTag*> list);

        ~NBTList();

        int size();
        void Add(NBTTag* tag);
        vector<NBTTag*> Get() { return list; }
        string ToString(int tab=0);
        int WriteToBuffer(char* buffer);
        NBTType GetType() { return listType; };
    };
}
