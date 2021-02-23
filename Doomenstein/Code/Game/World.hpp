#pragma once

#include <map>
#include <string>
#include "Engine/Core/EventSystem.hpp"

class Map;
class Game;
class Entity;
class Camera;
struct Vec3;
struct RaycastResult;

class World
{
public:
    World(Game* game, char const* mapFolder);
    ~World();

    void InitForRenderAssets();
    void Update();
    void UpdateLocal();
    void Render(Entity* playerPawn) const;

    Entity* SpawnEntityAtPlayerStart(size_t playerIdx, std::string const& entityDefName);

    Game* GetTheGame() const {return m_theGame;}

    bool SwitchMap(Entity* pawn, std::string const& mapName);
    bool SwitchMap(Entity* pawn,std::string const& mapName, Vec2 const& startPos, float startYaw);

private:
    void InitMaps(char const* mapFolder);
    void InitStartMap();

    Map* GetMapFromName(std::string const& name) const;
    std::string GetLoadedMapsNames() const;

private:
    std::map<std::string, Map*> m_maps;
    Map* m_startMap = nullptr;
    Game* m_theGame = nullptr;
};