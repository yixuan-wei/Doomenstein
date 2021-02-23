#include "Game/AuthoritativeServer.hpp"
#include "Game/PlayerClient.hpp"
#include "Game/SingleplayerGame.hpp"
#include "Game/MultiplayerGame.hpp"
#include "Game/NetworkMessage.hpp"
#include "Game/EntityDefinition.hpp"
#include "Game/App.hpp"
#include "Game/Entity.hpp"
#include "Game/Actor.hpp"
#include "Game/Projectile.hpp"
#include "Game/RemoteClient.hpp"
#include "Game/NetworkObserver.hpp"
#include "Engine/Network/Network.hpp"
#include "Engine/Network/TCPServer.hpp"
#include "Engine/Network/UDPSocket.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/MathUtils.hpp"


//////////////////////////////////////////////////////////////////////////
static Vec3 GetLocalPosOnZUpCylinderAlongDirection(float height, float radius, Vec3 const& direction, float startHeight)
{
    float xyLength = std::sqrtf(direction.x * direction.x + direction.y * direction.y);
    float fraction = xyLength / direction.z;
    if (fraction < radius / (height-startHeight) && fraction>0.f) {	//point on top
        float frac = (height-startHeight) / direction.z;
        return direction * frac;
    }
    else if (fraction > radius / (-startHeight) &&fraction<0.f) {//point on bottom
        float frac = -startHeight/direction.z;
        return direction*frac;
    }
    else {	//point on side
        float frac = radius / xyLength;
        return direction * frac;
    }
}

