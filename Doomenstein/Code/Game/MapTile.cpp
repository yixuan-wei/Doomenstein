#include "Game/MapTile.hpp"
#include "Game/MapRegion.hpp"

//////////////////////////////////////////////////////////////////////////
MapTile::MapTile(IntVec2 const& coords, MapRegionType const* type)
    : m_tileCoords(coords)
    , m_type(type)
{

}
