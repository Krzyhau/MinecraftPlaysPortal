#include "Chat.hpp"

Chat* g_chat;

Chat::Chat(MinecraftServer* server)
    : server(server)
{
    
}

// converts ChatMessage object into JSON that can be sent to other clients
string Chat::ChatMessageToString(ChatMessage msg)
{
    string chatMsg = "[{\"text\":\"\"},";

    string fixedMsg = SanitizeInput(msg.message);
    // just for safety. I'm already doing this during login
    string playerName = SanitizeInput(msg.sender->playerName); 

    if (msg.type == Join) {
        chatMsg += "{\"text\":\"+ " + playerName + "\",\"color\":\"green\"}]";
    }
    else if (msg.type == Leave) {
        chatMsg += "{\"text\":\"- " + playerName + "\",\"color\":\"red\"}]";
    }
    else if (msg.type == Message) {
        chatMsg += "{\"text\":\"" + playerName + ": \",\"bold\":true,\"color\":\"";
        chatMsg += DumbController::inputColors[msg.sender->lastControllerZone];
        chatMsg += "\"},{\"text\":\"" + fixedMsg + "\",\"color\":\""+msg.color+"\"}]";
    }
    else if (msg.type == Info) {
        chatMsg += "{\"text\":\"" + fixedMsg + "\",\"color\":\"" + msg.color + "\"}]";
    }
    else if (msg.type == Raw) {
        chatMsg = msg.message;
    }

    return chatMsg;
}

// sanitizes the string message for proper json format
// avoiding fucking crash caused by posting quotation mark lol
string Chat::SanitizeInput(string raw)
{
    string fixed = "";
    for (const char& c : raw) {
        switch (c) {
        case 0x08: fixed += "\\b"; break;
        case 0x0C: fixed += "\\f"; break;
        case 0x0A: fixed += "\\n"; break;
        case 0x0D: fixed += "\\r"; break;
        case 0x09: fixed += "\\t"; break;
        case 0x22: fixed += "\\\""; break;
        case 0x5C: fixed += "\\\\"; break;
        default: fixed += c;
        }
    }
    return fixed;
}

// sends given message object to given client
// if client is not given, sending the message to everyone on the server
void Chat::Send(ChatMessage msg, MinecraftConnection* receiver)
{
    // attempt to process the message as a command first
    if (ProcessCommandMsg(msg)) return;

    // debug the message
    if (msg.type == Message) {
        cout << "[CHAT] " << msg.sender->playerName << ": " << SanitizeInput(msg.message) << endl;
    }

    string processedMsg = ChatMessageToString(msg);

    if (receiver == nullptr) {
        for (MinecraftConnection* con : server->GetInGameConnections()) {
            SendRawTo(processedMsg, con);
        }
    }
    else {
        SendRawTo(processedMsg, receiver);
    }
}

void Chat::SendRawTo(string msg, MinecraftConnection* receiver) 
{
    MCP::Packet messagePacket(0x0E);
    messagePacket.WriteString(msg);
    messagePacket.WriteByte(0);
    messagePacket.WriteUUID({ 0,0 });
    messagePacket.Send(receiver);
}

bool Chat::ProcessCommandMsg(ChatMessage msg)
{
    if (msg.type != Message)return false;
    if (msg.message[0] != '/') return false;

    string cmd, param = "";

    int spacePos = msg.message.find(' ');
    if (spacePos != string::npos) {
        cmd = msg.message.substr(1, spacePos - 1);
        if (spacePos + 1 < msg.message.size()) {
            param = msg.message.substr(spacePos+1);
        }
    }
    else {
        cmd = msg.message.substr(1, msg.message.size() - 1);
    }
    

    cout << "[COMMAND] " << msg.sender->playerName << " executed \"/" << cmd << " " << param << "\"" << endl;
    CommandCallback callback = nullptr;
    for (Command& command : commands) {
        if (command.name == cmd) {
            callback = command.callback;
            break;
        }
    }
    if (callback) {
        callback(msg.sender, param);
    }
    else {
        string invalid = "Invalid command: " + SanitizeInput(cmd);
        Send({ msg.sender, invalid, Info, "red" }, msg.sender);
    }

    return true;
}

void Chat::AddCommand(string name, CommandCallback callback)
{
    for (Command& cmd : commands) {
        if (cmd.name == name)return;
    }
    commands.push_back({ name,callback });
}
