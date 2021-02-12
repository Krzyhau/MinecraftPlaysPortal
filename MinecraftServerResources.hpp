#pragma once

#include <string>
#include "common.hpp"
#include "NBT.hpp"
#include <fstream>

namespace MCP {
    static NBTTag* GetDimensionTypeNBT() {
        return NBTTag::List("element", TAG_Compound, {
            NBTTag::Byte("piglin_safe",0),
            NBTTag::Byte("natural",0),
            NBTTag::Float("ambient_light",0.0f),
            NBTTag::String("infiniburn","minecraft:infiniburn_end"),
            NBTTag::Byte("respawn_anchor_works", 0),
            NBTTag::Byte("has_skylight",0),
            NBTTag::Byte("bed_works",0),
            NBTTag::String("effects","minecraft:the_end"),
            NBTTag::Byte("has_raids",1),
            NBTTag::Int("logical_height",256),
            NBTTag::Double("coordinate_scale",1.0),
            NBTTag::Byte("ultrawarm",0),
            NBTTag::Byte("has_ceiling",0),
            NBTTag::Int("fixed_time",6000),
        });
    }
    static NBTTag GetDimensionCodecNBT() {
        //storing the tag in a static pointer
        static NBTTag nbt;
        static bool created = false;
        if (!created) {
            // binary mode is only for switching off newline translation
            std::ifstream nbtFile("res/dimension_registry.nbt", std::ios::binary);

            nbtFile.seekg(0, std::ios::end);
            uint32_t nbtFileSize = (uint32_t)nbtFile.tellg();
            nbtFile.seekg(0, std::ios::beg);

            char* buffer = new char[nbtFileSize];
            nbtFile.read(buffer, nbtFileSize);

            nbt = NBTTag(buffer, true);
            created = true;
        }
        return nbt;
    }

    static std::string GetServerFaviconBase64() {
        return 
            "data:image/png;base64,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"
            "5ErkJggg==";
    }

    static string GetServerInfo(MinecraftConnection* con) {
        return
        "{"
            "\"version\": {"
                "\"name\": \"1.16.4\","
                "\"protocol\": " + to_string(con->protocolVer) + ""
            "},"
            "\"players\": {"
                "\"max\": -69,"
                "\"online\": 0"
            "},"
            "\"description\": {"
                "\"text\": \"Minecraft Plays Portal 2\""
            "},"
            "\"favicon\": \""+ GetServerFaviconBase64() +"\""
        "}";
    }

    static string HttpGetRequest(string address, string request) {
        string output("");

        addrinfo hints = {0}, * host;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        int status = getaddrinfo(address.c_str(), "443", &hints, &host);
        if(status != 0) return output;

        SOCKET webSock = socket(host->ai_family, host->ai_socktype, host->ai_protocol);

        if (connect(webSock, host->ai_addr, host->ai_addrlen) != 0) {
            return output;
        }

        string get_request("GET ");
        get_request += request + " HTTP/1.1\r\nHost: " + address + "\r\nConnection: close\r\n\r\n";
        send(webSock, get_request.c_str(), strlen(get_request.c_str()), 0);


        char* buffer = new char[MC_MAX_PACKET_SIZE];
        int dataSize = recv(webSock, buffer, 10000, 0);
        
        if (dataSize > 0) {
            output = string(buffer, dataSize);
        }

        closesocket(webSock);
        freeaddrinfo(host);
        delete[] buffer;
        return output;
    }
}