#include <shaiya/common/net/packet/PacketRegistry.hpp>
#include <shaiya/game/net/GameSession.hpp>

#include <glog/logging.h>

using namespace shaiya::net;

/**
 * Handles an incoming remove item request.
 * @param session   The session instance.
 * @param request   The inbound remove item request.
 */
void handleRemoveItem(Session& session, const CharacterRemoveItemRequest& request)
{
    auto& game     = dynamic_cast<GameSession&>(session);
    auto character = game.character();

    auto& inventory = character->inventory();

    auto page = request.page;
    auto slot = request.slot;

    if (page == 0 || page > inventory.pageCount())
        return;
    if (slot >= inventory.pageSize())
        return;
    page--;

    auto item = inventory.at(page, slot);
    if (!item)
        return;

    inventory.remove(page, slot, request.count);
}

/**
 * A template specialization used for registering a remove item handler.
 */
template<>
void PacketRegistry::registerPacketHandler<CharacterRemoveItemRequestOpcode>()
{
    registerHandler<CharacterRemoveItemRequestOpcode, CharacterRemoveItemRequest>(&handleRemoveItem);
}