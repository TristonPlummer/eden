#include <shaiya/common/util/Json.hpp>
#include <shaiya/game/model/actor/npc/Npc.hpp>
#include <shaiya/game/model/actor/npc/NpcDefinition.hpp>
#include <shaiya/game/model/actor/player/Player.hpp>
#include <shaiya/game/model/map/Map.hpp>
#include <shaiya/game/model/map/MapCell.hpp>
#include <shaiya/game/service/GameWorldService.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <glog/logging.h>

#include <array>
#include <cassert>
#include <cfenv>
#include <cmath>
#include <iostream>

using namespace shaiya::game;

/**
 * The size of a cell (16x16).
 */
constexpr auto CELL_SIZE = 16;

/**
 * The observable radius from a center cell.
 */
constexpr auto OBSERVABLE_CELL_RADIUS = 3;

/**
 * Initialises this map.
 * @param world The world instance.
 */
Map::Map(GameWorldService& world): world_(world)
{
}

/**
 * Loads this map by populating the cells, and parsing the heightmap and objects.
 * @param stream    The input stream.
 */
void Map::load(std::ifstream& stream)
{
    using boost::property_tree::ptree;
    using boost::property_tree::read_json;

    // Read the stream as a JSON file
    ptree tree;
    read_json(stream, tree);

    // Read the id, and the size
    id_   = tree.get<uint16_t>("id");
    size_ = tree.get<size_t>("size");

    // Calculate the cell rows and columns
    // Cells always fit perfectly into a map, and map sizes are only ever 1024x1024 or 2048x2048.
    rowCount_    = size_ / CELL_SIZE;
    columnCount_ = rowCount_;

    // The total number of cells
    auto totalCells = rowCount_ * columnCount_;
    cells_.resize(totalCells);

    // Generate the cells
    for (auto i = 0; i < totalCells; i++)
        cells_.at(i) = std::make_shared<MapCell>();
}

/**
 * Loads an initial npc spawn for this map.
 * @param stream    The input stream.
 */
void Map::loadNpc(std::ifstream& stream)
{
    using namespace nlohmann;

    struct NpcSpawn
    {
        uint8_t type;
        uint8_t typeId;
        // std::vector<Position> position;
    };

    auto json = json::parse(stream);
    for (const auto& spawn: *json.find("npcs"))
    {
        uint8_t type   = spawn["type"];
        uint8_t typeId = spawn["typeId"];

        for (const auto& position: *spawn.find("positions"))
        {
            float x = position["x"];
            float y = position["y"];
            float z = position["z"];

            auto pos = Position(id_, x, y, z);

            auto* def   = new NpcDefinition();
            def->type   = type;
            def->typeId = typeId;

            auto npc = std::make_shared<Npc>(*def, world_);
            npc->setPosition(pos);

            world_.registerNpc(std::move(npc));
        }
    }
}

/**
 * Adds an entity to this map.
 * @param entity    The entity to add.
 */
void Map::add(std::shared_ptr<Entity> entity) const
{
    // Adjust the position where needed
    adjustPosition(entity->position());

    // Get the cell to place this entity into.
    auto cell = getCell(entity->position());
    assert(cell);
    cell->addEntity(entity);
}

/**
 * Removes an entity from this map.
 * @param entity    The entity to remove.
 */
void Map::remove(std::shared_ptr<Entity> entity) const
{
    // Adjust the position where needed
    adjustPosition(entity->position());

    // Get the cell to remove this entity from
    auto cell = getCell(entity->position());
    assert(cell);
    cell->removeEntity(entity);
}

/**
 * Attempts to get an entity with a specified id and type.
 * @param pos   The position to start the search at.
 * @param id    The entity id.
 * @param type  The entity type to search for.
 * @return      The entity instance.
 */
std::shared_ptr<Entity> Map::get(Position& pos, size_t id, EntityType type)
{
    auto neighbours = getNeighbouringCells(pos);
    for (auto&& cell: neighbours)
    {
        for (auto&& entity: cell->entities())
        {
            if (entity->type() != type)
                continue;
            if (entity->id() == id)
                return entity;
        }
    }
    return nullptr;
}

/**
 * Get a cell in the map based on a position.
 * @param position  The position.
 * @return          The map cell.
 */
std::shared_ptr<MapCell> Map::getCell(Position& position) const
{
    // Adjust the position where needed
    adjustPosition(position);
    return cells_.at(getCellIndex(position));
}

/**
 * Get a cell index  based on a position.
 * @param position  The position.
 * @return          The map cell.
 */
size_t Map::getCellIndex(Position& position) const
{
    // Adjust the position where needed
    adjustPosition(position);

    // Set the rounding mode
    std::fesetround(FE_TOWARDZERO);

    // The x and z coordinates
    auto x = std::min(static_cast<size_t>(std::nearbyint(position.x())), size_);
    auto z = std::min(static_cast<size_t>(std::nearbyint(position.z())), size_);

    // The row and column
    auto row    = x / CELL_SIZE;
    auto column = z / CELL_SIZE;
    return row + (column * rowCount_);
}

/**
 * Gets the cells in a neighbouring radius of a position.
 * @param position  The position.
 * @return          The neighbouring cells.
 */
std::vector<std::shared_ptr<MapCell>> Map::getNeighbouringCells(Position& position) const
{
    // Adjust the position where needed
    adjustPosition(position);

    // The vector of cells
    std::vector<std::shared_ptr<MapCell>> cells;

    // The x and z coordinates.
    auto x = std::min(static_cast<size_t>(std::nearbyint(position.x())), size_);
    auto z = std::min(static_cast<size_t>(std::nearbyint(position.z())), size_);

    // The position's row and column
    auto centerRow    = x / CELL_SIZE;
    auto centerColumn = z / CELL_SIZE;

    // The maximum row and column
    auto maxRow    = centerRow + OBSERVABLE_CELL_RADIUS;
    auto maxColumn = centerColumn + OBSERVABLE_CELL_RADIUS;

    // Loop through the cell rows and columns
    for (auto row = centerRow - OBSERVABLE_CELL_RADIUS; row < maxRow; row++)
    {
        for (auto column = centerColumn - OBSERVABLE_CELL_RADIUS; column < maxColumn; column++)
        {
            auto id = row + (column * rowCount_);
            if (id >= 0 && id < cells_.size())
            {
                cells.push_back(cells_.at(id));
            }
        }
    }
    return cells;
}

/**
 * Adjusts a position to fit into the boundaries of this map.
 * @param position  The position to adjust.
 */
void Map::adjustPosition(Position& position) const
{
    // If the coordinates are less than 0, adjust them to 0.
    if (position.x() < 0)
        position.setX(0);
    if (position.y() < 0)
        position.setY(0);
    if (position.z() < 0)
        position.setZ(0);

    // If the coordinates are greater than or equal to the map size, adjust them to the map size - 1
    if (position.x() >= size_)
        position.setX(size_ - 1);
    if (position.z() >= size_)
        position.setZ(size_ - 1);
}