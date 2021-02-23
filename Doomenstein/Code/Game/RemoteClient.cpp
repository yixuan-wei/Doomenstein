#include "Game/RemoteClient.hpp"
#include "Game/Server.hpp"
#include "Game/NetworkMessage.hpp"
#include "Game/NetworkObserver.hpp"
#include "Engine/Network/UDPSocket.hpp"

//////////////////////////////////////////////////////////////////////////
RemoteClient::RemoteClient()
{
    m_isRemote = true;
}

//////////////////////////////////////////////////////////////////////////
void RemoteClient::Startup(Entity* playerPawn, Server* server)
{
    m_playerPawn = playerPawn;
    m_server = server;
}

//////////////////////////////////////////////////////////////////////////
void RemoteClient::Shutdown()
{
    if(m_udpSocket && m_udpSocket->IsValid()){
        m_udpSocket->SendUDPMessage(MakeQuitPackage());
    }

    Client::Shutdown();
}

//////////////////////////////////////////////////////////////////////////
bool RemoteClient::PlaySoundOnClient(size_t id)
{
    g_theObserver->AddSoundPlay(id);
    return true;
}
