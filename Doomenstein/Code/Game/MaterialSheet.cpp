#include "Game/MaterialSheet.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Math/AABB2.hpp"

std::vector<MaterialSheet*> MaterialSheet::sMaterialSheets;

//////////////////////////////////////////////////////////////////////////
void MaterialSheet::InitRenderAssets()
{
    for (MaterialSheet* sheet : sMaterialSheets) {
        sheet->m_diffuseTex = g_theRenderer->CreateOrGetTextureFromFile(sheet->m_diffusePath.c_str());
    
        //sprite sheet
        if (sheet->m_diffuseTex == nullptr) {
            g_theConsole->PrintError("fail to load diffuse texture, switch to White");
            sheet->m_diffuseTex = g_theRenderer->CreateOrGetTextureFromFile("White");
        }
        sheet->m_spriteSheet = new SpriteSheet(*sheet->m_diffuseTex, sheet->m_layout);
    }
}

//////////////////////////////////////////////////////////////////////////
MaterialSheet* MaterialSheet::GetMaterialSheetFromName(std::string const& name)
{
    for (size_t i = 0; i < sMaterialSheets.size(); i++) {
        MaterialSheet* sheet = sMaterialSheets[i];
        if (sheet->m_name == name) {
            return sheet;
        }
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
MaterialSheet::MaterialSheet(XmlElement const& element)
{
    m_name = ParseXmlAttribute(element, "name", "");
    if (m_name.empty()) {
        g_theConsole->PrintError("MaterialSheet with NULL name");
        return;
    }

    m_layout = ParseXmlAttribute(element, "layout", IntVec2(1,1));

    XmlElement const* diffuse = element.FirstChildElement("Diffuse");
    if (diffuse == nullptr) {
        g_theConsole->PrintError("MaterialSheet with no diffuse texture, set to white");
        m_diffusePath = "White";
    }
    else {
        m_diffusePath = ParseXmlAttribute(*diffuse, "image", "White");        

        if (diffuse->NextSiblingElement("Diffuse") != nullptr) {
            g_theConsole->PrintError(Stringf("Multiple Diffuse element in sheet %s", m_name.c_str()));
        }
    }    

    MaterialSheet::sMaterialSheets.push_back(this);
}

//////////////////////////////////////////////////////////////////////////
AABB2 MaterialSheet::GetSpriteUVs(IntVec2 const& spriteCoords) const
{
    if (spriteCoords.x < 0 || spriteCoords.x >= m_layout.x || 
        spriteCoords.y < 0 || spriteCoords.y >= m_layout.y) {
        g_theConsole->PrintError(Stringf("Sprite Coords (%i, %i) invalid for MaterialSheet %s",
            spriteCoords.x, spriteCoords.y, m_name.c_str()));
        return AABB2(Vec2::ZERO, Vec2::ONE);
    }

    int index = spriteCoords.y*m_layout.x + spriteCoords.x;
    Vec2 uvMins, uvMaxs;
    m_spriteSheet->GetSpriteUVs(uvMins, uvMaxs, index);
    return AABB2(uvMins, uvMaxs);
}
