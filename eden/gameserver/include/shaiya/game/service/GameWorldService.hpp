#pragma once
#include <shaiya/game/Forward.hpp>
#include <shaiya/game/util/EntityContainer.hpp>
#include <shaiya/game/world/model/commands/CommandManager.hpp>
#include <shaiya/game/world/model/map/MapRepository.hpp>
#include <shaiya/game/world/scheduling/Scheduler.hpp>

#include <boost/property_tree/ptree.hpp>

#include <memory>
#include <mutex>
#include <queue>
#include <vector>

namespace shaiya::game
{
    class GameWorldService
    {
    public:
        /**
         * Initialise this game world service.
         * @param db            The database service.
         * @param itemService   The item definition service.
         * @param worldId       The id of this world service.
         */
        explicit GameWorldService(shaiya::database::DatabaseService& db, ItemDefinitionService& itemService, size_t worldId);

        /**
         * Loads the game world service.
         * @param config    The configuration instance.
         */
        void load(boost::property_tree::ptree& config);

        /**
         * Handles the main tick of the world.
         * @param tickRate  The tick frequency
         */
        void tick(size_t tickRate);

        /**
         * Handles the registration of a character to this game world.
         * @param character The character to register.
         */
        void registerCharacter(std::shared_ptr<Character> character);

        /**
         * Removes a character from this game world.
         * @param character The character to remove.
         */
        void unregisterCharacter(std::shared_ptr<Character> character);

        /**
         * Registers a ground item to this world.
         * @param item  The ground item instance.
         */
        void registerItem(std::shared_ptr<GroundItem> item);

        /**
         * Removes a ground item from this world.
         * @param item  The ground item instance.
         */
        void unregisterItem(std::shared_ptr<GroundItem> item);

        /**
         * Registers an npc to this world.
         * @param item  The npc instance.
         */
        void registerNpc(std::shared_ptr<Npc> item);

        /**
         * Removes an npc from this world.
         * @param item  The npc instance.
         */
        void unregisterNpc(std::shared_ptr<Npc> item);

        /**
         * Finalises the registration of characters that are queued to be registered.
         */
        void finaliseRegistrations();

        /**
         * Finalises the unregistration of characters that are queued to be unregistered.
         */
        void finaliseUnregistrations();

        /**
         * Schedules a task to be executed in the future.
         * @param task  The task.
         */
        void schedule(std::shared_ptr<ScheduledTask> task);

        /**
         * Gets the map repository
         * @return  The map repository
         */
        [[nodiscard]] const MapRepository& maps() const
        {
            return mapRepository_;
        }

        /**
         * Gets the command manager.
         * @return  The command manager.
         */
        [[nodiscard]] const CommandManager& commands() const
        {
            return commandManager_;
        }

        /**
         * Gets the item definition service.
         * @return  The item definition service.
         */
        [[nodiscard]] const ItemDefinitionService& items() const
        {
            return itemDefs_;
        }

    private:
        /**
         * If this service is running.
         */
        bool running_{ true };

        /**
         * The database service.
         */
        shaiya::database::DatabaseService& db_;

        /**
         * The item definition service.
         */
        ItemDefinitionService& itemDefs_;

        /**
         * A vector containing the characters that are connected to this game world.
         */
        std::vector<std::shared_ptr<Character>> characters_;

        /**
         * The characters that are pending registration
         */
        std::queue<std::shared_ptr<Character>> newCharacters_;

        /**
         * The characters that are pending unregistration
         */
        std::queue<std::shared_ptr<Character>> oldCharacters_;

        /**
         * A container that holds all of the ground items that exist in the world.
         */
        EntityContainer<GroundItem> groundItems_;

        /**
         * A container that holds all of the npcs that exist in the world.
         */
        EntityContainer<Npc> npcs_;

        /**
         * The client synchronizer
         */
        std::unique_ptr<ClientSynchronizer> synchronizer_;

        /**
         * The character serializer
         */
        std::unique_ptr<CharacterSerializer> characterSerializer_;

        /**
         * The map repository.
         */
        MapRepository mapRepository_;

        /**
         * The task scheduler
         */
        Scheduler scheduler_;

        /**
         * The command manager.
         */
        CommandManager commandManager_;

        /**
         * The mutex to be used for locking access to the actor vectors.
         */
        std::mutex mutex_;
    };
}