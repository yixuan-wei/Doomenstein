#pragma once

#include "Game/Game.hpp"
#include <vector>

class Client; 
class UDPSocket;
class TCPClient;
struct NetMessageHeader;
typedef size_t SoundID;

class Server
{
public:
    Server() =default;
    virtual ~Server() =default;

    virtual void Startup(eGameType gameType);
    virtual void BeginFrame();
    virtual void Update();
    virtual void Render() const = 0;
    virtual void EndFrame();
    virtual void Shutdown() = 0;

    virtual void CreateUDPSocket(std::string const& ip, int toPort, int bindPort, int identifier) =0;
    virtual void SendOneMessage(std::string const& msg) = 0;
    virtual void SendReliableMessages();
    virtual void HandleUDPMessageOfIdentifier(NetMessageHeader const& header, std::string const& content, int identifier, bool reliable)=0;

    virtual void AddPlayer(Client* newClient) = 0;
    virtual void RemovePlayer(Client* client);
    virtual void RemovePlayerOfIdentifier(int identifier);
    virtual void SwitchPlayerMap(Client* client, std::string const& mapName) = 0;

    virtual void ReceiveInputInfo(Client* client) = 0;

    virtual void PlayGlobalSound(SoundID id) = 0;

    virtual Client* GetClientOfUDPSocket(UDPSocket* soc) const = 0;
    virtual Client* GetClientOfPawnIndex(int idx) const =0;
    virtual bool IsPawnAPlayer(Entity* pawn) const = 0;

public:
    Game* m_theGame = nullptr;
    bool m_isAuthoritative = true;

    //Multi clients
    std::vector<Client*> m_clients;
};