#include "Game/Client.hpp"
#include "Game/Server.hpp"
#include "Game/NetworkMessage.hpp"
#include "Game/RemoteClient.hpp"
#include "Game/PlayerClient.hpp"
#include "Game/NetworkObserver.hpp"
#include "Engine/Network/UDPSocket.hpp"
#include "Engine/Network/Network.hpp"
#include "Engine/Network/TCPSocket.hpp"
#include "Engine/Core/StringUtils.hpp"

//////////////////////////////////////////////////////////////////////////
Client* Client::StartClientWithBool(bool isPlayerClient)
{
    if (isPlayerClient) {
        return (Client*)new PlayerClient();
    }
    else {
        return (Client*)new RemoteClient();
    }
}

//////////////////////////////////////////////////////////////////////////
void Client::Shutdown()
{    
    std::string quitMsg = MakeClientQuitPackage();
    NetworkPackageHeader* head = reinterpret_cast<NetworkPackageHeader*>(&quitMsg[0]);
    head->m_key = m_identifier;

    if (m_tcpClientSocket) {
        m_tcpClientSocket->Send(quitMsg.data(), quitMsg.size());
        m_tcpClientSocket->Shutdown();
    }

    if (m_udpSocket) {
        if (m_udpSocket->IsValid()) {
            g_theNetwork->StopUDPSocket(m_udpSocket->GetBindPort());
        }
        m_udpSocket = nullptr;
    }
    m_isQuiting = true;
}

//////////////////////////////////////////////////////////////////////////
void Client::SendReliableMessages()
{
    if (m_udpSocket == nullptr || !m_udpSocket->IsValid() || m_reliableMsgs.empty()) {
        return;
    }

    std::queue<std::string> packages;
    PackUpMessages(m_reliableMsgs, packages, true);
    while (!packages.empty()) {
        std::string pack = packages.front();
        NetworkPackageHeader* headerPtr = reinterpret_cast<NetworkPackageHeader*>(&pack[0]);
        headerPtr->m_key = m_identifier;
        m_udpSocket->SendUDPMessage(pack);
        packages.pop();
    }
}

//////////////////////////////////////////////////////////////////////////
void Client::InsertDeleteMsg(std::string const& deleteMsg)
{
    if (GetHeaderTypeForMessage(deleteMsg) != MESSAGE_ENTITY_DELETE) {
        return;
    }

    unsigned short seqNo = GetSeqNoForMessage(deleteMsg);
    int entityIdx = GetEntityIdxFromOneTrunkMessage(deleteMsg);

    for (std::string const& str : m_reliableMsgs) {
        if (GetSeqNoForMessage(str) == seqNo ||
            (GetHeaderTypeForMessage(str)==MESSAGE_ENTITY_DELETE && 
            GetEntityIdxFromOneTrunkMessage(str) == entityIdx)) {
            return;
        }
    }
    m_reliableMsgs.push_back(deleteMsg);
}

//////////////////////////////////////////////////////////////////////////
void Client::InsertHealthMsg(std::string const& healthMsg)
{
    if (GetHeaderTypeForMessage(healthMsg) != MESSAGE_ACTOR_HEALTH) {
        return;
    }

    unsigned short seqNo = GetSeqNoForMessage(healthMsg);
    int entityIdx = GetEntityIdxFromTwoChunksMessage(healthMsg);

    //delete outdated health msg
    for (size_t i = 0; i < m_reliableMsgs.size();) {
        std::string const& str = m_reliableMsgs[i];
        if (GetSeqNoForMessage(str) == seqNo ||
            (GetHeaderTypeForMessage(str)==MESSAGE_ACTOR_HEALTH && 
            GetEntityIdxFromTwoChunksMessage(str) == entityIdx)) {
            m_reliableMsgs.erase(m_reliableMsgs.begin()+i);
        }
        else {
            i++;
        }
    }

    m_reliableMsgs.push_back(healthMsg);
}

//////////////////////////////////////////////////////////////////////////
void Client::InsertTeleportMsg(std::string const& teleportMsg)
{
    eNetMessageHeaderType type = GetHeaderTypeForMessage(teleportMsg);
    if (type != MESSAGE_ENTITY_TELEPORT) {
        return;
    }

    unsigned short seqNo = GetSeqNoForMessage(teleportMsg);
    int entityIdx = GetEntityIdxFromTwoChunksMessage(teleportMsg);

    //delete old teleport
    for (size_t i = 0; i < m_reliableMsgs.size();) {
        std::string const& str = m_reliableMsgs[i];
        if (GetSeqNoForMessage(str) == seqNo ||
            (GetHeaderTypeForMessage(str) == type && 
            GetEntityIdxFromTwoChunksMessage(str) == entityIdx)) {
            m_reliableMsgs.erase(m_reliableMsgs.begin() + i);
        }
        else {
            i++;
        }
    }

    m_reliableMsgs.push_back(teleportMsg);
}

//////////////////////////////////////////////////////////////////////////
void Client::InsertCreateMsg(std::string const& createMsg)
{
    eNetMessageHeaderType type = GetHeaderTypeForMessage(createMsg);
    if (type != MESSAGE_ENTITY_CREATE) {
        return;
    }

    unsigned short seqNo = GetSeqNoForMessage(createMsg);
    int idx =0;
    std::string typeName;
    GetEntityCreateInfoFromMessage(createMsg, idx, typeName);

    //delete old create
    for (size_t i = 0; i < m_reliableMsgs.size();) {
        std::string const& str = m_reliableMsgs[i];
        int index=0;
        std::string thisName;
        GetEntityCreateInfoFromMessage(str, index, thisName);
        if ((GetHeaderTypeForMessage(str) == type && index == idx && typeName == thisName)
            || GetSeqNoForMessage(str) == seqNo) {
            m_reliableMsgs.erase(m_reliableMsgs.begin() + i);
        }
        else {
            i++;
        }
    }

    m_reliableMsgs.push_back(createMsg);
}

//////////////////////////////////////////////////////////////////////////
void Client::ReceiveACKMessage(std::string const& content)
{
    unsigned short seqNo = (unsigned short)StringConvert(content.c_str(), 0u);
    for (size_t i = 0; i < m_reliableMsgs.size();) {
        std::string const& str = m_reliableMsgs[i];
        if (GetSeqNoForMessage(str) == seqNo) {
            m_reliableMsgs.erase(m_reliableMsgs.begin()+i);
        }
        else {
            i++;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
std::string Client::MakeQuitPackage() const
{
    std::string header = MakeClientQuitPackage();
    NetworkPackageHeader* headerPtr = reinterpret_cast<NetworkPackageHeader*>(&header[0]);
    headerPtr->m_key = m_identifier;
    return header;
}
