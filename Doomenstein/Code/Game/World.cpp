#include "Game/World.hpp"
#include "Game/Map.hpp"
#include "Game/Game.hpp"
#include "Game/TileMap.hpp"
#include "Game/Entity.hpp"
#include "Game/Client.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/NamedStrings.hpp"

//////////////////////////////////////////////////////////////////////////
World::World(Game* game, char const* mapFolder)
    : m_theGame(game)
{
    InitMaps(mapFolder);
    InitStartMap();
}

//////////////////////////////////////////////////////////////////////////
World::~World()
{
    for (auto it = m_maps.begin(); it != m_maps.end(); it++) {
        it->second->ClearForRenderAssets();
        delete it->second;
        it->second = nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////
void World::InitForRenderAssets()
{
    for(auto it=m_maps.begin();it!=m_maps.end();it++){
        Map* m = it->second;
        m->InitForRenderAssets();
        m->UpdateMeshes();
    }
}

//////////////////////////////////////////////////////////////////////////
void World::Update()
{
    for(auto it=m_maps.begin();it!=m_maps.end();it++){
        Map* m = it->second;
        m->UpdateForEntities();
        m->UpdateForCollisions();
        m->UpdateForOutOfMap();
    }
}

//////////////////////////////////////////////////////////////////////////
void World::UpdateLocal()
{
    float deltaSeconds = (float)m_theGame->GetClock()->GetLastDeltaSeconds();
    for (auto it = m_maps.begin(); it != m_maps.end(); it++) {
        Map* m = it->second;
        m->UpdateForEntitiesLocal(deltaSeconds);
    }
}

//////////////////////////////////////////////////////////////////////////
void World::Render(Entity* playerPawn) const
{
    playerPawn->GetMap()->Render(playerPawn);
}

//////////////////////////////////////////////////////////////////////////
Entity* World::SpawnEntityAtPlayerStart(size_t playerIdx, std::string const& entityDefName)
{
    if (m_startMap != nullptr) {
        Entity* newPlayer = m_startMap->SpawnNewEntityOfType(entityDefName);
        if (newPlayer==nullptr) {
            newPlayer = m_startMap->SpawnNewEntityOfType("Marine");
        }

        PlayerStart const& start = m_startMap->GetPlayerStartOfIndex(playerIdx);
        newPlayer->SetPosition(start.position);
        newPlayer->SetPitchYawRollDegrees(Vec3(0.f,start.yaw,0.f));
        return newPlayer;
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
bool World::SwitchMap(Entity* pawn, std::string const& mapName, Vec2 const& startPos, float startYaw)
{
    Map* newMap = GetMapFromName(mapName);
    if (newMap == nullptr) {
        g_theConsole->PrintError(Stringf("map %s couldn't be found, loaded maps:\n%s",
            mapName.c_str(), GetLoadedMapsNames().c_str()));
        return false;
    }    

    pawn->UpdateMap(newMap);
    newMap->AppendNewEntity(pawn);
    m_theGame->MoveEntity(pawn,startPos, Vec3(0.f,startYaw,0.f));
    g_theConsole->SetIsOpen(false);
    g_theConsole->PrintString(Rgba8::WHITE, Stringf("Loaded map %s", mapName.c_str()));
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool World::SwitchMap(Entity* pawn, std::string const& mapName)
{
    Map* newMap = GetMapFromName(mapName);
    if (newMap == nullptr) {
        g_theConsole->PrintError(Stringf("map %s couldn't be found, loaded maps:\n%s",
            mapName.c_str(), GetLoadedMapsNames().c_str()));
        return false;
    }

    pawn->UpdateMap(newMap);
    newMap->AppendNewEntity(pawn);
    PlayerStart const& start = newMap->GetPlayerStartOfIndex(0);
    m_theGame->MoveEntity(pawn, start.position, Vec3(0.f,start.yaw,0.f));
    g_theConsole->SetIsOpen(false);
    g_theConsole->PrintString(Rgba8::WHITE, Stringf("Loaded map %s", mapName.c_str()));
    return true;
}

//////////////////////////////////////////////////////////////////////////
void World::InitMaps(char const* mapFolder)
{
    g_theConsole->PrintString(Rgba8(100,0,255), "Start parsing map folder...");

    std::vector<std::string> mapPathList = FilesFindInDirectory(mapFolder, "*.xml");
    for (std::string mapPath : mapPathList) {
        g_theConsole->PrintString(Rgba8(100,100,255), Stringf("parsing %s...", mapPath.c_str()));
        Map* newMap = Map::CreateFromMapPath(mapPath.c_str());
        newMap->m_theWorld = this;
        if (newMap) {
            std::string name = GetFileNameFromPath(mapPath);
            m_maps[name] = newMap;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void World::InitStartMap()
{
    std::string startMapName = g_gameConfigBlackboard->GetValue("startMap", "EmptyRoom");
    m_startMap = GetMapFromName(startMapName);

    if (m_startMap == nullptr) {
        g_theConsole->PrintError(Stringf("Fail to load StartMap %s defined in GameConfig", startMapName.c_str()));
        if (m_maps.size() > 0) {
            auto iter = m_maps.begin();
            m_startMap = iter->second;
            startMapName = iter->first;
        }
        else {
            g_theConsole->PrintError("No maps loaded correctly");
            return;
        }
    }

    g_theConsole->PrintString(Rgba8::WHITE, Stringf("Loaded map %s", startMapName.c_str()));
}

//////////////////////////////////////////////////////////////////////////
Map* World::GetMapFromName(std::string const& name) const
{
    std::map<std::string, Map*>::const_iterator iter = m_maps.find(name);
    if (iter != m_maps.end()) {
        return iter->second;
    }
    else return nullptr;
}

//////////////////////////////////////////////////////////////////////////
std::string World::GetLoadedMapsNames() const
{
    std::string names;
    for (auto iter = m_maps.begin(); iter != m_maps.end(); iter++) {
        names += iter->first;
        names+='\n';
    }
    return names;
}
