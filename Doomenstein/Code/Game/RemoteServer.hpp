#pragma once

#include "Game/Server.hpp"

class TCPClient;

// sync with authoritative server
class RemoteServer : public Server 
{
public:
    RemoteServer();

    void SetUpTCPClient(std::string const& serverIp, int port);

    void BeginFrame() override;  //receive updated info
    void Render() const override;
    void Shutdown() override;

    void CreateUDPSocket(std::string const& ip, int toPort, int bindPort, int identifier) override;
    void SendReliableMessages() override;
    void SendOneMessage(std::string const& msg) override;
    void HandleUDPMessageOfIdentifier(NetMessageHeader const& header, std::string const& content, int identifier, bool reliable) override;
    void HandleEntityCreateMessage(std::string const& content);

    void RequestAddPlayer();
    void AddPlayer(Client* newClient) override;
    void RemoveTCPClient(TCPClient* client);
    void SwitchPlayerMap(Client* client, std::string const& mapName) override;
    void ActualSwitchPlayerMap(Entity* e, std::string const& mapName);

    void ReceiveInputInfo(Client* client) override;

    void PlayGlobalSound(SoundID id) override;

    Client* GetClientOfUDPSocket(UDPSocket* soc) const override;
    Client* GetClientOfPawnIndex(int idx) const override;
    bool IsPawnAPlayer(Entity* pawn) const override;

public:
    std::string m_serverIP;

private:
    TCPClient* m_tcpClient = nullptr;
};