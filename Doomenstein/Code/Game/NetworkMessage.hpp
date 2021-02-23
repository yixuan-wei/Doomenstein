#pragma once

#include <string>

class Entity;
class TCPServer;
class TCPSocket;
struct InputInfo;
struct Vec2;

enum eNetMessageHeaderType : unsigned short
{
    MESSAGE_INVALID=0,

    MESSAGE_ADD_PLAYER,
    MESSAGE_PLAYER_INPUT,
    MESSAGE_ENTITY_TRANSFORM,
    MESSAGE_ENTITY_CREATE,
    MESSAGE_ENTITY_DELETE,
    MESSAGE_ENTITY_TELEPORT,
    MESSAGE_ACTOR_HEALTH,
    MESSAGE_SOUND_PLAY,

    MESSAGE_ACK
};

struct NetMessageHeader
{
    unsigned short m_type=0;
    unsigned short m_size=0;
    unsigned short m_seqNo=0;
};

constexpr int MESSAGE_HEADER_LEN = (int)sizeof(NetMessageHeader);

eNetMessageHeaderType GetHeaderTypeForMessage(std::string const& msg);
unsigned short GetSeqNoForMessage(std::string const& msg);
int GetEntityIdxFromOneTrunkMessage(std::string const& msg);
int GetEntityIdxFromTwoChunksMessage(std::string const& msg);
void GetEntityCreateInfoFromMessage(std::string const& msg, int& idx, std::string& type);

std::string MakeMessageHeader(eNetMessageHeaderType type);
std::string MakeCustomClientConnectPackage(std::string const& clientIP);
std::string MakePlayerInputMessage(InputInfo const& input);
std::string MakeEntityTransformMessage(Entity const* entity);
std::string MakeEntityCreateMessage(Entity const* entity);
std::string MakeEntityTeleportMessage(Entity const* entity);
std::string MakeEntityTeleportMessage(int entityIdx, std::string const& mapName);
std::string MakeEntityDeleteMessage(int entityIdx);
std::string MakeSoundPlayMessage(size_t id);
std::string MakeActorHealthMessage(int entityIdx, float newHealth);
std::string MakeReliableACKMessage(unsigned short msgSeqNo);
std::string MakeClientStartMessage(int udpToPort, int udpBindPort);

bool ParseActorHealthMessage(std::string const& content);
bool ParseEntityDeleteMessage(std::string const& content);
bool ParseEntityTeleportMessage(std::string const& content);
bool ParseClientConnectPackage(std::string const& content);
bool ParseClientStartMessage(std::string const& content, TCPSocket* client);
Entity* ParseEntityCreateMessage(std::string const& content);
bool ParseEntityTransformMessage(std::string const& content);
bool ParsePlayerInputMessage(std::string content, InputInfo& input);
