#include "NBT.hpp"

namespace MCP {

    NBTTag::NBTTag()
        : type(TAG_End)
        , name("")
        , payload(nullptr) 
        , headerSize(0)
        , written(false)
    {}

    NBTTag::NBTTag(string name)
        : NBTTag()
    {
        this->name = name;
        headerSize = 3 + name.size();
    }

    NBTTag::NBTTag(NBTType id, string name, char* payload)
        : type(id)
        , name(name)
        , payload(payload)
        , written(false)
    {
        headerSize = 3 + name.size();
        size(true, true);
    }

    NBTTag::NBTTag(char* data, bool readOnly) : written(!readOnly) {
        // load id using built-in functions
        payload = data;
        type = (NBTType)GetByte(true);

        if (type == TAG_End) {
            headerSize = 1;
        }
        else {
            // do the same for name
            payload = &data[1];
            name = GetString(true);

            // set the payload to the correct offset
            uint16_t nameLen = GetShort(true);
            headerSize = 3 + nameLen;
            payload = &data[headerSize];
            // make a copy of the buffer if it's not read only
            int length = size(true, true);
            if (!readOnly) {
                char* newPayload = new char[length];
                memcpy(newPayload, payload, length);
                payload = newPayload;
            }
        }
    }

    NBTTag::~NBTTag()
    {
        Clear();
    }

    void NBTTag::Clear() {
        if (written && payload != nullptr) delete payload;
        payload = nullptr;
        written = true;
    }


    // quick NBTTag creators

    NBTTag* NBTTag::Byte(string name, uint8_t value)
    {
        NBTTag* tag = new NBTTag(name);
        tag->SetByte(value);
        return tag;
    }

    NBTTag* NBTTag::Short(string name, uint16_t value)
    {
        NBTTag* tag = new NBTTag(name);
        tag->SetShort(value);
        return tag;
    }

    NBTTag* NBTTag::Int(string name, uint32_t value)
    {
        NBTTag* tag = new NBTTag(name);
        tag->SetInt(value);
        return tag;
    }

    NBTTag* NBTTag::Long(string name, uint64_t value)
    {
        NBTTag* tag = new NBTTag(name);
        tag->SetLong(value);
        return tag;
    }

    NBTTag* NBTTag::Float(string name, float value)
    {
        NBTTag* tag = new NBTTag(name);
        tag->SetFloat(value);
        return tag;
    }

    NBTTag* NBTTag::Double(string name, double value)
    {
        NBTTag* tag = new NBTTag(name);
        tag->SetDouble(value);
        return tag;
    }

    NBTTag* NBTTag::String(string name, string value)
    {
        NBTTag* tag = new NBTTag(name);
        tag->SetString(value);
        return tag;
    }

    NBTTag* NBTTag::List(string name, NBTList* value)
    {
        NBTTag* tag = new NBTTag(name);
        tag->SetList(value);
        return tag;
    }

    NBTTag* NBTTag::List(string name, NBTType type, vector<NBTTag*> list)
    {
        NBTList* nbtlist = new NBTList(type, list);
        NBTTag* tag = NBTTag::List(name, nbtlist);
        delete nbtlist;
        return tag;
    }

    NBTTag* NBTTag::CompoundEnd()
    {
        return new NBTTag(TAG_End, "", nullptr);
    }



    // getters from payload byte buffer array

    uint8_t NBTTag::GetByte(bool force) const
    {
        if (!force && type != TAG_Byte) return 0;
        return *(uint8_t*)payload;
    }

    uint16_t NBTTag::GetShort(bool force) const 
    {
        if (!force && type != TAG_Short) return 0;
        return ((uint16_t)payload[0] << 8) + (uint16_t)payload[1];
    }

    uint32_t NBTTag::GetInt(bool force) const 
    {
        if (!force && type != TAG_Int) return 0;
        uint32_t result = 0;
        for (int i = 0; i < 4; i++) {
            uint32_t b = (uint32_t)payload[i];
            result += ((b & 0xFF) << (32 - ((i + 1) * 8)));
        }
        return result;
    }

    uint64_t NBTTag::GetLong(bool force) const 
    {
        if (!force && type != TAG_Long) return 0;
        uint64_t result = 0;
        for (int i = 0; i < 8; i++) {
            uint64_t b = (uint64_t)payload[i];
            result += ((b & 0xFF) << (64 - ((i + 1) * 8)));
        }
        return result;
        
    }

    float NBTTag::GetFloat(bool force) const 
    {
        if (!force && type != TAG_Float) return 0;
        int i = GetInt(true);
        return *(float*)&i;
    }

    double NBTTag::GetDouble(bool force) const 
    {
        if (!force && type != TAG_Double) return 0;
        uint64_t l = GetLong(true);
        return *(double*)& l;
    }

    string NBTTag::GetString(bool force) const 
    {
        if (!force && type != TAG_String) return "";
        int length = GetShort(true);
        if (length <= 0)return "";
        return std::string(&payload[2], length);
    }

