#pragma once

#include "common.hpp"

#include "MinecraftServer.hpp"

class MinecraftServer;

enum ChatMessageType {
    Message,
    Join,
    Leave,
    Info,
    Raw
};

struct ChatMessage {
    MinecraftConnection* sender;
    ChatMessageType type;
    string message;
    string color;

    ChatMessage(MinecraftConnection* sender, string message) 
        : sender(sender), message(message), type(Message), color("white") {};
    ChatMessage(MinecraftConnection* sender, ChatMessageType type)
        : sender(sender), type(type) {};
    ChatMessage(MinecraftConnection* sender, string message, ChatMessageType type)
        : sender(sender), message(message), type(type), color("white") {};
    ChatMessage(MinecraftConnection* sender, string message, ChatMessageType type, string color)
        : sender(sender), message(message), type(type), color(color) {};
};

typedef void(*CommandCallback)(MinecraftConnection* sender, string arg);

struct Command {
    string name;
    CommandCallback callback;
};

class Chat {
private:
    MinecraftServer* server;
    vector<Command> commands;
public:
    Chat(MinecraftServer* server);

    string ChatMessageToString(ChatMessage msg);
    string SanitizeInput(string raw);

    void Send(ChatMessage msg, MinecraftConnection* receiver = nullptr);
    void SendRawTo(string msg, MinecraftConnection* receiver);

    bool ProcessCommandMsg(ChatMessage msg);
    void AddCommand(string name, CommandCallback callback);
};

extern Chat* g_chat;

