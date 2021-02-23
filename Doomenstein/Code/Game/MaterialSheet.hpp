#pragma once

#include <vector>
#include <string>
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/XMLUtils.hpp"

class Texture;
class SpriteSheet;

class MaterialSheet
{
public:
    static std::vector<MaterialSheet*> sMaterialSheets;
    static void InitRenderAssets();
    static MaterialSheet* GetMaterialSheetFromName(std::string const& name);

    MaterialSheet(XmlElement const& element);

    AABB2 GetSpriteUVs(IntVec2 const& spriteCoords) const;
    Texture* GetDiffuseTex() const {return m_diffuseTex;}

private:
    std::string m_name;
    std::string m_diffusePath;
    IntVec2 m_layout;
    Texture* m_diffuseTex = nullptr;
    SpriteSheet* m_spriteSheet = nullptr;
};