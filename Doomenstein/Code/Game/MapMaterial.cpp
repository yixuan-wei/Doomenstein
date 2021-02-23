#include "Game/MapMaterial.hpp"
#include "Game/GameCommon.hpp"
#include "Game/MaterialSheet.hpp"
#include "Engine/Core/DevConsole.hpp"


std::vector<MapMaterialType*> MapMaterialType::sMapMaterials;

//////////////////////////////////////////////////////////////////////////
void MapMaterialType::InitMapMaterialTypesFromFile(char const* mapMaterialPath)
{
    g_theConsole->PrintString(Rgba8(100,0,255), Stringf("Start parsing %s...", mapMaterialPath));

    XmlDocument mapMatDoc;
    XmlError code = mapMatDoc.LoadFile(mapMaterialPath);
    if(code!=XmlError::XML_SUCCESS){
        g_theConsole->PrintError(Stringf("Fail to load xml %s, code %i", mapMaterialPath, (int)code));
        return;
    }

    XmlElement* root = mapMatDoc.RootElement();
    if (root == nullptr) {
        g_theConsole->PrintError(Stringf("Fail to retrieve root element of %s", mapMaterialPath));
        return;
    }
    if (strcmp(root->Name(), "MapMaterialTypes") != 0) {
        g_theConsole->PrintError(Stringf("Root %s invalid for %s", root->Name(), mapMaterialPath));
        return;
    }

    //read in material sheet
    XmlElement const* child = root->FirstChildElement();
    while (child != nullptr && strcmp(child->Name(),"MaterialsSheet")==0) {
        new MaterialSheet(*child);
        child = child->NextSiblingElement();
    }

    //read in material type
    while (child != nullptr && strcmp(child->Name(), "MaterialType")==0) {
        //TODO add sMapMaterials number check for success or not
        new MapMaterialType(*child);
        child = child->NextSiblingElement();
    }

    if (child != nullptr) {
        g_theConsole->PrintError(Stringf("%s has unrecognized element %s, check format", mapMaterialPath, child->Name()));
    }
}

//////////////////////////////////////////////////////////////////////////
void MapMaterialType::InitRenderAssets()
{
    for (MapMaterialType* type : sMapMaterials) {
        type->m_uvs = type->m_sheet->GetSpriteUVs(type->m_spriteCoords);
    }
}

//////////////////////////////////////////////////////////////////////////
MapMaterialType* MapMaterialType::GetMapMaterialTypeFromName(std::string const& name)
{
    for (size_t i = 0; i < sMapMaterials.size(); i++) {
        MapMaterialType* mat = sMapMaterials[i];
        if (mat->m_name == name) {
            return mat;
        }
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
MapMaterialType::MapMaterialType(XmlElement const& element)
{
    m_name = ParseXmlAttribute(element, "name", "");
    if (m_name.empty()) {
        g_theConsole->PrintError("MaterialType with NULL name");
        return;
    }

    m_spriteCoords = ParseXmlAttribute(element, "spriteCoords", IntVec2(0,0));
    std::string sheetName = ParseXmlAttribute(element, "sheet", "");
    m_sheet = MaterialSheet::GetMaterialSheetFromName(sheetName);
    if (m_sheet==nullptr) {
        g_theConsole->PrintError(Stringf("MaterialType with illegal sheet name: %s", sheetName.c_str()));
        return;
    }

    sMapMaterials.push_back(this);
}

//////////////////////////////////////////////////////////////////////////
Texture* MapMaterialType::GetTexture() const
{
    return m_sheet->GetDiffuseTex();
}
