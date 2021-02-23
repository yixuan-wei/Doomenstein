#pragma once

#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include <vector>
#include <string>

class SpriteSheet;
class MaterialSheet;
class Texture;

class MapMaterialType
{
public:
    static std::vector<MapMaterialType*> sMapMaterials;
    static void InitMapMaterialTypesFromFile(char const* mapMaterialPath);
    static void InitRenderAssets();
    static MapMaterialType* GetMapMaterialTypeFromName(std::string const& name);

    MapMaterialType(XmlElement const& element);

    AABB2 GetUVs() const {return m_uvs;}
    Texture* GetTexture() const;

private:
    std::string m_name;
    MaterialSheet* m_sheet = nullptr;
    IntVec2 m_spriteCoords;
    AABB2 m_uvs;
};