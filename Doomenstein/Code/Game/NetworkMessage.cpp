#include "Game/NetworkMessage.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Entity.hpp"
#include "Game/Actor.hpp"
#include "Game/EntityDefinition.hpp"
#include "Game/Game.hpp"
#include "Game/Projectile.hpp"
#include "Game/Server.hpp"
#include "Game/RemoteServer.hpp"
#include "Game/AuthoritativeServer.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Network/NetworkCommon.hpp"

#include <set>

static unsigned short sSequenceNo = 0;
static std::set<int> sUsedPorts;

//////////////////////////////////////////////////////////////////////////
int RollRandomUnusedPort()
{
    int bindPort = g_theRNG->RollRandomIntInRange(48001, 49000);
    while (sUsedPorts.find(bindPort) != sUsedPorts.end()) {
        bindPort = g_theRNG->RollRandomIntInRange(48001, 49000);
    }
    sUsedPorts.insert(bindPort);
    return bindPort;
}

//////////////////////////////////////////////////////////////////////////
eNetMessageHeaderType GetHeaderTypeForMessage(std::string const& msg)
{
    NetMessageHeader const* head = reinterpret_cast<NetMessageHeader const*>(&msg[0]);
    eNetMessageHeaderType type = (eNetMessageHeaderType)head->m_type;
    return type;
}

//////////////////////////////////////////////////////////////////////////
unsigned short GetSeqNoForMessage(std::string const& msg)
{
    NetMessageHeader const* head = reinterpret_cast<NetMessageHeader const*>(&msg[0]);
    return head->m_seqNo;
}

//////////////////////////////////////////////////////////////////////////
int GetEntityIdxFromOneTrunkMessage(std::string const& msg)
{
    std::string content(&msg[MESSAGE_HEADER_LEN]);
    return StringConvert(content.c_str(),-1);
}

//////////////////////////////////////////////////////////////////////////
int GetEntityIdxFromTwoChunksMessage(std::string const& msg)
{
    std::string content(&msg[MESSAGE_HEADER_LEN]);
    Strings trunks = SplitStringOnDelimiter(content,';');
    if (trunks.size() != 2) {
        return -1;
    }

    return StringConvert(trunks[0].c_str(), -1);
}

//////////////////////////////////////////////////////////////////////////
void GetEntityCreateInfoFromMessage(std::string const& msg, int& idx, std::string& type)
{
    std::string content(&msg[MESSAGE_HEADER_LEN]);
    Strings trunks = SplitStringOnDelimiter(content,';');
    if (trunks.size() != 4) {
        return;
    }

    idx = StringConvert(trunks[0].c_str(), -1);
    type = trunks[1];
}

//////////////////////////////////////////////////////////////////////////
std::string MakeMessageHeader(eNetMessageHeaderType type)
{
    char header[MESSAGE_HEADER_LEN];
    NetMessageHeader* headerPtr = reinterpret_cast<NetMessageHeader*>(&header[0]);
    headerPtr->m_type = type;
    headerPtr->m_size = 0;
    headerPtr->m_seqNo = sSequenceNo++;
    return std::string(header, MESSAGE_HEADER_LEN);
}

//////////////////////////////////////////////////////////////////////////
std::string MakeCustomClientConnectPackage(std::string const& clientIP)
{
    std::string packHeader= MakeClientConnectPackageHeader();
    NetworkPackageHeader* pHeaderPtr = reinterpret_cast<NetworkPackageHeader*>(&packHeader[0]);
    int bindPort =RollRandomUnusedPort();
    int toPort = RollRandomUnusedPort();
    int identifier = g_theRNG->RollRandomIntInRange(INT_MIN+1, INT_MAX-1);
    while (identifier >= 0 && identifier < 100000) {
        identifier = g_theRNG->RollRandomIntInRange(INT_MIN + 1, INT_MAX - 1);
    }
    //TODO not handled same identifier
    g_theServer->CreateUDPSocket(clientIP,bindPort, toPort, identifier);
    std::string content = Stringf("%i;%i;%i", toPort,bindPort,identifier);
    pHeaderPtr->m_size = (uint16_t)content.size();
    return packHeader+content;
}

