#pragma once

#include "Game/Server.hpp"

class TCPServer;
class TCPSocket;
class Actor;

// actually run the game
class AuthoritativeServer : public Server
{
public:
    AuthoritativeServer();

    void SetUpTCPServer(int port);

    void BeginFrame() override;  
    void Update()override;
    void Render() const override;
    void Shutdown() override;

    void UpdateTCPUDPReference(TCPSocket* client, int toPort, int bindPort);
    void CreateUDPSocket(std::string const& ip, int toPort, int bindPort, int identifier) override;
    void SendOneMessage(std::string const& msg) override;
    void HandleUDPMessageOfIdentifier(NetMessageHeader const& header, std::string const& content, int identifier, bool reliable) override;

    void AddPlayer(Client* newClient) override;
    void RemovePlayer(Client* client) override;
    void RemoveTCPClient(TCPSocket* clientSoc);
    void SwitchPlayerMap(Client* client, std::string const& mapName) override;

    void AddEntity(Entity* entity);
    void DeleteEntity(Entity* entity);
    void ReceiveInputInfo(Client* client) override;
    void PlayGlobalSound(SoundID id) override;
    void AddRemoveMessageToClients(int entityIdxToRemove);
    void AddTeleportMessageToClients(Entity* entityToTeleport);
    void AddHealthMessageToClients(Actor* actorToUpdateHealth);

    Client* GetClientOfUDPSocket(UDPSocket* soc) const override;
    Client* GetClientOfPawnIndex(int idx) const override;
    bool IsPawnAPlayer(Entity* pawn) const override;

private:
    TCPServer* m_tcpServer = nullptr;
};