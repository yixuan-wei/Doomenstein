#include "Game/EntityDefinition.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Entity.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/IntVec2.hpp"

std::vector<EntityDef*> EntityDef::sEntityDefs;

//////////////////////////////////////////////////////////////////////////
void EntityDef::InitEntityDefinitionsFromFile(char const* entityDefPath)
{
    g_theConsole->PrintString(Rgba8(100,0,255), Stringf("Start parsing %s...", entityDefPath));

    XmlDocument entityDoc;
    XmlError code = entityDoc.LoadFile(entityDefPath);
    if (code != XmlError::XML_SUCCESS) {
        g_theConsole->PrintError(Stringf("Fail to load xml %s, code %i", entityDefPath, (int)code));
        return;
    }

    XmlElement* root = entityDoc.RootElement();
    if (root == nullptr) {
        g_theConsole->PrintError("Fail to retrieve root element");
        return;
    }
    if (strcmp(root->Name(), "EntityTypes") != 0) {
        g_theConsole->PrintError(Stringf("Root %s invalid for %s", root->Name(), entityDefPath));
        return;
    }

    XmlElement const* type = root->FirstChildElement();
    while (type != nullptr) {
        EntityDef* newEntityDef = new EntityDef(*type);
        if (!newEntityDef->m_isValid) {
            delete newEntityDef;
            g_theConsole->PrintError(Stringf("fail to construct from element %s, check format", type->Name()));
        }
        type = type->NextSiblingElement();
    }
}