//////////////////////////////////////////////////////////////////////////
std::string MakePlayerInputMessage(InputInfo const& input)
{
    std::string content = Stringf("%f,%f;%f,%f;%i;%i",input.playerMove.x,input.playerMove.y,
        input.mouseMove.x,input.mouseMove.y,(int)input.isClosing, (int)input.isFiring);
    
    char header[MESSAGE_HEADER_LEN];
    NetMessageHeader* headerPtr = reinterpret_cast<NetMessageHeader*>(&header[0]);
    headerPtr->m_type = eNetMessageHeaderType::MESSAGE_PLAYER_INPUT;
    headerPtr->m_size = (unsigned short)content.size();
    headerPtr->m_seqNo = sSequenceNo++;

    std::string package = std::string(header,MESSAGE_HEADER_LEN);
    return package+content;
}

//////////////////////////////////////////////////////////////////////////
std::string MakeEntityTransformMessage(Entity const* entity)
{
    Vec2 pos = entity->GetEntityPosition2D();
    Vec3 pitchYawRoll = entity->GetEntityPitchYawRollDegrees();
    float height=0.f;
    if (entity->GetEntityType() == ENTITY_PROJECTILE) {
        height = ((Projectile*)entity)->GetFlyHeight();
    }
    std::string content = Stringf("%i;%f,%f,%f;%f;%f", entity->GetIndex(),
        pos.x, pos.y, height, pitchYawRoll.x, pitchYawRoll.y);

    char header[MESSAGE_HEADER_LEN];
    NetMessageHeader* headerPtr = reinterpret_cast<NetMessageHeader*>(&header[0]);
    headerPtr->m_type = eNetMessageHeaderType::MESSAGE_ENTITY_TRANSFORM;
    headerPtr->m_size=(unsigned short)content.size();
    headerPtr->m_seqNo = sSequenceNo++;

    std::string msgHeader = std::string(header, MESSAGE_HEADER_LEN);
    return msgHeader+content;
}

//////////////////////////////////////////////////////////////////////////
std::string MakeEntityCreateMessage(Entity const* entity)
{
    std::string type = entity->GetEntityDefinition()->m_name;
    Vec2 pos = entity->GetEntityPosition2D();
    Vec3 pitchYawRoll = entity->GetEntityPitchYawRollDegrees();
    float damage = 0.f;
    if (entity->GetEntityType() == ENTITY_PROJECTILE) {
        damage = ((Projectile*)entity)->GetDamage();
    }
    std::string content = Stringf("%i;%s;%s;%f",entity->GetIndex(), type.c_str(),
        entity->GetMap()->GetName().c_str(), damage);

    char header[MESSAGE_HEADER_LEN];
    NetMessageHeader* headerPtr = reinterpret_cast<NetMessageHeader*>(&header[0]);
    headerPtr->m_type = eNetMessageHeaderType::MESSAGE_ENTITY_CREATE;
    headerPtr->m_size = (unsigned short)content.size();
    headerPtr->m_seqNo = sSequenceNo++;

    std::string msgHeader = std::string(header, MESSAGE_HEADER_LEN);
    return msgHeader+content;
}

//////////////////////////////////////////////////////////////////////////
std::string MakeEntityTeleportMessage(Entity const* entity)
{
    return MakeEntityTeleportMessage(entity->GetIndex(), entity->GetMap()->GetName().c_str());
}

//////////////////////////////////////////////////////////////////////////
std::string MakeEntityTeleportMessage(int entityIdx, std::string const& mapName)
{
    std::string content = Stringf("%i;%s", entityIdx, mapName.c_str());

    char header[MESSAGE_HEADER_LEN];
    NetMessageHeader* headerPtr = reinterpret_cast<NetMessageHeader*>(&header[0]);
    headerPtr->m_type = eNetMessageHeaderType::MESSAGE_ENTITY_TELEPORT;
    headerPtr->m_size = (unsigned short)content.size();
    headerPtr->m_seqNo = sSequenceNo++;

    return std::string(header, MESSAGE_HEADER_LEN) + content;
}

