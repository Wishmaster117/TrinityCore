/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ScriptMgr.h"
#include "playerbots/core/playerbots_manager.h"

// R2: GM-only spawn commands (.pb ...)
void AddSC_playerbots_commands();
void AddSC_playerbots_rndbots_autospawn();

 namespace Playerbots
 {
     class PlayerbotsWorldScript final : public WorldScript
     {
     public:
         PlayerbotsWorldScript() : WorldScript("PlayerbotsWorldScript") { }
 
         void OnStartup() override
         {
             Manager::Instance().OnStartup();
         }
 
         void OnUpdate(uint32 diff) override
         {
             Manager::Instance().Update(diff);
         }
     };
 }
 
 void AddSC_playerbots_mvp()
 {
     new Playerbots::PlayerbotsWorldScript();
     AddSC_playerbots_commands();
     AddSC_playerbots_rndbots_autospawn();
 }