//////////////////////////////////////////////////////////////////////////
EntityDef* EntityDef::GetEntityDefinitionFromName(std::string const& name)
{
    for (size_t i = 0; i < sEntityDefs.size(); i++) {
        EntityDef* entDef = sEntityDefs[i];
        if (entDef->m_name == name) {
            return entDef;
        }
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
EntityDef::EntityDef(XmlElement const& element)
{
    m_type=GetEntityTypeFromText(element.Name());
    if (m_type == ENTITY_INVALID) {
        g_theConsole->PrintError("Entity type invalid: Entity/Actor/Projectile/Portal supported only");
        return;
    }

    m_name = ParseXmlAttribute(element, "name", "");
    if (m_name.empty()) {
        g_theConsole->PrintError("EntityType with NULL name");
        return;
    }

    bool physicsInited = false;
    bool apperanceInited = false;
    bool gameplayInited = false;
    XmlElement const* child = element.FirstChildElement();
    while(child != nullptr) {
        if(strcmp(child->Name(), "Physics")==0){    //Physics
            if (physicsInited) {
                g_theConsole->PrintError(Stringf("Multiple Physics element for entity %s defined", m_name.c_str()));
            }
            else{
                m_radius = ParseXmlAttribute(*child, "radius", 0.f);
                m_height = ParseXmlAttribute(*child, "height", 0.f);
                m_eyeHeight = ParseXmlAttribute(*child, "eyeHeight", m_height);
                m_walkSpeed = ParseXmlAttribute(*child, "walkSpeed", 0.f);
                m_speed = ParseXmlAttribute(*child, "speed", 0.f);

                if (m_radius <= 0.f) {
                    g_theConsole->PrintString(Rgba8::MAGENTA, Stringf("%s has non positive radius %f", m_name.c_str(), m_radius));
                }
                if (m_height <= 0.f) {
                    g_theConsole->PrintString(Rgba8::MAGENTA, Stringf("%s has non positive height %f", m_name.c_str(), m_height));
                }
                if (m_eyeHeight <= 0.f) {
                    g_theConsole->PrintString(Rgba8::MAGENTA, Stringf("%s has non positive eye height %f", m_name.c_str(), m_eyeHeight));
                }
                if (m_walkSpeed <= 0.f) {
                    g_theConsole->PrintString(Rgba8::MAGENTA, Stringf("%s has non positive walk speed %f", m_name.c_str(), m_walkSpeed));
                }
                if (m_speed <= 0.f) {
                    g_theConsole->PrintString(Rgba8::MAGENTA, Stringf("%s has non positive speed %f", m_name.c_str(), m_speed));
                }

                physicsInited = true;
            }
        }
        else if (strcmp(child->Name(), "Appearance") == 0) {    //Appearance
            if (apperanceInited) {
                g_theConsole->PrintError(Stringf("Multiple Appearance element for entity %s defined", m_name.c_str()));
            }
            else {
                m_spriteSize = ParseXmlAttribute(*child, "size", Vec2::ZERO);
                if (m_spriteSize.x <= 0.f || m_spriteSize.y <= 0.f) {
                    g_theConsole->PrintError(Stringf("%s has non positive sprite size (%f, %f)", m_name.c_str(), m_spriteSize.x, m_spriteSize.y));
                }
                else {
                    std::string billboardText = ParseXmlAttribute(*child, "billboard", "CameraFacingXY");
                    m_billboardMode = GetBillboardModeFromText(billboardText);

                    std::string texPath = ParseXmlAttribute(*child, "spriteSheet", "White");
                    Texture* sheetTex = g_theRenderer->CreateOrGetTextureFromFile(texPath.c_str());
                    if (sheetTex == nullptr) {
                        g_theConsole->PrintError(Stringf("Set sprite sheet of %s to White", m_name.c_str()));
                        sheetTex = g_theRenderer->CreateOrGetTextureFromFile("White");
                    }
                    IntVec2 layout = ParseXmlAttribute(*child, "layout", IntVec2(1,1));
                    m_spriteSheet = new SpriteSheet(*sheetTex, layout);

                    XmlElement const* animElement =  child->FirstChildElement();
                    while (animElement!=nullptr){
                        if (!InitOneAnimationElement(*animElement)) {
                            g_theConsole->PrintError(Stringf("Fail to parse some animations in %s", m_name.c_str()));
                        }
                        animElement = animElement->NextSiblingElement();
                    }
                }
                apperanceInited = true;
            }
        }
        else if (strcmp(child->Name(), "Gameplay") == 0) {  //gameplay
            if (gameplayInited) {
                g_theConsole->PrintError(Stringf("Multiple Gameplay element defined in %s", m_name.c_str()));
            }
            else{
                m_damage = ParseXmlAttribute(*child, "damage", FloatRange(0.f));
                m_health = ParseXmlAttribute(*child, "health", 0.f);
                if (m_damage.minimum <= 0.f) {
                    g_theConsole->PrintString(Rgba8::MAGENTA, Stringf("damage of %s set to non-positive (%f-%f)", m_name.c_str(), m_damage.minimum, m_damage.maximum));
                }
                if (m_health < 0.f) {
                    g_theConsole->PrintString(Rgba8::MAGENTA, Stringf("health of %s set to negative %f", m_name.c_str(), m_health));
                }
                gameplayInited = true;
            }
        }
        else {
            g_theConsole->PrintError(Stringf("Unrecognized element %s in entity %s", child->Name(), m_name.c_str()));
        }

        child = child->NextSiblingElement();
    }

    m_isValid = true;
    EntityDef::sEntityDefs.push_back(this);
}

//////////////////////////////////////////////////////////////////////////
Texture const* EntityDef::GetTexture() const
{
    if (m_spriteSheet != nullptr) {
        return &m_spriteSheet->GetTexture();
    }
    else return nullptr;
}

//////////////////////////////////////////////////////////////////////////
bool EntityDef::IsAnimNameExist(char const* animName) const
{
    std::map<std::string, std::vector<AnimStruct>>::const_iterator iter = m_animations.find(animName);
    return iter!=m_animations.end();
}

//////////////////////////////////////////////////////////////////////////
void EntityDef::GetAnimSpriteUVs(std::string const& animName, float animTime, Vec2 const& localDir, Vec2& uvMins, Vec2& uvMaxs) const
{
    std::map<std::string, std::vector<AnimStruct>>::const_iterator iter = m_animations.find(animName);
    if (iter == m_animations.end()) {   //find fail, return first
        iter = m_animations.begin();
    }

    if (iter == m_animations.end()) {   //non exist, return
        return;
    }

    std::vector<AnimStruct> const& anims = iter->second;
    float dotProduct = 0.f;
    unsigned int selectedIndex = 0;
    for (unsigned int i = 0; i < anims.size(); i++) {
        float newDot = DotProduct2D(anims[i].idealNormal, localDir);
        if (newDot > dotProduct) {
            selectedIndex = i;
            dotProduct = newDot;
        }
    }
    SpriteDefinition const& spriteDef = anims[selectedIndex].animDef->GetSpriteDefAtTime(animTime);
    spriteDef.GetUVs(uvMins, uvMaxs);
}

//////////////////////////////////////////////////////////////////////////
bool EntityDef::InitOneAnimationElement(XmlElement const& element)
{
    std::string animName = element.Name();
    if (animName.empty()) {
        g_theConsole->PrintError("Animation should have a name");
        return false;
    }

    bool successParsed = true;
    XmlAttribute const* animAttr = element.FirstAttribute();
    std::vector<AnimStruct> animations;
    while (animAttr != nullptr) {
        AnimStruct newAnim;
        std::vector<int> spriteIndexes;
        spriteIndexes = ParseXmlAttribute(element, animAttr->Name(), spriteIndexes);
        newAnim.animDef = new SpriteAnimDefinition(*m_spriteSheet, spriteIndexes, 1.f);
        newAnim.idealNormal = GetIdealNormalFromDirectionText(animAttr->Name());
        if (newAnim.idealNormal != Vec2::ZERO) {
            animations.push_back(newAnim);
        }
        else {
            successParsed = false;
        }
        animAttr = animAttr->Next();
    }
    
    m_animations[animName] = animations;
    return successParsed;
}

//////////////////////////////////////////////////////////////////////////
Vec2 GetIdealNormalFromDirectionText(char const* text)
{
    if(strcmp(text, "front")==0)           { return Vec2(1.f, 0.f);   }
    else if(strcmp(text, "frontRight")==0) { return Vec2(fSQRT_2_OVER_2, -fSQRT_2_OVER_2); }
    else if(strcmp(text, "right")==0)      { return Vec2(0.f, -1.f); }
    else if(strcmp(text, "backRight")==0)  { return Vec2(-fSQRT_2_OVER_2, -fSQRT_2_OVER_2); }
    else if(strcmp(text, "back")==0)       { return Vec2(-1.f, 0.f); }
    else if(strcmp(text, "backLeft")==0)   { return Vec2(-fSQRT_2_OVER_2, fSQRT_2_OVER_2); }
    else if(strcmp(text, "left")==0)       { return Vec2(0.f, 1.f); }
    else if(strcmp(text, "frontLeft")==0)  { return Vec2(fSQRT_2_OVER_2, fSQRT_2_OVER_2); }
    else {
        g_theConsole->PrintError(Stringf("Fail to recognize animation direction %s", text));
        return Vec2::ZERO;
    }
}

//////////////////////////////////////////////////////////////////////////
eEntityType GetEntityTypeFromText(std::string const& text)
{
    if (text=="Actor") {
        return ENTITY_ACTOR;
    }
    else if (text== "Projectile") {
        return ENTITY_PROJECTILE;
    }
    else if (text== "Portal") {
        return ENTITY_PORTAL;
    }
    else if (text== "Entity") {
        return ENTITY_ENTITY;
    }
    else {
        return ENTITY_INVALID;
    }
}

//////////////////////////////////////////////////////////////////////////
eBillboardMode GetBillboardModeFromText(std::string const& text)
{
    if (text == "CameraFacingXY") {
        return CAM_FACING_XY;
    }
    else if (text == "CameraOpposingXY") {
        return CAM_OPPOSING_XY;
    }
    else if (text == "CameraFacingXYZ") {
        return CAM_FACING_XYZ;
    }
    else if (text == "CameraOpposingXYZ") {
        return CAM_OPPOSING_XYZ;
    }
    else {
        g_theConsole->PrintError(Stringf("Fail to translate %s into billboard mode, set to CameraFacingXY", text.c_str()));
        return CAM_FACING_XY;
    }
}
