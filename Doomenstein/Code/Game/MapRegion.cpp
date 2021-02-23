#include "Game/MapRegion.hpp"
#include "Game/GameCommon.hpp"
#include "Game/MapMaterial.hpp"
#include "Engine/Core/DevConsole.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

std::vector<MapRegionType*> MapRegionType::sMapRegions;

//////////////////////////////////////////////////////////////////////////
void MapRegionType::InitMapRegionTypesFromFile(char const* mapRegionTypesPath)
{
    g_theConsole->PrintString(Rgba8(100,0,255), Stringf("Start parsing %s...", mapRegionTypesPath));

    XmlDocument mapRegionDoc;
    XmlError code = mapRegionDoc.LoadFile(mapRegionTypesPath);
    if (code != XmlError::XML_SUCCESS) {
        g_theConsole->PrintError(Stringf("Fail to load xml %s, code %i", mapRegionTypesPath, (int)code));
        return;
    }

    XmlElement* root = mapRegionDoc.RootElement();
    if (root == nullptr) {
        g_theConsole->PrintError(Stringf("Fail to retrieve root element of %s", mapRegionTypesPath));
        return;
    }
    if (strcmp(root->Name(), "MapRegionTypes") != 0){
        g_theConsole->PrintError(Stringf("Root %s invalid for %s", root->Name(), mapRegionTypesPath));
        return;
    }

    XmlElement const* regionType = root->FirstChildElement();
    while (regionType != nullptr && strcmp(regionType->Name(), "RegionType")==0) {
        new MapRegionType(*regionType);
        //TODO count for sMapRegions number to avoid memory leak
        regionType = regionType->NextSiblingElement();
    }

    if (regionType != nullptr) {
        g_theConsole->PrintError(Stringf("MapRegionTypes has unrecognized %s element, check format", regionType->Name()));
    }
}

//////////////////////////////////////////////////////////////////////////
MapRegionType* MapRegionType::GetMapRegionTypeForName(std::string const& name)
{
    for (size_t i = 0; i < sMapRegions.size(); i++) {
        MapRegionType* region = sMapRegions[i];
        if (region->m_name == name) {
            return region;
        }
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
MapRegionType::MapRegionType(XmlElement const& element)
{
    m_name = ParseXmlAttribute(element, "name", "");
    if (m_name.empty()) {
        g_theConsole->PrintError("MapRegion with NULL name");
        return;
    }

    m_isSolid = ParseXmlAttribute(element, "isSolid", false);

    XmlElement const* side = element.FirstChildElement("Side");
    XmlElement const* floor = element.FirstChildElement("Floor");
    XmlElement const* ceiling = element.FirstChildElement("Ceiling");
    if (m_isSolid) {    // only side material
        if (floor != nullptr || ceiling != nullptr) {
            g_theConsole->PrintError(Stringf("MapRegionType %s is solid but has Floor/Ceiling element", m_name.c_str()));
        }
        if (side == nullptr) {
            g_theConsole->PrintError(Stringf("MapRegionType %s solid, has no Side element", m_name.c_str()));
            return;
        }

        std::string sideMatName = ParseXmlAttribute(*side, "material", "");
        m_sideMat = MapMaterialType::GetMapMaterialTypeFromName(sideMatName);
        if (m_sideMat == nullptr) {
            g_theConsole->PrintError(Stringf("MapRegionType %s Side element INVALID", m_name.c_str()));
            //TODO add fall back MapMaterial
            return;
        }

        if (side->NextSiblingElement("Side") != nullptr) {
            g_theConsole->PrintError(Stringf("MapRegionType %s has multiple Side element", m_name.c_str()));
        }
    }
    else {  // only floor and ceiling
        if (side != nullptr) {
            g_theConsole->PrintError(Stringf("MapRegionType %s non-solid but has Side element", m_name.c_str()));
        }
        if (floor == nullptr || ceiling == nullptr) {
            g_theConsole->PrintError(Stringf("MapRegionType %s non solid, has no floor/ceiling element", m_name.c_str()));
            return;
        }

        std::string floorMatName = ParseXmlAttribute(*floor, "material", "");
        std::string ceilingMatName = ParseXmlAttribute(*ceiling, "material", "");
        m_floorMat = MapMaterialType::GetMapMaterialTypeFromName(floorMatName);
        m_ceilingMat = MapMaterialType::GetMapMaterialTypeFromName(ceilingMatName);
        if (m_floorMat == nullptr) {
            g_theConsole->PrintError(Stringf("MapRegionType %s floor material %s not exist", m_name.c_str(), floorMatName.c_str()));
            return;
        }
        if (m_ceilingMat == nullptr) {
            g_theConsole->PrintError(Stringf("MapRegionType %s ceiling material %s not exist", m_name.c_str(), ceilingMatName.c_str()));
            return;
        }

        if (floor->NextSiblingElement("Floor") != nullptr)  {
            g_theConsole->PrintError(Stringf("MapRegionType %s has multiple Floor element", m_name.c_str()));
        }
        if (ceiling->NextSiblingElement("Ceiling") != nullptr) {
            g_theConsole->PrintError(Stringf("MapRegionType %s has multiple ceiling element", m_name.c_str()));
        }
    }

    sMapRegions.push_back(this);
}

//////////////////////////////////////////////////////////////////////////
AABB2 MapRegionType::GetFloorUVs() const
{
    if (m_floorMat) {
        return m_floorMat->GetUVs();
    }

    return AABB2(Vec2::ZERO, Vec2::ONE);
}

//////////////////////////////////////////////////////////////////////////
AABB2 MapRegionType::GetCeilingUVs() const
{
    if (m_ceilingMat) {
        return m_ceilingMat->GetUVs();
    }

    return AABB2(Vec2::ZERO, Vec2::ONE);
}

//////////////////////////////////////////////////////////////////////////
AABB2 MapRegionType::GetSideUVs() const
{
    if (m_sideMat) {
        return m_sideMat->GetUVs();
    }

    return AABB2(Vec2::ZERO, Vec2::ONE);    
}

//////////////////////////////////////////////////////////////////////////
Texture* MapRegionType::GetTexture() const
{
    if (m_isSolid) {
        return m_sideMat->GetTexture();
    }
    else {
        Texture* ceilingTex = m_ceilingMat->GetTexture();
        Texture* floorTex = m_floorMat->GetTexture();
        if (ceilingTex != floorTex) {
            g_theConsole->PrintString(Rgba8::MAGENTA, "ceiling & floor texture not on same sprite");
        }
        return ceilingTex;
    }
}
