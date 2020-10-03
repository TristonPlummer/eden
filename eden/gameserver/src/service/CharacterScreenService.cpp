#include <shaiya/game/net/GameSession.hpp>

using namespace shaiya;
using namespace shaiya::game;
using namespace shaiya::net;

/**
 * The number of bytes in an empty character list entry (opcode = 2, slot = 1, id = 4)
 */
constexpr auto EMPTY_CHARCTER_LENGTH = 3;

/**
 * The name of the query for fetching the faction for an account
 */
constexpr auto FETCH_ACCOUNT_FACTION = "fetch_account_faction";

/**
 * The name of the query for updating the faction of an account.
 */
constexpr auto UPDATE_ACCOUNT_FACTION = "update_account_faction";

/**
 * Initialises the character screen service.
 * @param db        The database service to use.
 * @param worldId   The id of this world server.
 */
CharacterScreenService::CharacterScreenService(shaiya::database::DatabaseService& db, uint32_t worldId)
    : db_(db), worldId_(worldId)
{
    db.prepare(FETCH_ACCOUNT_FACTION, "SELECT faction FROM gamedata.factions WHERE userid = $1 and world = $2");
    db.prepare(UPDATE_ACCOUNT_FACTION,
               "INSERT INTO gamedata.factions (world, userid, faction) VALUES ($1, $2, $3) ON CONFLICT (userid) DO UPDATE "
               "SET faction = $3");
}

/**
 * Displays the character screen for a session.
 * @param session   The session.
 */
void CharacterScreenService::display(GameSession& session)
{
    // The player's faction
    auto faction = getFaction(session);

    // Send the player their faction
    AccountFactionNotify factionNotify;
    factionNotify.faction = faction;
    session.write(factionNotify);

    // If the faction is neither Light nor Fury, don't send the character list
    if (faction == ShaiyaFaction::Neither)
        return;

    // Send the empty character list
    for (auto i = 0; i < 5; i++)
    {
        CharacterListEntry entry;
        entry.slot = i;
        session.write(entry, EMPTY_CHARCTER_LENGTH);
    }
}

/**
 * Gets the faction for a given session.
 * @param session   The session instance.
 * @return          The session's faction.
 */
ShaiyaFaction CharacterScreenService::getFaction(shaiya::net::GameSession& session)
{
    try
    {
        // Create a new connection to the database
        auto connection = db_.connection();
        pqxx::work tx(*connection);

        // Get the faction for the user on this server
        auto rows = tx.exec_prepared(FETCH_ACCOUNT_FACTION, session.userId(), worldId_);

        // If no faction was found, return a notification for the user to select their faction
        if (rows.empty())
            return ShaiyaFaction::Neither;

        // Get the first row
        auto row = rows.front();

        // Set the faction id
        auto faction = row["faction"].as<int>();
        return static_cast<ShaiyaFaction>(faction);
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "Exception occurred while fetching faction for user id " << session.userId() << " from ip address "
                   << session.remoteAddress() << ": " << e.what();
        return ShaiyaFaction::Neither;
    }
}

/**
 * Sets the faction for a session.
 * @param session   The session.
 * @param faction   The selected faction.
 */
bool CharacterScreenService::setFaction(shaiya::net::GameSession& session, ShaiyaFaction faction)
{
    // The faction may only be Light or Fury
    if (faction != ShaiyaFaction::Fury && faction != ShaiyaFaction::Light)
        return false;
    try
    {
        // Create a new connection to the database
        auto connection = db_.connection();
        pqxx::work tx(*connection);

        // Execute the update
        tx.exec_prepared(UPDATE_ACCOUNT_FACTION, worldId_, session.userId(), (int)faction);
        tx.commit();
        return true;
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "Exception occurred while setting faction to " << (int)faction << " for session with user id "
                   << session.userId() << " from ip address " << session.remoteAddress() << ": " << e.what();
        return false;
    }
}