//////////////////////////////////////////////////////////////////////////
std::string MakeEntityDeleteMessage(int entityIdx)
{
    std::string content = Stringf("%i",entityIdx);

    char header[MESSAGE_HEADER_LEN];
    NetMessageHeader* headerPtr = reinterpret_cast<NetMessageHeader*>(&header[0]);
    headerPtr->m_type = eNetMessageHeaderType::MESSAGE_ENTITY_DELETE;
    headerPtr->m_size = (unsigned short)content.size();
    headerPtr->m_seqNo = sSequenceNo++;

    return std::string(header, MESSAGE_HEADER_LEN) + content;
}

//////////////////////////////////////////////////////////////////////////
std::string MakeSoundPlayMessage(size_t id)
{
    std::string content = Stringf("%llu", id);

    char header[MESSAGE_HEADER_LEN];
    NetMessageHeader* headerPtr = reinterpret_cast<NetMessageHeader*>(&header[0]);
    headerPtr->m_type = eNetMessageHeaderType::MESSAGE_SOUND_PLAY;
    headerPtr->m_size = (unsigned short)content.size();
    headerPtr->m_seqNo = sSequenceNo++;

    return std::string(header, MESSAGE_HEADER_LEN) + content;
}

//////////////////////////////////////////////////////////////////////////
std::string MakeActorHealthMessage(int entityIdx, float newHealth)
{
    std::string content = Stringf("%i;%f", entityIdx, newHealth);

    char header[MESSAGE_HEADER_LEN];
    NetMessageHeader* headerPtr = reinterpret_cast<NetMessageHeader*>(&header[0]);
    headerPtr->m_type = eNetMessageHeaderType::MESSAGE_ACTOR_HEALTH;
    headerPtr->m_size = (unsigned short)content.size();
    headerPtr->m_seqNo = sSequenceNo++;

    return std::string(header, MESSAGE_HEADER_LEN) + content;
}

//////////////////////////////////////////////////////////////////////////
std::string MakeReliableACKMessage(unsigned short msgSeqNo)
{
    std::string content = Stringf("%u", msgSeqNo);

    char header[MESSAGE_HEADER_LEN];
    NetMessageHeader* headerPtr = reinterpret_cast<NetMessageHeader*>(&header[0]);
    headerPtr->m_type = eNetMessageHeaderType::MESSAGE_ACK;
    headerPtr->m_size = (unsigned short)content.size();
    headerPtr->m_seqNo = sSequenceNo++;

    return std::string(header, MESSAGE_HEADER_LEN) + content;
}

//////////////////////////////////////////////////////////////////////////
std::string MakeClientStartMessage(int udpToPort, int udpBindPort)
{
    std::string packHeader = MakeClientStartPackageHeader();
    NetworkPackageHeader* pHeaderPtr = reinterpret_cast<NetworkPackageHeader*>(&packHeader[0]);
    std::string content = Stringf("%i;%i", udpBindPort, udpToPort);
    pHeaderPtr->m_size = (uint16_t)content.size();
    return packHeader + content;
}

