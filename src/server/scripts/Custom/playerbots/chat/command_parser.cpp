/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "playerbots/chat/command_parser.h"

#include <sstream>
#include <cctype>

namespace Playerbots
{
    static std::string Trim(std::string const& s)
    {
        size_t b = 0;
        while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
            ++b;

        size_t e = s.size();
        while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
            --e;

        return s.substr(b, e - b);
    }

    static void ToLowerInPlace(std::string& s)
    {
        for (char& c : s)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    bool CommandParser::TryParse(std::string const& msg, Command& out)
    {
        out = Command();
        out.Raw = Trim(msg);
        if (out.Raw.empty())
            return false;

        std::istringstream iss(out.Raw);
        std::string tok;
        while (iss >> tok)
        {
            ToLowerInPlace(tok);
            out.Args.push_back(tok);
        }

        if (out.Args.empty())
            return false;

        out.Name = out.Args.front();

        // ACore-like support: allow optional dot prefix (".follow") without requiring it.
        if (!out.Name.empty() && out.Name[0] == '.')
            out.Name.erase(0, 1);

        return !out.Name.empty();
    }
}