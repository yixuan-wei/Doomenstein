#pragma once

#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Game/GameCommon.hpp"
#include <vector>
#include <string>
#include <map>

class SpriteSheet;
class Texture;
class SpriteAnimDefinition;
enum eEntityType : int;

struct AnimStruct
{
    Vec2             idealNormal;
    SpriteAnimDefinition* animDef;
};

Vec2            GetIdealNormalFromDirectionText(char const* text);
eEntityType     GetEntityTypeFromText(std::string const& text);
eBillboardMode  GetBillboardModeFromText(std::string const& text);

//////////////////////////////////////////////////////////////////////////
class EntityDef
{
public:
    static std::vector<EntityDef*> sEntityDefs;
    static void InitEntityDefinitionsFromFile(char const* entityDefPath);
    static EntityDef* GetEntityDefinitionFromName(std::string const& name);

    EntityDef(XmlElement const& element);

    Texture const* GetTexture() const;
    bool IsAnimNameExist(char const* animName) const; 
    void GetAnimSpriteUVs(std::string const& animName, float animTime, Vec2 const& localDir, Vec2& uvMins, Vec2& uvMaxs) const;

public:
    bool m_isValid = false;

    eEntityType m_type;
    std::string m_name;

    Vec2 m_spriteSize;
    eBillboardMode m_billboardMode  = CAM_FACING_XY;
    SpriteSheet* m_spriteSheet      = nullptr;
    std::map<std::string, std::vector<AnimStruct>> m_animations;

    float m_radius      = 0.f;
    float m_height      = 0.f;
    float m_eyeHeight   = 0.f;
    float m_walkSpeed   = 0.f;
    float m_speed       = 0.f;
    float m_mass        = 1.f;
    bool m_canBePushedByWalls    = true;
    bool m_canBePushedByEntities = true;
    bool m_canPushEntities       = true;

    float m_health = 0.f;
    FloatRange m_damage;

private:
    bool InitOneAnimationElement(XmlElement const& element);
};