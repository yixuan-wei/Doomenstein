#include "Game/RemoteServer.hpp"
#include "Game/Client.hpp"
#include "Game/Entity.hpp"
#include "Game/SingleplayerGame.hpp"
#include "Game/MultiplayerGame.hpp"
#include "Game/NetworkMessage.hpp"
#include "Game/NetworkObserver.hpp"
#include "Engine/Network/Network.hpp"
#include "Engine/Network/TCPClient.hpp"
#include "Engine/Network/UDPSocket.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/StringUtils.hpp"

//////////////////////////////////////////////////////////////////////////
RemoteServer::RemoteServer()
{
    m_isAuthoritative = false;
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::SetUpTCPClient(std::string const& serverIP, int port)
{
    m_serverIP = serverIP;
    m_tcpClient = g_theNetwork->CreateTCPClient(serverIP, (unsigned short)port, NET_NON_BLOCKING);
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::BeginFrame()
{
    Server::BeginFrame();

    Client* c = m_clients[0];
    if(c->m_udpSocket){
        c->BeginFrame();
    }
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::Render() const
{
    Client* c = m_clients[0];
    if(c->m_playerPawn){
        m_theGame->UpdateCameraForPawn(c->m_playerPawn);
        c->Render();
    }
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::Shutdown()
{
    if(!m_clients.empty()){
        Client* c = m_clients[0];
        RemovePlayer(c);
        delete c;
        m_clients.clear();
    }

    m_theGame->ShutDown();

    delete m_theGame;
    m_theGame = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::CreateUDPSocket(std::string const& ip, int toPort, int bindPort, int identifier)
{
    Client* c = m_clients[0];
    c->m_udpSocket = g_theNetwork->CreateUDPSocket(ip, toPort, bindPort);
    c->m_identifier = identifier;

    std::string str = MakeClientStartMessage(toPort, bindPort);
    m_tcpClient->SendTCPMessage(str);

    RequestAddPlayer();
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::SendReliableMessages()
{
    Server::SendReliableMessages();
    m_clients[0]->m_shot = false;
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::SendOneMessage(std::string const& msg)
{
    Client* c = m_clients[0];
    if (c->m_udpSocket==nullptr || !c->m_udpSocket->IsValid()) {
        return;
    }

    std::string customMsg = msg;
    NetworkPackageHeader* headerPtr = reinterpret_cast<NetworkPackageHeader*>(&customMsg[0]);
    headerPtr->m_key = c->m_identifier;
    c->m_udpSocket->SendUDPMessage(customMsg);
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::HandleUDPMessageOfIdentifier(NetMessageHeader const& header, std::string const& content, int identifier, bool reliable)
{
    Client* c = m_clients[0];
    if (identifier != c->m_identifier) {
        return;
    }

    switch (header.m_type) {
    case MESSAGE_ENTITY_CREATE:    {
        HandleEntityCreateMessage(content);
        break;
    }
    case MESSAGE_ENTITY_TRANSFORM:    {
        ParseEntityTransformMessage(content);
        break;
    }
    case MESSAGE_ENTITY_TELEPORT:    {
        ParseEntityTeleportMessage(content);
        break;
    }
    case MESSAGE_ENTITY_DELETE:    {
        ParseEntityDeleteMessage(content);
        break;
    }
    case MESSAGE_ACTOR_HEALTH:    {
        ParseActorHealthMessage(content);
        break;
    }
    case MESSAGE_SOUND_PLAY:    {
        size_t id = StringConvert(content.c_str(), 0ull);
        if (!m_clients.empty()) {
            m_clients[0]->PlaySoundOnClient(id);
        }
        break;
    }
    case MESSAGE_ACK:    {
        c->ReceiveACKMessage(content);
        break;
    }
    }

    if (reliable) {
        std::string ack = MakeReliableACKMessage(header.m_seqNo);
        g_theObserver->AddMessage(ack);
    }
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::HandleEntityCreateMessage(std::string const& content)
{
    Entity* newEntity = ParseEntityCreateMessage(content);
    Client* c = m_clients[0];
    //TODO refine pawn assign logic
    if (newEntity != nullptr && c->m_playerPawn == nullptr && 
        c->m_identifier == newEntity->GetIndex()) {
        c->Startup(newEntity, this);
    }
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::RequestAddPlayer()
{
    std::string content = MakeMessageHeader(MESSAGE_ADD_PLAYER);
    std::string pack = MakeTextPackage(content);
    SendOneMessage(pack);
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::AddPlayer(Client* newClient)
{
    m_clients.push_back(newClient);
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::RemoveTCPClient(TCPClient* client)
{
    UNUSED(client);

    if (!m_clients.empty()) {
        RemovePlayer(m_clients[0]);
    }
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::SwitchPlayerMap(Client* client, std::string const& mapName)
{
    if (client->m_playerPawn == nullptr) {
        return;
    }

    std::string msg = MakeEntityTeleportMessage(client->m_playerPawn->GetIndex(),mapName);
    g_theObserver->AddMessage(msg);
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::ActualSwitchPlayerMap(Entity* e, std::string const& mapName)
{
    m_theGame->SwitchMapForEntity(mapName,e);
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::ReceiveInputInfo(Client* client)
{
    client->m_input.isFiring = client->m_shot;
    std::string str = MakePlayerInputMessage(client->m_input);
    g_theObserver->AddMessage(str);
}

//////////////////////////////////////////////////////////////////////////
void RemoteServer::PlayGlobalSound(SoundID id)
{
    g_theObserver->AddSoundPlay(id);
}

//////////////////////////////////////////////////////////////////////////
Client* RemoteServer::GetClientOfUDPSocket(UDPSocket* soc) const
{
    if (m_clients.empty()) {
        return nullptr;
    }

    Client* c = m_clients[0];
    if (c->m_udpSocket == soc) {
        return c;
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
Client* RemoteServer::GetClientOfPawnIndex(int idx) const
{
    if (m_clients.empty()) {
        return nullptr;
    }

    Client* c = m_clients[0];
    if (c->m_playerPawn->GetIndex() == idx) {
        return c;
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
bool RemoteServer::IsPawnAPlayer(Entity* pawn) const
{
    if (m_clients.empty()) {
        return false;
    }

    Entity* player = m_clients[0]->m_playerPawn;
    return player == pawn;
}