//////////////////////////////////////////////////////////////////////////
bool ParseActorHealthMessage(std::string const& content)
{
    Strings trunks = SplitStringOnDelimiter(content, ';');

    int idx = StringConvert(trunks[0].c_str(), -1);
    Entity* entity = nullptr;
    entity = g_theGame->GetEntityOfIndex(idx);
    if (entity == nullptr || entity->GetEntityType() != ENTITY_ACTOR) {
        g_theConsole->PrintError(Stringf("Fail to find actor health idx %i", idx));
        return false;
    }

    float newHealth = StringConvert(trunks[1].c_str(), 0.f);
    ((Actor*)entity)->SetHealth(newHealth);
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool ParseEntityDeleteMessage(std::string const& content)
{
    int idx = StringConvert(content.c_str(), -1);
    Entity* entity = nullptr;
    entity = g_theGame->GetEntityOfIndex(idx);
    if (entity == nullptr) {
        g_theConsole->PrintString(Rgba8::YELLOW, Stringf("Fail to find entity delete %i", idx));
        return false;
    }

    g_theGame->RemoveEntity(entity);
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool ParseEntityTeleportMessage(std::string const& content)
{
    Strings trunks = SplitStringOnDelimiter(content, ';');
    if (trunks.size() != 2) {
        g_theConsole->PrintError(Stringf("Fail to parse entity teleport %s", content.c_str()));
        return false;
    }

    int idx = StringConvert(trunks[0].c_str(),-1);
    Entity* entity = nullptr;
    entity = g_theGame->GetEntityOfIndex(idx);
    if (entity == nullptr) {
        g_theConsole->PrintError(Stringf("Fail to find entity idx %i in teleport", idx));
        return false;
    }

    if(g_theServer->m_isAuthoritative){
        Client* c = g_theServer->GetClientOfPawnIndex(idx);
        if (c == nullptr) {
            g_theConsole->PrintError(Stringf("Fail to find client of pawn idx %i", idx));
            return false;
        }
        g_theServer->SwitchPlayerMap(c, trunks[1]);
    }
    else {
        RemoteServer* remoServer = static_cast<RemoteServer*>(g_theServer);
        remoServer->ActualSwitchPlayerMap(entity, trunks[1]);
    }
    
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool ParseClientConnectPackage(std::string const& content)
{
    Strings trunks = SplitStringOnDelimiter(content,';');
    if (trunks.size() != 3) {
        g_theConsole->PrintError(Stringf("Fail to parse client connect %s", content.c_str()));
        return false;
    }

    if(g_theServer->m_isAuthoritative){
        g_theConsole->PrintError("Fail to receive client connect package on server");
        return false;
    }

    int toPort = StringConvert(trunks[0].c_str(),48000);
    int bindPort = StringConvert(trunks[1].c_str(),48000);
    int identifier = StringConvert(trunks[2].c_str(),0xffffffff);
    RemoteServer* remoServer = static_cast<RemoteServer*>(g_theServer);
    g_theServer->CreateUDPSocket(remoServer->m_serverIP, toPort,bindPort,identifier);
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool ParseClientStartMessage(std::string const& content, TCPSocket* client)
{
    Strings trunks = SplitStringOnDelimiter(content, ';');
    if (trunks.size() != 2) {
        g_theConsole->PrintError(Stringf("Fail to parse client start %s", content.c_str()));
        return false;
    }

    if (!g_theServer->m_isAuthoritative) {
        g_theConsole->PrintError("Fail to receive client start package on remote server");
        return false;
    }

    int toPort = StringConvert(trunks[0].c_str(), 0);
    int bindPort = StringConvert(trunks[1].c_str(), 0);
    AuthoritativeServer* server = (AuthoritativeServer*)g_theServer;
    server->UpdateTCPUDPReference(client, toPort, bindPort);
    return true;
}

//////////////////////////////////////////////////////////////////////////
Entity* ParseEntityCreateMessage(std::string const& content)
{
    Strings trunks = SplitStringOnDelimiter(content,';');
    if (trunks.size() != 4) {
        g_theConsole->PrintError(Stringf("Fail to parse entity create %s", content.c_str()));
        return nullptr;
    }

    int idx = StringConvert(trunks[0].c_str(),-1);
    Entity* entity = nullptr;    
    entity = g_theGame->GetEntityOfIndex(idx);
    if(entity){
        std::string entityTypeName = entity->GetEntityDefinition()->m_name;
        if (entityTypeName != trunks[1]) {
            g_theConsole->PrintError(Stringf("Deleting existing entity of type %s not same as create type %s",
                entityTypeName.c_str(), trunks[1].c_str()));
            g_theGame->RemoveEntity(entity);
            entity = nullptr;
        }
        else {
            return nullptr;
        }
    }
    if (entity == nullptr) {
        entity = g_theGame->SpawnEntityAtPlayerStart(0, trunks[1]);
        entity->SetIndex(idx);
    }

    //switch map
    std::string mapName = trunks[2];
    g_theGame->SwitchMapForEntity(mapName, entity);

    float damage = 0.f;
    if (entity->GetEntityType() == ENTITY_PROJECTILE) {
        damage = StringConvert(trunks[3].c_str(), 0.f);
        ((Projectile*)entity)->SetDamage(damage);
    }

    return entity;
}

//////////////////////////////////////////////////////////////////////////
bool ParseEntityTransformMessage(std::string const& content)
{
    Strings trunks = SplitStringOnDelimiter(content,';');
    if (trunks.size() != 4) {
        g_theConsole->PrintError(Stringf("Fail to parse entity transform content %s", content.c_str()));
        return false;
    }

    int idx = StringConvert(trunks[0].c_str(),-1);
    Entity* entity = g_theGame->GetEntityOfIndex(idx);
    if (entity == nullptr) {
        g_theConsole->PrintError(Stringf("Fail to get entity of idx %i",idx));
        return false;
    }

    Strings rawPos = SplitStringOnDelimiter(trunks[1],',');
    if (rawPos.size() != 3) {
        g_theConsole->PrintError(Stringf("Fail to parse entity pos %s", trunks[1].c_str()));
    }
    else
    {
        Vec2 newPos = entity->GetEntityPosition2D();
        newPos.x = StringConvert(rawPos[0].c_str(),newPos.x);
        newPos.y = StringConvert(rawPos[1].c_str(),newPos.y);
        entity->SetPosition(newPos);

        float height = StringConvert(rawPos[2].c_str(), 0.f);
        if (entity->GetEntityType() == ENTITY_PROJECTILE) {
            ((Projectile*)entity)->SetHeight(height);
        }
    }

    Vec3 pitchYawRoll = entity->GetEntityPitchYawRollDegrees();
    pitchYawRoll.x = StringConvert(trunks[2].c_str(),pitchYawRoll.x);
    pitchYawRoll.y = StringConvert(trunks[3].c_str(),pitchYawRoll.y);
    entity->SetPitchYawRollDegrees(pitchYawRoll);
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool ParsePlayerInputMessage(std::string content, InputInfo& input)
{
    Strings trunks = SplitStringOnDelimiter(content,';');
    if (trunks.size() != 4) {
        g_theConsole->PrintError(Stringf("Fail to parse player input content %s", content.c_str()));
        return false;
    }

    Strings rawPlayer = SplitStringOnDelimiter(trunks[0],',');
    if (rawPlayer.size() != 2) {
        g_theConsole->PrintError(Stringf("Fail to parse player move input %s",trunks[0].c_str()));
    }
    else{
        float playerMoveX = StringConvert(rawPlayer[0].c_str(), 0.f);
        input.playerMove.x = Clamp(playerMoveX, -1.f, 1.f);
        float playerMoveY = StringConvert(rawPlayer[1].c_str(), 0.f);
        input.playerMove.y = Clamp(playerMoveY, -1.f, 1.f);
    }

    Strings rawMouse = SplitStringOnDelimiter(trunks[1], ',');
    if (rawMouse.size() != 2) {
        g_theConsole->PrintError(Stringf("Fail to parse mouse move input %s", trunks[1].c_str()));
    }
    else{
        float mouseMoveX = StringConvert(rawMouse[0].c_str(), 0.f);
        input.mouseMove.x = Clamp(mouseMoveX, -1.f, 1.f);
        float mouseMoveY = StringConvert(rawMouse[1].c_str(), 0.f);
        input.mouseMove.y = Clamp(mouseMoveY, -1.f, 1.f);
    }

    int quit = StringConvert(trunks[2].c_str(),0);
    input.isClosing = quit > 0;

    int fire = StringConvert(trunks[3].c_str(), 0);
    input.isFiring = fire>0;

    return true;

}
