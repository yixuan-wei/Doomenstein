#include "Game/NetworkObserver.hpp"
#include "Game/NetworkMessage.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Server.hpp"
#include "Game/Entity.hpp"
#include "Game/App.hpp"
#include "Engine/Network/NetworkCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/NamedProperties.hpp"

static int udpFailNum = 0;

//////////////////////////////////////////////////////////////////////////
COMMAND(UDPReceiveFail, "receive UDP fail, quit", EVENT_NETWORK)
{
    udpFailNum++;

    if (g_theServer->m_isAuthoritative && udpFailNum>30) {
        void* udp = args.GetValue("ptr", (void*)nullptr);
        Client* c = g_theServer->GetClientOfUDPSocket(static_cast<UDPSocket*>(udp));
        if (c) {
            g_theServer->RemovePlayer(c);
        }
    }
    else if(udpFailNum>30){
        g_theApp->HandleQuitRequisted();
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
COMMAND(UDPSocketReceive, "received udp socket, data=... ptr=&(UDPSocket)", EVENT_NETWORK)
{
    udpFailNum = 0;

    std::string data = args.GetValue("data","");
    if (data.empty()) {
        return false;
    }

    NetworkPackageHeader* headerPtr = reinterpret_cast<NetworkPackageHeader*>(&data[0]);
    int identifier = headerPtr->m_key;
    bool reliable = headerPtr->m_reliable;
    if (headerPtr->m_type == eNetworkPackageHeaderType::HEAD_CLIENT_CLOSE) {
        g_theServer->RemovePlayerOfIdentifier(identifier);  //quit not reliable
        return true;
    }
    else if(headerPtr->m_type==eNetworkPackageHeaderType::HEAD_TEXT){    
        std::string messages = std::string(&data[NET_HEADER_LEN], headerPtr->m_size);
        while(!messages.empty()){
            NetMessageHeader* header = reinterpret_cast<NetMessageHeader*>(&messages[0]);
            g_theServer->HandleUDPMessageOfIdentifier(*header, std::string(&messages[MESSAGE_HEADER_LEN], header->m_size), identifier,reliable);
            messages = messages.substr(MESSAGE_HEADER_LEN + header->m_size);
        }
        return true;
    }

    //TODO not handled other type of package
    return false;
}

//////////////////////////////////////////////////////////////////////////
void PackUpMessages(std::vector<std::string> const& msgs, std::queue<std::string>& packages, bool reliable)
{
    std::string curPack;
    for (size_t i = 0; i < msgs.size(); i++) {
        std::string const& pack = msgs[i];
        //TODO not handled package len > message length
        if (curPack.size() + pack.size() <= NET_MAX_DATA_LEN) {
            curPack += pack;
        }
        else {
            packages.push(MakeTextPackage(curPack, reliable));
            curPack = pack;
        }
    }
    if (!curPack.empty()) {
        packages.push(MakeTextPackage(curPack, reliable));
    }
}

//////////////////////////////////////////////////////////////////////////
NetworkObserver::NetworkObserver()
{
    m_sendTimer.SetTimerSeconds(1.0 / (double)g_sendRatePerSec);
}

//////////////////////////////////////////////////////////////////////////
void NetworkObserver::AddEntityTransformUpdate(Entity* entity)
{
    for (Entity* e : m_entityTransformChanged) {
        if (e == entity) {
            return;
        }
    }

    m_entityTransformChanged.push_back(entity);
}

//////////////////////////////////////////////////////////////////////////
void NetworkObserver::AddSoundPlay(size_t id)
{
    for (size_t i : m_SFXToPlay) {
        if (i == id) {
            return;
        }
    }

    m_SFXToPlay.push_back(id);
}

//////////////////////////////////////////////////////////////////////////
void NetworkObserver::AddMessage(std::string const& package)
{
    m_messages.push_back(package);
}

//////////////////////////////////////////////////////////////////////////
void NetworkObserver::Restart()
{
    m_entityTransformChanged.clear();
    m_SFXToPlay.clear();
    m_messages.clear();

    std::queue<std::string> emptyQueue;
    m_packages.swap(emptyQueue);
}

//////////////////////////////////////////////////////////////////////////
void NetworkObserver::EndFrame()
{
    if(m_sendTimer.CheckAndReset()){
        UpdateEntityTransformMessages();
        UpdateSoundPlayMessages();
        UpdatePackages();

        while(!m_packages.empty()){
            std::string pack = m_packages.front();
            g_theServer->SendOneMessage(pack);
            m_packages.pop();
        }

        g_theServer->SendReliableMessages();
    }
}

//////////////////////////////////////////////////////////////////////////
void NetworkObserver::UpdateSendTimer(Clock* clock)
{
    m_sendTimer.SetTimerSeconds(clock,1.f/(double)g_sendRatePerSec);
}

//////////////////////////////////////////////////////////////////////////
void NetworkObserver::UpdateSoundPlayMessages()
{
    for (size_t i : m_SFXToPlay) {
        std::string newMsg = MakeSoundPlayMessage(i);
        AddMessage(newMsg);
    }

    m_SFXToPlay.clear();
}

//////////////////////////////////////////////////////////////////////////
void NetworkObserver::UpdateEntityTransformMessages()
{
    for (Entity* e : m_entityTransformChanged) {
        std::string newPackage = MakeEntityTransformMessage(e);
        AddMessage(newPackage);
    }

    m_entityTransformChanged.clear();
}

//////////////////////////////////////////////////////////////////////////
void NetworkObserver::UpdatePackages()
{
    PackUpMessages(m_messages, m_packages);
    m_messages.clear();
}
