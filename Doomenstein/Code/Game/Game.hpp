#pragma once

#include "Engine/Math/AABB2.hpp"
#include "Game/GameCommon.hpp"
#include <string>
#include <vector>

class RandomNumberGenerator;
class SpriteSheet;
class Entity;
class Clock;
class World;
class AudioSystem;
class RenderContext;
class Camera;
class BitmapFont;
typedef size_t SoundID;

enum eGameType
{
    GAME_SINGLE_PLAYER,
    GAME_MULTI_PLAYER
};

//////////////////////////////////////////////////////////////////////////
class Game
{
    friend class AuthoritativeServer;
    friend class RemoteServer;

public:
    Game(){}
    virtual ~Game() {}

    virtual void StartUp();
    virtual void ShutDown();
    virtual void Update();
    virtual void UpdateLocal();
    virtual void Render(Entity* playerPawn)const;

    virtual void InitializeAssets(AudioSystem* audioSys, RenderContext* rtx);

    virtual Entity* SpawnEntityAtPlayerStart(size_t playerIdx, std::string const& entityDefName="Marine");
    virtual void SwitchMapForEntity(std::string const& mapName, Entity* playerPawn);
    virtual void MoveEntity(Entity* playerPawn, Vec2 const& pos, Vec3 const& pitchYawRoll);
    virtual void RemoveEntity(Entity* playerPawn);
    virtual void AddNewlySpawnedEntity(Entity* entity);

    virtual void PlayTeleportSound() const;
    virtual void UpdateCameraForPawn(Entity* playerPawn);
    virtual void UpdatePlayerForInput(Entity* playerPawn, InputInfo const& info);

    virtual void RenderForRaycastDebug(Entity* pawn) const;

    virtual void UpdateKeyboardStates(InputInfo& input) const;

    Entity* GetEntityOfIndex(int idx) const;
    Clock* GetClock() const { return m_gameClock; }

protected:
    eGameType m_type = GAME_SINGLE_PLAYER;
    Clock* m_gameClock = nullptr;

    SoundID m_testSoundID = 0;
    AABB2 m_hud;
    AABB2 m_gun;
    SpriteSheet* m_viewSheet = nullptr;

    World* m_world = nullptr;
    std::vector<Entity*> m_entities;
    int m_entityIdx=0;

    Camera* m_worldCamera = nullptr;
    Camera* m_uiCamera = nullptr;

    virtual void UpdateInputForMovement(InputInfo& input) const;

    virtual void RenderForWorld(Entity* playerPawn) const;
    virtual void RenderForUI(Entity* playerPawn) const;
    virtual void RenderForDebug() const;
};