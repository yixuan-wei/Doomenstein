#include "Game/Server.hpp"
#include "Game/AuthoritativeServer.hpp"
#include "Game/RemoteServer.hpp"
#include "Game/SingleplayerGame.hpp"
#include "Game/MultiplayerGame.hpp"
#include "Game/NetworkMessage.hpp"
#include "Game/PlayerClient.hpp"
#include "Game/App.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Network/TCPData.hpp"
#include "Engine/Network/NetworkCommon.hpp"
#include "Engine/Network/TCPSocket.hpp"

//////////////////////////////////////////////////////////////////////////
COMMAND(TCPClientReceive,"on receive TCP client, data=... ptr=&(TCPClient)",EVENT_NETWORK)
{
    std::string data = args.GetValue("data","");
    void* ptr = args.GetValue("ptr", (TCPClient*)nullptr);
    if (g_theServer->m_isAuthoritative || data.empty()) {
        return false;
    }

    TCPData tcpData(data.size(), &data[0]);
    eNetworkPackageHeaderType type = tcpData.GetHeader();
    switch (type)
    {
    case eNetworkPackageHeaderType::HEAD_SERVER_LISTEN:    {
        return ParseClientConnectPackage(tcpData.GetContent());
    }
    case eNetworkPackageHeaderType::HEAD_CLIENT_CLOSE:    {
        ((RemoteServer*)g_theServer)->RemoveTCPClient((TCPClient*)ptr);
        return true;
    }
    default:{
        return false;
    }
    }
}

//////////////////////////////////////////////////////////////////////////
COMMAND(TCPServerFailOneClient, "server receive client tcp fail, data=... ptr=&(TCPSocket)", EVENT_NETWORK)
{
    std::string data = args.GetValue("data", "");
    void* ptr = args.GetValue("ptr", (TCPSocket*)nullptr);
    if (!g_theServer->m_isAuthoritative || data.empty()) {
        return false;
    }

    TCPData tcpData(data.size(), &data[0]);
    eNetworkPackageHeaderType type = tcpData.GetHeader();
    switch (type)
    {
    case eNetworkPackageHeaderType::HEAD_CLIENT_START:    {
        return ParseClientStartMessage(tcpData.GetContent(), (TCPSocket*)ptr);
    }
    case eNetworkPackageHeaderType::HEAD_CLIENT_CLOSE:    {
        ((AuthoritativeServer*)g_theServer)->RemoveTCPClient((TCPSocket*)ptr);
        return true;
    }
    default:    {
        return false;
    }
    }
}

//////////////////////////////////////////////////////////////////////////
void Server::Startup(eGameType gameType)
{
    switch (gameType) {
    case GAME_SINGLE_PLAYER:
    {
        m_theGame = static_cast<Game*>(new SingleplayerGame());
        break;
    }
    case GAME_MULTI_PLAYER:
    {
        m_theGame = static_cast<Game*>(new MultiplayerGame());
        break;
    }
    }

    m_theGame->StartUp();
}

//////////////////////////////////////////////////////////////////////////
void Server::BeginFrame()
{
    if (m_clients.empty()) {
        g_theApp->HandleQuitRequisted();
    }
}

//////////////////////////////////////////////////////////////////////////
void Server::Update()
{
    m_theGame->UpdateLocal();

    for (size_t i=0;i<m_clients.size();) {
        Client* c = m_clients[i];
        if (c->m_isQuiting) {
            delete c;
            m_clients.erase(m_clients.begin()+i);
        }
        else {
            if (c->CouldUpdateInput()) {
                ReceiveInputInfo(c);
            }
            if (!g_theApp->IsQuiting()) {
                c->Update();
            }
            i++;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void Server::EndFrame()
{
    for (Client* c : m_clients) {
        c->EndFrame();
    }
}

//////////////////////////////////////////////////////////////////////////
void Server::SendReliableMessages()
{
    for (Client* c : m_clients) {
        c->SendReliableMessages();
    }
}

//////////////////////////////////////////////////////////////////////////
void Server::RemovePlayer(Client* client)
{
    m_theGame->RemoveEntity(client->m_playerPawn);
    client->Shutdown();
}

//////////////////////////////////////////////////////////////////////////
void Server::RemovePlayerOfIdentifier(int identifier)
{
    for (Client* c : m_clients) {
        if (identifier == c->m_identifier && c->m_isRemote) {   //TODO server player not delete
            RemovePlayer(c);
            return;
        }
    }
}
