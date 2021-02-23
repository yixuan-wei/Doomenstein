#pragma once

#include <string>
#include <vector>
#include "Game/GameCommon.hpp"

class Entity;
class Server;
class UDPSocket;
class TCPSocket;
typedef size_t SoundID;

class Client
{
public:    
    static Client* StartClientWithBool(bool isPlayerClient);
    virtual ~Client() {}

    virtual void Startup(Entity* playerPawn, Server* server) = 0;
    virtual void BeginFrame() = 0;
    virtual void Update() = 0;
    virtual void Render() const = 0;
    virtual void EndFrame() = 0;
    virtual void Shutdown();

    virtual void SendReliableMessages();
    virtual void InsertDeleteMsg(std::string const& deleteMsg);
    virtual void InsertHealthMsg(std::string const& healthMsg);
    virtual void InsertTeleportMsg(std::string const& teleportMsg);
    virtual void InsertCreateMsg(std::string const& createMsg);
    virtual void ReceiveACKMessage(std::string const& content);
    virtual bool PlaySoundOnClient(size_t id) = 0;

    virtual bool CouldUpdateInput() const = 0;
    std::string MakeQuitPackage() const;

public:
    bool m_isQuiting = false;
    bool m_isRemote = false;
    Entity* m_playerPawn = nullptr;
    Server* m_server = nullptr;

    InputInfo m_input;
    bool m_shot = false;

    TCPSocket* m_tcpClientSocket = nullptr;
    UDPSocket* m_udpSocket = nullptr;
    int m_identifier = -1;

    std::vector<std::string> m_reliableMsgs;
};