    NBTList NBTTag::GetByteArray(bool force) const
    {
        if (!force && type != TAG_ByteArray) return NBTList(TAG_Byte, nullptr);
        return NBTList(TAG_Byte, payload);
    }

    NBTList NBTTag::GetIntArray(bool force) const
    {
        if (!force && type != TAG_IntArray) return NBTList(TAG_Int, nullptr);
        return NBTList(TAG_Int, payload);
    }

    NBTList NBTTag::GetLongArray(bool force) const
    {
        if (!force && type != TAG_LongArray) return NBTList(TAG_Long, nullptr);
        return NBTList(TAG_Long, payload);
    }

    NBTList NBTTag::GetList(bool force) const
    {
        if (!force && type != TAG_List) return NBTList(nullptr, false);
        return NBTList(payload, false);
    }

    NBTList NBTTag::GetCompound(bool force) const
    {
        if (!force && type != TAG_Compound) return NBTList(nullptr, true);
        return NBTList(payload, true);
    }



    // setters, working basically on the same principle

    void NBTTag::SetByte(uint8_t value)
    {
        Clear();
        type = TAG_Byte;
        payload = new char[1];
        payload[0] = value;
        size(true, true);
    }

    void NBTTag::SetShort(uint16_t value)
    {
        Clear();
        type = TAG_Short;
        payload = new char[2];
        payload[0] = (value & 0xFF00) >> 8;
        payload[1] = (value & 0x00FF);
        size(true, true);
    }

    void NBTTag::SetInt(uint32_t value)
    {
        Clear();
        type = TAG_Int;
        payload = new char[4];
        for (int i = 0; i < 4; i++) {
            payload[i] = (value & ((uint32_t)0xFF << ((3-i) * 8))) >> ((3-i)*8);
        }
        size(true, true);
    }

    void NBTTag::SetLong(uint64_t value)
    {
        Clear();
        type = TAG_Long;
        payload = new char[8];
        for (int i = 0; i < 8; i++) {
            payload[i] = (value & ((uint64_t)0xFF << ((7 - i) * 8))) >> ((7 - i) * 8);
        }
        size(true, true);
    }

    void NBTTag::SetFloat(float value)
    {
        Clear();
        SetInt(*(uint32_t*)& value);
        type = TAG_Float;
        size(true, true);
    }

    void NBTTag::SetDouble(double value)
    {
        Clear();
        SetLong(*(uint64_t*)& value);
        type = TAG_Double;
        size(true, true);
    }

    void NBTTag::SetString(string value)
    {
        Clear();
        type = TAG_String;
        payload = new char[value.size() + 2];
        payload[0] = (value.size() & 0xFF00) >> 8;
        payload[1] = (value.size() & 0x00FF);

        const char* c = value.c_str();
        for (size_t i = 0; i < value.size(); i++) {
            payload[2 + i] = c[i];
        }
        size(true, true);
    }

    void NBTTag::SetList(NBTList* list)
    {
        Clear();
        type = list->GetType();
        payload = new char[list->size()];
        list->WriteToBuffer(payload);
        size(true, true);
    }


    // get size of the tag (full size is header + payload size)
    // also used to recalculate the size when changing the tag
    int NBTTag::size(bool noHeader, bool recalculate) {
        int size = 0;
        if (recalculate) {
            switch (type) {
            case TAG_Byte:  size += 1; break;
            case TAG_Short: size += 2; break;
            case TAG_Int: case TAG_Float:
                size += 4; break;
            case TAG_Long: case TAG_Double:
                size += 8; break;
            case TAG_ByteArray:
                size += GetByteArray(true).size();
                break;
            case TAG_IntArray:
                size += GetIntArray(true).size();
                break;
            case TAG_LongArray:
                size += GetLongArray(true).size();
                break;
            case TAG_List:
                size += GetList(true).size();
                break;
            case TAG_Compound:
                size += GetCompound(true).size();
                break;
            case TAG_String:
                size += 2 + GetString(true).size();
                break;
            default: break;
            }
            tagSize = size;
        }
        else {
            size = tagSize;
        }
        size += noHeader ? 0 : headerSize;
        return size;
    }

    // prints the nbt tag in a nice way
    string NBTTag::ToString(int tab, bool showHeader) {
        string output = string(tab * 2, ' ');
        if (showHeader) {
            static string typeNames[13] = {
                "End", "Byte", "Short", "Int", "Long",
                "Float", "Double", "ByteArray", "String",
                "List", "Compound", "IntArray", "LongArray"
            };
            output += "(" + typeNames[(int)type] + ") ";
            if(name.size()>0) output += name + ": ";
        }

        switch (type) {
        case TAG_Byte:
            output += to_string((int)GetByte()); break;
        case TAG_Short:
            output += to_string(GetShort()); break;
        case TAG_Int:
            output += to_string(GetInt()); break;
        case TAG_Long:
            output += to_string(GetLong()); break;
        case TAG_Float:
            output += to_string(GetFloat()); break;
        case TAG_Double:
            output += to_string(GetDouble()); break;
        case TAG_ByteArray:
            output += GetByteArray().ToString(tab); break;
        case TAG_IntArray:
            output += GetIntArray().ToString(tab); break;
        case TAG_LongArray:
            output += GetLongArray().ToString(tab); break;
        case TAG_List:
            output += GetList().ToString(tab); break;
        case TAG_Compound:
            output += GetCompound().ToString(tab); break;
        case TAG_String: {
            string str = GetString();
            if (str.size() > 50)str = str.substr(0, 50) + " ...";
            output += str;
            break;
        }
        default: break;
        }

        return output;
    }

