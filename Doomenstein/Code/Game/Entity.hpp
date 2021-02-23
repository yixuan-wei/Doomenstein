#pragma once

#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Game/Map.hpp"
#include <string>

class EntityDef;
class Camera;
enum eBillboardMode : int;

enum eEntityType
{
    ENTITY_INVALID = -1,

    ENTITY_ACTOR = 0,
    ENTITY_PORTAL,
    ENTITY_PROJECTILE,
    ENTITY_ENTITY
};

class Entity
{
public:
    Entity(Map* map, EntityDef const* definition);

    virtual void OnProjectileHit(Projectile* proje){proje;}
    virtual void Update();
    virtual void UpdateLocal(float deltaSeconds);
    virtual void Render(Entity* playerPawn) const;

    virtual FloatRange  GetEntityHeightRange()const;

    void MarkAsGarbage();
    void SetIndex(int idx);
    void UpdateMap(Map* newMap);
    void Translate(Vec2 const& translation);
    void SetPosition(Vec2 const& newPos);
    void SetPitchYawRollDegrees(Vec3 const& pitchYawRoll);
    void RotateDeltaPitchYawRollDegrees(Vec3 const& deltaDegrees);

    void SetIsControlledByAI(bool isAIControlled);
    bool SetAnimationName(char const* animName);

    EntityDef const*        GetEntityDefinition() const {return m_entityDef;}
    RaycastResult const&    GetRaycastResult() const {return m_raycast;}
    eBillboardMode  GetBillboardMode() const;
    eEntityType     GetEntityType() const;
    int         GetIndex() const {return m_index;}
    bool        CanPushedByEntities() const;
    bool        CanPushEntities() const;
    bool        CanPushedByWalls() const;
    bool        IsGarbage() const {return m_isGarbage;}
    float       GetWalkSpeed() const;
    float       GetEntityHeight() const;
    float       GetEntityRadius() const;
    Vec2        GetSpriteSize() const;
    Vec2        GetEntityPosition2D() const {return m_position;}
    Vec3        GetEntityPitchYawRollDegrees() const;
    Vec3        GetEntityForward() const {return m_forward;}
    Vec3        GetEntityPosition() const;
    Vec3        GetEntityEyePosition() const;
    Map*        GetMap() const {return m_theMap;}

protected:
    void        UpdateRaycast();
    void        RenderForPosition(Vec3 const& camPos, Vec3 const& camForward, Vec3 const& centerPos) const;

    Vec2        GetLocalVector(Vec2 const& worldVec) const;

protected:
    EntityDef const* m_entityDef = nullptr;
    Map* m_theMap = nullptr;
    int m_index=-1;

    Vec2 m_position;
    float m_pitchDegrees = 0.f;
    float m_rollDegrees = 0.f;
    float m_yawDegrees = 0.f;    
    Vec3 m_forward;

    bool m_isGarbage = false;
    bool m_isControlledByAI = true;
    float m_timerAI = 0.f;
    RaycastResult m_raycast;

    float m_animTimer = 0.f;
    std::string m_animName;
};