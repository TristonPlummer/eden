#include <shaiya/game/world/model/commands/CommandManager.hpp>
#include <shaiya/game/world/model/commands/impl/SpawnItemCommand.hpp>
#include <shaiya/game/world/model/commands/impl/TeleportCommand.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <glog/logging.h>

using namespace shaiya::game;

/**
 * Initialises the command manager.
 */
CommandManager::CommandManager()
{
    registerCommand(std::make_shared<SpawnItemCommand>());
    registerCommand(std::make_shared<TeleportCommand>());
}

/**
 * Attempts to execute a command.
 * @param character The character that is trying to execute a command.
 * @param text      The command text.
 */
void CommandManager::execute(Character& character, const std::string& text) const
{
    using namespace std;
    using namespace boost;

    // Split the string by spaces, unless they are surrounded in quotation marks.
    vector<string> args;
    tokenizer<escaped_list_separator<char>> token(text, escaped_list_separator<char>('\\', ' ', '\"'));
    for (auto itr = token.begin(); itr != token.end(); ++itr)
        args.push_back(*itr);

    // Erase the first argument, as it's our identifier
    auto identifier = (*args.begin()).substr(1);
    args.erase(args.begin());
    algorithm::to_lower(identifier);

    // If a command with the specified identifier exists, execute it.
    if (commands_.contains(identifier))
    {
        auto command = commands_.at(identifier);
        try
        {
            command->execute(character, args);
        }
        catch (const std::exception& e)
        {
            LOG(INFO) << "Command with identifier \"" << identifier << "\" threw an exception: " << e.what();
        }
    }
}

/**
 * Registers a command instance.
 * @param command   The command instance.
 */
void CommandManager::registerCommand(std::shared_ptr<Command> command)
{
    commands_[command->identifier()] = std::move(command);
}