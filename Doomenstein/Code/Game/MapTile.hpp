#pragma once

#include "Engine/Math/IntVec2.hpp"

class MapRegionType;

class MapTile
{
    friend class TileMap;

public:
    MapTile(IntVec2 const& coords, MapRegionType const* type);

private:
    IntVec2 m_tileCoords;
    MapRegionType const* m_type = nullptr;
};