//////////////////////////////////////////////////////////////////////////
AuthoritativeServer::AuthoritativeServer()
{
    m_isAuthoritative = true;
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::SetUpTCPServer(int port)
{
    TCPServer* server = g_theNetwork->CreateTCPServer((uint16_t)port, NET_NON_BLOCKING);
    server->m_onNewConnectAccept = MakeCustomClientConnectPackage;
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::BeginFrame()
{
    for (Client* c : m_clients) {
        c->BeginFrame();
    }

    Server::BeginFrame();
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::Update()
{
    m_theGame->Update();
    Server::Update();
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::Render() const
{    
    for (Client* c : m_clients) {
        if(!c->m_isRemote){
            m_theGame->UpdateCameraForPawn(c->m_playerPawn);
            c->Render();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::Shutdown()
{
    for (Client* c: m_clients) {
        if(c&& !c->m_isQuiting){
            RemovePlayer(c);
        }
    }

    for (Client* c : m_clients) {
        if (c) {
            delete c;
        }
    }
    m_theGame->ShutDown();

    delete m_theGame;
    m_theGame = nullptr;

    //TODO delete client here
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::UpdateTCPUDPReference(TCPSocket* client, int toPort, int bindPort)
{
    UNUSED(toPort);

    for (Client* c : m_clients) {
        if (!c->m_isRemote || c->m_udpSocket == nullptr) {
            continue;
        }

        UDPSocket* soc = c->m_udpSocket;
        if (soc->GetBindPort() == bindPort) {
            c->m_tcpClientSocket = client;
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::CreateUDPSocket(std::string const& ip, int toPort, int bindPort, int identifier)
{
    RemoteClient* newClient = new RemoteClient();
    m_clients.push_back(newClient);
    newClient->m_udpSocket = g_theNetwork->CreateUDPSocket(ip, toPort, bindPort);
    newClient->m_identifier = identifier;
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::SendOneMessage(std::string const& msg)
{
    for (Client* c : m_clients) {
        if (c->m_isRemote && c->m_udpSocket && c->m_udpSocket->IsValid()) {
            std::string customMsg = msg;
            NetworkPackageHeader* headerPtr = reinterpret_cast<NetworkPackageHeader*>(&customMsg[0]);
            headerPtr->m_key = c->m_identifier;
            c->m_udpSocket->SendUDPMessage(customMsg);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::HandleUDPMessageOfIdentifier(NetMessageHeader const& header, std::string const& content, int identifier, bool reliable)
{
    //for individual messages
    for (Client* c : m_clients) {
        if (c->m_identifier == identifier) {
            switch (header.m_type) {
            case MESSAGE_ADD_PLAYER:
            {
                Entity* e = g_theGame->GetEntityOfIndex(identifier);
                if(e==nullptr || e->GetEntityDefinition()->m_name!="Marine"){
                    if (e) {
                        DeleteEntity(e);
                    }
                    e = g_theGame->SpawnEntityAtPlayerStart(m_clients.size());
                    e->SetIndex(identifier);
                }
                c->Startup(e,this);
                break;
            }
            case MESSAGE_PLAYER_INPUT:
            {
                InputInfo input;
                if (ParsePlayerInputMessage(content, input)) {
                    c->m_input = input;
                }
                break;
            }
            case MESSAGE_ACK:
            {
                c->ReceiveACKMessage(content);
                break;
            }
            }

            //reliable send immediately
            if (reliable && c->m_udpSocket && c->m_udpSocket->IsValid()) {
                std::string ack = MakeReliableACKMessage(header.m_seqNo);
                std::string pack = MakeTextPackage(ack);
                NetworkPackageHeader* ptr = reinterpret_cast<NetworkPackageHeader*>(&pack[0]);
                ptr->m_key = c->m_identifier;
                c->m_udpSocket->SendUDPMessage(pack);
            }
        }
    }

    //for all clients messages
    eNetMessageHeaderType type = (eNetMessageHeaderType)header.m_type;
    if(type==MESSAGE_SOUND_PLAY)    {
        size_t id = StringConvert(content.c_str(), 0);
        for (Client* c : m_clients) {
            c->PlaySoundOnClient(id);
        }
    }
    else if (type == MESSAGE_ENTITY_TELEPORT) {
        ParseEntityTeleportMessage(content);
    }
    else if (type == MESSAGE_ADD_PLAYER) {
        for (Client* cl : m_clients) {
            if (cl->m_playerPawn) {
                AddEntity(cl->m_playerPawn);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::AddPlayer(Client* newClient)
{
    Entity* entity = m_theGame->SpawnEntityAtPlayerStart(m_clients.size());
    m_clients.push_back(newClient);
    newClient->Startup(entity, this);
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::RemovePlayer(Client* client)
{
    if(client->m_playerPawn){
        AddRemoveMessageToClients(client->m_playerPawn->GetIndex());
    }

    Server::RemovePlayer(client);
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::RemoveTCPClient(TCPSocket* clientSoc)
{
    for (Client* c : m_clients) {
        if (c->m_isRemote && c->m_tcpClientSocket == clientSoc) {
            RemovePlayer(c);
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::SwitchPlayerMap(Client* client, std::string const& mapName)
{
    m_theGame->SwitchMapForEntity(mapName, client->m_playerPawn);
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::AddEntity(Entity* entity)
{
    std::string msg = MakeEntityCreateMessage(entity);
    g_theObserver->AddEntityTransformUpdate(entity);
    std::string health;
    bool shouldHealth = false;
    if(entity->GetEntityType()==ENTITY_ACTOR){
        Actor* a = (Actor*)entity;
        shouldHealth = true;
        health = MakeActorHealthMessage(entity->GetIndex(), a->GetHealth());
    }
    for (Client* c : m_clients) {
        if (c->m_isRemote) {
            c->InsertCreateMsg(msg);
            if(shouldHealth){
                c->InsertHealthMsg(health);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::DeleteEntity(Entity* entity)
{
    if (g_theServer->IsPawnAPlayer(entity)) {
        g_theServer->RemovePlayerOfIdentifier(entity->GetIndex());
    }
    else {
        AddRemoveMessageToClients(entity->GetIndex());
        g_theGame->RemoveEntity(entity);
    }
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::ReceiveInputInfo(Client* client)
{
    InputInfo& info = client->m_input;
    Entity* pawn = client->m_playerPawn;
    m_theGame->UpdatePlayerForInput(pawn, info);

    if (info.isFiring && 
        (client->m_isRemote || ((Actor*)client->m_playerPawn)->GetHealth()>0.f)) {
        Projectile* bullet = (Projectile*)g_theGame->SpawnEntityAtPlayerStart(0,"Bullet");
        Vec3 onPawnPos = GetLocalPosOnZUpCylinderAlongDirection(pawn->GetEntityHeight(), 
            pawn->GetEntityRadius(), pawn->GetEntityForward(), pawn->GetEntityDefinition()->m_eyeHeight-.1f);
        if (bullet->GetMap() != pawn->GetMap()) {
            g_theGame->SwitchMapForEntity(pawn->GetMap()->GetName(),bullet);
        }
        float dist = bullet->GetEntityRadius() * 2.f + .04f; 
        onPawnPos = onPawnPos*1.1f + onPawnPos.GetNormalized()*dist;
        Vec3 firePoint = pawn->GetEntityEyePosition()+Vec3(.0f,.0f,-.1f) + onPawnPos; 
        bullet->SetPosition(Vec2(firePoint.x,firePoint.y));
        bullet->SetPitchYawRollDegrees(pawn->GetEntityPitchYawRollDegrees());
        bullet->SetHeight(firePoint.z);
        AddEntity((Entity*)bullet);
    }

    if (info.isClosing && !client->m_isRemote) {
        g_theApp->HandleQuitRequisted();
    }
    
    if (info.isClosing) {
        RemovePlayer(client);
    }    

    client->m_input.isFiring=false;
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::PlayGlobalSound(SoundID id)
{
    for (Client* c : m_clients) {
        c->PlaySoundOnClient(id);
    }
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::AddRemoveMessageToClients(int entityIdxToRemove)
{
    std::string deleteMsg = MakeEntityDeleteMessage(entityIdxToRemove);
    for (Client* c : m_clients) {
        if (!c->m_isRemote || c->m_playerPawn==nullptr) {
            continue;
        }

        RemoteClient* remoClient = (RemoteClient*)c;
        remoClient->InsertDeleteMsg(deleteMsg);
    }
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::AddTeleportMessageToClients(Entity* entityToTeleport)
{
    std::string teleportMsg = MakeEntityTeleportMessage(entityToTeleport);
    for (Client* c : m_clients) {
        if (!c->m_isRemote || c->m_playerPawn == nullptr) {
            continue;
        }

        RemoteClient* remoClient = (RemoteClient*)c;
        remoClient->InsertTeleportMsg(teleportMsg);
    }
}

//////////////////////////////////////////////////////////////////////////
void AuthoritativeServer::AddHealthMessageToClients(Actor* actorToUpdateHealth)
{
    std::string healthMsg = MakeActorHealthMessage(actorToUpdateHealth->GetIndex(), actorToUpdateHealth->GetHealth());
    for (Client* c : m_clients) {
        if (!c->m_isRemote || c->m_playerPawn == nullptr) {
            continue;
        }

        RemoteClient* remoClient = (RemoteClient*)c;
        remoClient->InsertHealthMsg(healthMsg);
    }
}

//////////////////////////////////////////////////////////////////////////
Client* AuthoritativeServer::GetClientOfUDPSocket(UDPSocket* soc) const
{
    for (Client* c : m_clients) {
        if (c->m_udpSocket == soc) {
            return c;
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
Client* AuthoritativeServer::GetClientOfPawnIndex(int idx) const
{
    for (Client* c : m_clients) {
        if (c->m_playerPawn && c->m_playerPawn->GetIndex() == idx) {
            return c;
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
bool AuthoritativeServer::IsPawnAPlayer(Entity* pawn) const
{
    for (Client* c : m_clients) {
        if (c->m_playerPawn == pawn) {
            return true;
        }
    }

    return false;
}