    // write the tag to custom byte buffer
    int NBTTag::WriteToBuffer(char* buffer, bool noHeader) {
        int s = size(true, true);

        int offset = 0;

        if (!noHeader) {
            buffer[0] = type;
            buffer[1] = (name.size() & 0xFF00) >> 8;
            buffer[2] = (name.size() & 0x00FF);
            for (size_t i = 0; i < name.size(); i++) {
                buffer[3 + i] = name[i];
            }
            offset = 3 + name.size();
        }

        memcpy(&buffer[offset], payload, s);

        return s + offset;
    }




    // NBTList - object for all list-typed tag payloads

    NBTList::NBTList(char* data, bool compound)
        : compound(compound)
        , commonType(!compound)
    {
        if (data == nullptr) return;

        if (compound) {
            int offset = 0;

            NBTTag* tag;
            do {
                tag = new NBTTag(&data[offset]);
                offset += tag->size();
                list.push_back(tag);
            } while (tag->GetType() != TAG_End);
        }
        else {
            NBTType type = (NBTType)data[0];
            NBTTag nbtsize(TAG_Int, "", &data[1]);
            int size = nbtsize.GetInt();

            int offset = 5;

            for (int i = 0; i < size; i++) {
                NBTTag* tag = new NBTTag(type, "", &data[offset]);
                offset += tag->size(true);
                list.push_back(tag);
            }
        }

        listType = (compound) ? TAG_Compound : TAG_List;
    }

    NBTList::NBTList(NBTType type, char* data)
        : compound(false)
        , commonType(false)
    {
        if (data == nullptr) return;

        NBTTag nbtsize(TAG_Int, "", &data[0]);
        int size = nbtsize.GetInt();

        int offset = 4;

        for (int i = 0; i < size; i++) {
            NBTTag* tag = new NBTTag(type, "", &data[offset]);
            offset += tag->size(true);
            list.push_back(tag);
        }

        switch (type) {
            case TAG_Byte: listType = TAG_ByteArray; break;
            case TAG_Int: listType = TAG_IntArray; break;
            case TAG_Long: listType = TAG_LongArray; break;
            default: listType = TAG_List;
        }
    }

    NBTList::NBTList(NBTType type) 
    {
        this->listType = type;

        compound = (type == TAG_Compound);
        commonType = (type == TAG_List);
    }

    NBTList::NBTList(NBTType type, vector<NBTTag*> nbtlist) : NBTList(type)
    {
        for (NBTTag* tag : nbtlist) {
            Add(tag);
        }
    }

    NBTList::~NBTList() {
        for (NBTTag* tag : list) {
            delete tag;
        }

        list.clear();
    }

    int NBTList::size() {
        int size = 0;
        if (!compound) size += 4; // all other containers start with 4byte size field
        if (commonType) size += 1; // non-fixed type container (list) has 1 byte for common type

        for (NBTTag* t : list) {
            // only compound container has named tags
            size += t->size(!compound);
        }
        return size;
    }

    void NBTList::Add(NBTTag* tag) {
        if (compound) {
            if (list.size() == 0 || (*(list.end() - 1))->GetType() != TAG_End) {
                list.push_back(NBTTag::CompoundEnd());
            }
            list.insert(list.end() - 1, tag);
        }
        else {
            list.push_back(tag);
        }
    }

    string NBTList::ToString(int tab) {
        string output;
        output += "{\n";
        for (NBTTag* t : list) {
            if (t->GetType() == TAG_End)continue;
            output += t->ToString(tab+1, compound) + "\n";
        }
        output += string(tab * 2, ' ') + "}";
        return output;
    }

    int NBTList::WriteToBuffer(char* buffer) {

        int offset = 0;

        if (commonType) {
            buffer[offset] = list.size() > 0 ? list.at(0)->GetType() : TAG_Int;
            offset += 1;
        }
        if (!compound) {
            int size = list.size();
            for (int i = 0; i < 4; i++) {
                buffer[offset+i] = (size & ((uint32_t)0xFF << ((3 - i) * 8))) >> ((3 - i) * 8);
            }
            offset += 4;
        }

        for (NBTTag* t : list) {
            
            int size = t->WriteToBuffer(&buffer[offset], !compound);
            offset += size;
        }

        return offset;

    }
}