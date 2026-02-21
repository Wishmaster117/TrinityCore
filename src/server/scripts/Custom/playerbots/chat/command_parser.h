/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOTS_COMMAND_PARSER_H
#define PLAYERBOTS_COMMAND_PARSER_H

#include <string>
#include <vector>

namespace Playerbots
{
    struct Command
    {
        std::string Name;                 // lower-cased
        std::vector<std::string> Args;    // original tokens (lower-cased for convenience)
        std::string Raw;                  // original message (trimmed)
    };

    class CommandParser final
    {
    public:
        // Returns false if message is empty / whitespace only.
        static bool TryParse(std::string const& msg, Command& out);
    };
}

#endif // PLAYERBOTS_COMMAND_PARSER_H