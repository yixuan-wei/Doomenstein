#pragma once

#include <string>
#include <vector>
#include "Engine/Core/XMLUtils.hpp"

class MapMaterialType;
class Texture;

class MapRegionType
{
public:
    static std::vector<MapRegionType*> sMapRegions;
    static void InitMapRegionTypesFromFile(char const* mapRegionTypesPath);
    static MapRegionType* GetMapRegionTypeForName(std::string const& name);

    MapRegionType(XmlElement const& element);   

    bool IsTypeSolid() const {return m_isSolid;}
    AABB2 GetFloorUVs() const;
    AABB2 GetCeilingUVs() const;
    AABB2 GetSideUVs() const;
    Texture* GetTexture() const;

private:
    std::string m_name;
    bool m_isSolid = false;
    MapMaterialType* m_sideMat = nullptr;
    MapMaterialType* m_floorMat = nullptr;
    MapMaterialType* m_ceilingMat = nullptr;
};