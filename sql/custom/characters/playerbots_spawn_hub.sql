CREATE TABLE IF NOT EXISTS `playerbots_spawn_hubs` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `teamId` TINYINT UNSIGNED NOT NULL,          -- 0 = Alliance, 1 = Horde (TC TeamId)
  `level_min` TINYINT UNSIGNED NOT NULL,
  `level_max` TINYINT UNSIGNED NOT NULL,
  `map` SMALLINT UNSIGNED NOT NULL,
  `x` FLOAT NOT NULL,
  `y` FLOAT NOT NULL,
  `z` FLOAT NOT NULL,
  `o` FLOAT NOT NULL DEFAULT 0,
  `areaId` INT UNSIGNED NOT NULL DEFAULT 0,    -- 0 => calcul auto via Map::GetAreaId
  PRIMARY KEY (`id`),
  KEY `idx_team_level` (`teamId`, `level_min`, `level_max`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ================================================================
-- playerbots_spawn_hubs — Zones de spawn par level range
-- Progression complète The War Within (patch 11.x)
-- 
-- ARCHITECTURE :
--   teamId 0 = Alliance  |  teamId 1 = Horde  (TrinityCore TeamId)
--   level_min / level_max : tranche de niveau inclusive
--   map : mapId TrinityCore (world DB)
--   x, y, z, o : coordonnées monde (o = orientation radians)
--   areaId : 0 = calcul automatique via Map::GetAreaId()
--
-- !! LIGNES (APPROX) : coordonnées estimées, à vérifier in-game
--    avec la commande GM : .gps
--    ou via wago.tools/db2/AreaTable et wowhead.com
--
-- ORDRE DE PRIORITÉ pour le code C++ :
--   1. Trouver toutes les lignes matchant (teamId, level BETWEEN level_min AND level_max)
--   2. Choisir aléatoirement parmi les résultats
-- ================================================================

-- ===========================================================
-- NIVEAU 1-10  : Zones de départ raciales + Exile's Reach
-- ===========================================================
INSERT INTO `playerbots_spawn_hubs`
  (`teamId`,`level_min`,`level_max`,`map`,`x`,`y`,`z`,`o`,`areaId`)
VALUES
  (0,1,10,2175,-484.445007,-2618.379883,4.012000,0.0,0), -- [A] Exile's Reach (tutoriel TWW, toutes races) [Alliance],
  (1,1,10,2175,-484.445007,-2618.379883,4.012000,0.0,0), -- [H] Exile's Reach (tutoriel TWW, toutes races) [Horde],
  (0,1,10,0,-8952.73,-127.96,81.04,0.0,0), -- [A] Northshire Valley – Humain,
  (0,1,10,0,-6240.03,336.88,382.82,0.0,0), -- [A] Coldridge Valley – Nain / Gnome,
  (0,1,10,1,10338.80,832.33,1326.41,0.0,0), -- [A] Shadowglen (Teldrassil) – Elfe de nuit,
  (0,1,10,530,-4044.07,-13863.50,73.46,0.0,0), -- [A] Ammen Vale (Azuremyst Isle) – Draenei,
  (0,1,10,654,-1305.60,227.50,117.50,0.0,0), -- [A] Gilnéas – Worgen,
  (1,1,10,1,-618.54,-4330.72,38.72,0.0,0), -- [H] Vallée des Épreuves – Orc / Troll,
  (1,1,10,0,1676.71,1669.85,96.82,0.0,0), -- [H] Deathknell, Tirisfal – Mort-vivant,
  (1,1,10,1,-2917.58,-258.41,115.18,0.0,0), -- [H] Camp Narache, Mulgore – Tauren,
  (1,1,10,530,10348.707031,-6357.927734,33.484032,0.0,0), -- [H] Sunstrider Isle, Eversong – Elfe de sang,
  (1,1,10,648,-8390.339844,1342.359985,101.990997,0.0,0), -- [H] Kezan – Gobelin (APPROX);

-- =============================================
-- NIVEAU 10-20 : Classic – zones bas niveau
-- =============================================
  (0,10,20,0,-9468.86,63.37,56.23,0.0,0), -- [A] Goldshire – Forêt d'Elwynn,
  (0,10,20,0,-11109.01,1569.49,52.13,0.0,0), -- [A] Sentinel Hill – Terres de l'Ouest,
  (0,10,20,0,-5728.25,-497.26,393.72,0.0,0), -- [A] Thelsamar – Loch Modan,
  (0,10,20,1,7388.00,1186.00,7.00,0.0,0), -- [A] Lor'danel – Rivage obscur (post-Cata),
  (0,10,20,530,-5148.50,-14061.30,0.00,0.0,0), -- [A] Blood Watch – Île de Sang (suivi Draenei),
  (1,10,20,1,341.02,-4683.10,17.87,0.0,0), -- [H] Razor Hill – Durotar,
  (1,10,20,1,-441.00,-2596.00,96.10,0.0,0), -- [H] Les Croisées – Les Tarides,
  (1,10,20,0,-424.00,1622.00,59.00,0.0,0), -- [H] La Sépulture – Forêt des Pins argentés,
  (1,10,20,1,-2905.00,-384.00,91.00,0.0,0), -- [H] Bloodhoof Village – Mulgore,
  (1,10,20,530,9959.50,-7190.20,22.78,0.0,0), -- [H] Falconwing Square – Bois des Chants éternels (BE);

-- =============================================
-- NIVEAU 20-30 : Classic – zones milieu bas
-- =============================================
  (0,20,30,0,-9384.96,-2254.36,64.45,0.0,0), -- [A] Lakeshire – Crêtes de Feux-Rouges,
  (0,20,30,0,-10467.40,-1194.81,35.59,0.0,0), -- [A] Darkshire – Bois des Ombres,
  (0,20,30,0,-3781.53,-804.17,10.69,0.0,0), -- [A] Port de Ménéthil – Les Paluns,
  (0,20,30,1,2485.66,-3490.45,67.48,0.0,0), -- [A] Astranaar – Ashenvale,
  (1,20,30,0,-739.00,-720.00,53.00,0.0,0), -- [H] Moulin de Tarren – Hautes-terres de Khaz Modan,
  (1,20,30,1,2092.00,-2038.00,92.00,0.0,0), -- [H] Camp Bûcheron – Ashenvale,
  (1,20,30,0,1400.00,-296.00,52.00,0.0,0), -- [H] Brill – Tirisfal,
  (1,20,30,530,10172.00,-6541.00,30.00,0.0,0), -- [H] Tranquillien – Terres fantômes (BE suivi);

-- ==============================================
-- NIVEAU 30-40 : Classic – zones milieu haut
-- ==============================================
  (0,30,40,0,-14332.07,537.95,23.95,0.0,0), -- [A] Baie-du-Butin – Strangleronce (STV),
  (0,30,40,0,-2031.23,-1832.14,104.21,0.0,0), -- [A] Point de Refuge – Hautes-terres d'Arathi,
  (0,30,40,1,-866.00,-1270.00,168.81,0.0,0), -- [A] Nijel's Point – Désolace,
  (0,30,40,1,1639.00,-609.00,101.55,0.0,0), -- [A] Talonbranch – Contreforts de Hautebrande,
  (1,30,40,0,-13194.67,241.37,7.27,0.0,0), -- [H] Camp Grom'gol – Strangleronce (STV),
  (1,30,40,0,-1214.00,-2071.00,95.00,0.0,0), -- [H] Hammerfall – Hautes-terres d'Arathi,
  (1,30,40,1,-3367.00,-659.00,64.00,0.0,0), -- [H] Portombre – Désolace,
  (1,30,40,1,1116.00,-1612.00,48.00,0.0,0), -- [H] Camp Taurajo – Les Tarides du Sud;

-- ==============================================
-- NIVEAU 40-50 : Classic – zones haut niveau
-- ==============================================
  (0,40,50,0,207.24,-1876.57,129.88,0.0,0), -- [A] Pic de l'Aire – Les Hinterlands,
  (0,40,50,1,-7122.64,-3829.82,8.11,0.0,0), -- [A] Gadgetzan – Tanaris,
  (0,40,50,1,-4629.00,1166.00,0.00,0.0,0), -- [A] Forteresse Plumedelune – Féralas (APPROX),
  (0,40,50,1,-2620.00,-2657.00,92.00,0.0,0), -- [A] Marécage de la Tristesse – Theramore (APPROX),
  (1,40,50,1,-4552.00,674.00,73.00,0.0,0), -- [H] Camp Mojache – Féralas,
  (1,40,50,1,-4786.00,-1579.00,81.00,0.0,0), -- [H] Freewind Post – Mille Pointes,
  (1,40,50,1,-7122.64,-3829.82,8.11,0.0,0), -- [H] Gadgetzan – Tanaris,
  (1,40,50,0,761.00,-1175.00,16.00,0.0,0), -- [H] Camp Aiguilles-Sanglantes – Hinterlands (APPROX);

-- ==============================================================
-- NIVEAU 50-60 : Classic end-game / Shadowlands Chromie Time
-- ==============================================================
  (0,50,60,0,2978.54,-3440.73,160.47,0.0,0), -- [A] Chapelle des Derniers Espoirs – Terres de la Peste E.,
  (0,50,60,0,-7575.53,-1072.15,265.36,0.0,0), -- [A] Veille de Morgan – Steppes ardentes,
  (0,50,60,0,-9316.00,-2286.00,43.00,0.0,0), -- [A] Chillwind Camp – Terres de la Peste O. (APPROX),
  (0,50,60,1,-6983.00,817.00,47.00,0.0,0), -- [A] Poste de Cénarius – Silithus,
  (1,50,60,0,3289.00,-3387.00,175.00,0.0,0), -- [H] Zone Forsaken – Terres de la Peste Est (APPROX),
  (1,50,60,0,1101.00,-1745.00,63.00,0.0,0), -- [H] Zone Forsaken – Terres de la Peste Ouest (APPROX),
  (1,50,60,0,-7328.00,-1049.00,250.00,0.0,0), -- [H] Kargath – Badlands,
  (1,50,60,1,-6983.00,817.00,47.00,0.0,0), -- [H] Poste de Cénarius – Silithus,
  (1,50,60,1,5126.00,-820.00,449.00,0.0,0), -- [H] Sanctuaire d'Émeraude – Gangrebois,
  (0,50,60,2222,-723.00,-1117.00,249.00,0.0,0), -- [A] Oribos – Hub Shadowlands (APPROX) [Alliance],
  (1,50,60,2222,-723.00,-1117.00,249.00,0.0,0), -- [H] Oribos – Hub Shadowlands (APPROX) [Horde],
  (0,50,60,2112,-3892.00,-3384.00,156.00,0.0,0), -- [A] Bastion – Elysian Hold (APPROX),
  (1,50,60,2113,1230.00,-1940.00,122.00,0.0,0), -- [H] Maldraxxus – Seat of the Primus (APPROX),
  (0,50,60,2133,2044.00,-2540.00,303.00,0.0,0), -- [A] Ardenweald – Heart of the Forest (APPROX) [Alliance],
  (1,50,60,2133,2044.00,-2540.00,303.00,0.0,0), -- [H] Ardenweald – Heart of the Forest (APPROX) [Horde],
  (0,50,60,2170,-3190.00,-170.00,249.00,0.0,0), -- [A] Revendreth – Sinfall (APPROX) [Alliance],
  (1,50,60,2170,-3190.00,-170.00,249.00,0.0,0), -- [H] Revendreth – Sinfall (APPROX) [Horde];

-- ==========================================================
-- NIVEAU 58-68 : Outland – The Burning Crusade (MAP 530)
-- ==========================================================
  (0,58,68,530,-282.04,1019.75,81.12,0.0,0), -- [A] Honor Hold – Péninsule des Flammes infernales,
  (0,58,68,530,-1753.00,5434.00,13.00,0.0,0), -- [A] Telredor – Marais de Zangar,
  (0,58,68,530,-3795.30,4902.10,25.67,0.0,0), -- [A] Allerian Stronghold – Forêt de Terokkar,
  (0,58,68,530,-1126.00,7558.00,6.00,0.0,0), -- [A] Telaar – Nagrand,
  (0,58,68,530,1979.00,5756.00,271.00,0.0,0), -- [A] Sylvanaar – Haut-Royaume (APPROX),
  (0,58,68,530,3309.00,1316.00,204.00,0.0,0), -- [A] Area 52 – Nécrolande de Farahlon (APPROX),
  (1,58,68,530,318.63,3695.48,111.68,0.0,0), -- [H] Thrallmar – Péninsule des Flammes infernales,
  (1,58,68,530,-220.00,6881.00,7.00,0.0,0), -- [H] Zabra'jin – Marais de Zangar,
  (1,58,68,530,-3517.00,4680.00,7.00,0.0,0), -- [H] Stonebreaker Hold – Forêt de Terokkar (APPROX),
  (1,58,68,530,-1064.00,7718.00,10.00,0.0,0), -- [H] Garadar – Nagrand (APPROX),
  (1,58,68,530,2060.00,4975.00,258.00,0.0,0), -- [H] Thunderlord Stronghold – Haut-Royaume (APPROX);

-- ===============================================
-- NIVEAU 60-68 : Outland – Shattrath (neutre)
-- ===============================================
  (0,60,68,530,-1957.80,5568.34,30.54,0.0,0), -- [A] Shattrath City – Outland (neutre) [Alliance],
  (1,60,68,530,-1957.80,5568.34,30.54,0.0,0), -- [H] Shattrath City – Outland (neutre) [Horde];

-- ===============================================================
-- NIVEAU 68-80 : Northrend – Wrath of the Lich King (MAP 571)
-- ===============================================================
  (0,68,80,571,-787.00,-4501.00,51.00,0.0,0), -- [A] Valgarde – Fjord Hurlant,
  (0,68,80,571,5765.36,5268.40,-0.98,0.0,0), -- [A] Forteresse Valeureux – Toundra Boréenne,
  (0,68,80,571,3728.39,-3843.95,174.63,0.0,0), -- [A] Bastion de Fordragon – Désolation des Dragons,
  (0,68,80,571,3143.00,-645.00,204.00,0.0,0), -- [A] Amberpine Lodge – Collines Grizzly (APPROX),
  (0,68,80,571,625.00,-30.00,446.00,0.0,0), -- [A] Camp de Storm Peaks Alliance (APPROX),
  (1,68,80,571,2897.00,6089.00,118.00,0.0,0), -- [H] Warsong Hold – Toundra Boréenne,
  (1,68,80,571,603.00,-4958.00,8.00,0.0,0), -- [H] New Agamand – Fjord Hurlant (APPROX),
  (1,68,80,571,3556.00,-4280.00,174.00,0.0,0), -- [H] Camp de la Guerre Éternelle – Désolation des Dragons (APPROX),
  (1,68,80,571,4208.00,-773.00,191.00,0.0,0), -- [H] Camp Oneqwah – Collines Grizzly (APPROX),
  (1,68,80,571,394.00,-500.00,440.00,0.0,0), -- [H] Zim'Torga – Zuldrak (APPROX),
  (0,68,80,571,5804.87,626.38,660.01,0.0,0), -- [A] Dalaran (Northrend) – neutre [Alliance],
  (1,68,80,571,5804.87,626.38,660.01,0.0,0), -- [H] Dalaran (Northrend) – neutre [Horde];

-- ============================
-- NIVEAU 80-85 : Cataclysm
-- ============================
  (0,80,85,0,4028.94,-4609.20,985.17,0.0,0), -- [A] Nordrassil – Mont Hyjal (APPROX) [Alliance],
  (1,80,85,0,4028.94,-4609.20,985.17,0.0,0), -- [H] Nordrassil – Mont Hyjal (APPROX) [Horde],
  (0,80,85,1,-8386.00,-2090.00,14.00,0.0,0), -- [A] Ramkahen – Uldum [Alliance],
  (1,80,85,1,-8386.00,-2090.00,14.00,0.0,0), -- [H] Ramkahen – Uldum [Horde],
  (0,80,85,0,-4512.00,-2710.00,37.00,0.0,0), -- [A] Highbank – Hautes-terres du Crépuscule (APPROX),
  (1,80,85,0,-5006.00,-3037.00,37.00,0.0,0), -- [H] Port des Seigneurs-Dragons – Crépuscule (APPROX);

-- =========================================================
-- NIVEAU 85-90 : Pandarie – Mists of Pandaria (MAP 870)
-- =========================================================
  (0,85,90,870,1380.00,820.00,161.00,0.0,0), -- [A] Village de Patte-Rieuse – Forêt de Jade (APPROX),
  (1,85,90,870,1590.00,980.00,165.00,0.0,0), -- [H] Jade Mine Hub Horde – Forêt de Jade (APPROX),
  (0,85,90,870,1126.00,-1630.00,439.00,0.0,0), -- [A] Pagode d'Or – Val des Quatre-Vents (APPROX) [Alliance],
  (1,85,90,870,1126.00,-1630.00,439.00,0.0,0), -- [H] Pagode d'Or – Val des Quatre-Vents (APPROX) [Horde],
  (0,85,90,870,-2534.00,-1020.00,87.00,0.0,0), -- [A] Lion's Landing – Île des Tonnerre (APPROX),
  (1,85,90,870,2250.00,-1500.00,35.00,0.0,0), -- [H] Domaine de Domination – Île des Tonnerre (APPROX);

-- ===========================================================
-- NIVEAU 90-100: Draenor – Warlords of Draenor (MAP 1116)
-- ===========================================================
  (0,90,100,1116,1832.91,1273.61,81.38,0.0,0), -- [A] Lunarfall – Vallée de la Lune fantôme (garrison Alliance),
  (0,90,100,1116,1760.00,-400.00,166.00,0.0,0), -- [A] Shattrath Talador – Hub Alliance (APPROX),
  (0,90,100,1116,2527.00,-4065.00,136.00,0.0,0), -- [A] Nagrand Draenor – Hub Alliance (APPROX),
  (1,90,100,1116,5180.00,-3492.00,185.00,0.0,0), -- [H] Frostwall – Crêtes Givresang (garrison Horde, APPROX),
  (1,90,100,1116,1510.00,-390.00,155.00,0.0,0), -- [H] Talador Hub Horde (APPROX),
  (1,90,100,1116,2527.00,-4065.00,136.00,0.0,0), -- [H] Nagrand Draenor – Hub Horde (APPROX);

-- ====================================================
-- NIVEAU 100-110: Îles Brisées – Legion (MAP 1220)
-- ====================================================
  (0,100,110,1220,-886.00,4173.00,28.00,0.0,0), -- [A] Azsuna – Îles Brisées (APPROX) [Alliance],
  (1,100,110,1220,-886.00,4173.00,28.00,0.0,0), -- [H] Azsuna – Îles Brisées (APPROX) [Horde],
  (0,100,110,1220,1078.00,2699.00,9.00,0.0,0), -- [A] Stormheim – Îles Brisées (APPROX) [Alliance],
  (1,100,110,1220,1078.00,2699.00,9.00,0.0,0), -- [H] Stormheim – Îles Brisées (APPROX) [Horde],
  (0,100,110,1220,-3023.00,2145.00,167.00,0.0,0), -- [A] Val'sharah – Îles Brisées (APPROX) [Alliance],
  (1,100,110,1220,-3023.00,2145.00,167.00,0.0,0), -- [H] Val'sharah – Îles Brisées (APPROX) [Horde],
  (0,100,110,1220,-3523.00,-2115.00,411.00,0.0,0), -- [A] Highmountain – Îles Brisées (APPROX) [Alliance],
  (1,100,110,1220,-3523.00,-2115.00,411.00,0.0,0), -- [H] Highmountain – Îles Brisées (APPROX) [Horde],
  (0,100,110,1220,-5044.00,6384.00,21.00,0.0,0), -- [A] Suramar – Îles Brisées (APPROX) [Alliance],
  (1,100,110,1220,-5044.00,6384.00,21.00,0.0,0), -- [H] Suramar – Îles Brisées (APPROX) [Horde],
  (0,100,110,1220,-5452.00,6226.00,740.00,0.0,0), -- [A] Dalaran flottant – Îles Brisées (APPROX, neutre) [Alliance],
  (1,100,110,1220,-5452.00,6226.00,740.00,0.0,0), -- [H] Dalaran flottant – Îles Brisées (APPROX, neutre) [Horde];

-- ======================================================
-- NIVEAU 110-120: Battle for Azeroth (MAP 1642/1643)
-- ======================================================
  (0,110,120,1643,-734.00,-2174.00,23.00,0.0,0), -- [A] Boralus – Kul Tiras (APPROX),
  (0,110,120,1643,1043.00,-836.00,33.00,0.0,0), -- [A] Baie de Tiragarde – Kul Tiras (APPROX),
  (0,110,120,1643,-1800.00,-790.00,134.00,0.0,0), -- [A] Drustvar – Kul Tiras (APPROX),
  (0,110,120,1643,810.00,1060.00,97.00,0.0,0), -- [A] Vallée de Stormsong – Kul Tiras (APPROX),
  (1,110,120,1642,-975.00,1478.00,105.00,0.0,0), -- [H] Dazar'alor – Zuldazar (APPROX),
  (1,110,120,1642,-1235.00,-3482.00,54.00,0.0,0), -- [H] Vol'dun – Zandalar (APPROX),
  (1,110,120,1642,988.00,-3011.00,1.00,0.0,0), -- [H] Nazmir – Zandalar (APPROX);

-- ========================================================================
-- NIVEAU 60-70 : Dragonflight – Dragon Isles (MAP 2444/2492/2494/2495)
-- ========================================================================
  (0,60,70,2444,-1164.00,-2548.00,191.00,0.0,0), -- [A] Rivages Éveillés – Dragon Isles (APPROX) [Alliance],
  (1,60,70,2444,-1164.00,-2548.00,191.00,0.0,0), -- [H] Rivages Éveillés – Dragon Isles (APPROX) [Horde],
  (0,60,70,2492,1540.00,-2720.00,138.00,0.0,0), -- [A] Plaines Ohn'ahran – Dragon Isles (APPROX) [Alliance],
  (1,60,70,2492,1540.00,-2720.00,138.00,0.0,0), -- [H] Plaines Ohn'ahran – Dragon Isles (APPROX) [Horde],
  (0,60,70,2494,-3856.00,-2104.00,306.00,0.0,0), -- [A] Étendue d'Azur – Dragon Isles (APPROX) [Alliance],
  (1,60,70,2494,-3856.00,-2104.00,306.00,0.0,0), -- [H] Étendue d'Azur – Dragon Isles (APPROX) [Horde],
  (0,60,70,2495,890.00,890.00,330.00,0.0,0), -- [A] Valdrakken – Thaldraszus, capitale DF (APPROX) [Alliance],
  (1,60,70,2495,890.00,890.00,330.00,0.0,0), -- [H] Valdrakken – Thaldraszus, capitale DF (APPROX) [Horde];

-- ========================================================================
-- NIVEAU 70-80 : The War Within – Khaz Algar (MAP 2552/2553/2054/2248)
-- ========================================================================
  (0,70,80,2552,-3283.00,-1897.00,243.00,0.0,0), -- [A] Dornogal – Île de Dorn, capitale TWW (APPROX) [Alliance],
  (1,70,80,2552,-3283.00,-1897.00,243.00,0.0,0), -- [H] Dornogal – Île de Dorn, capitale TWW (APPROX) [Horde],
  (0,70,80,2552,-890.00,-3640.00,131.00,0.0,0), -- [A] Île de Dorn – Côte Méridionale (APPROX) [Alliance],
  (1,70,80,2552,-890.00,-3640.00,131.00,0.0,0), -- [H] Île de Dorn – Côte Méridionale (APPROX) [Horde],
  (0,70,80,2553,492.00,-2186.00,-384.00,0.0,0), -- [A] Les Profondeurs Sonnantes – Hub (APPROX) [Alliance],
  (1,70,80,2553,492.00,-2186.00,-384.00,0.0,0), -- [H] Les Profondeurs Sonnantes – Hub (APPROX) [Horde],
  (0,70,80,2054,2045.00,-1473.00,-384.00,0.0,0), -- [A] Hallowfall – Luminesence centrale (APPROX) [Alliance],
  (1,70,80,2054,2045.00,-1473.00,-384.00,0.0,0), -- [H] Hallowfall – Luminesence centrale (APPROX) [Horde],
  (0,70,80,2248,4093.00,-2748.00,-900.00,0.0,0), -- [A] Azj-Kahet – Cité des Aragnides (APPROX) [Alliance],
  (1,70,80,2248,4093.00,-2748.00,-900.00,0.0,0), -- [H] Azj-Kahet – Cité des Aragnides (APPROX) [Horde];

-- ===============================================
-- CAPITALES : Hubs permanents toutes tranches
-- ===============================================
  (0,10,80,0,-8827.46,625.22,94.13,0.0,0), -- [A] Hurlevent – Capitale Alliance,
  (0,10,80,0,-4981.25,-882.12,501.66,0.0,0), -- [A] Forgefer – Capitale Nain/Gnome,
  (0,10,80,1,9952.42,2280.35,1341.38,0.0,0), -- [A] Darnassus – Capitale Elfe de nuit,
  (0,10,80,530,-3961.64,-11918.32,-0.02,0.0,0), -- [A] L'Exodar – Capitale Draenei,
  (1,10,80,1,1569.59,-4416.44,21.69,0.0,0), -- [H] Orgrimmar – Capitale Horde,
  (1,10,80,1,-1282.73,122.59,131.19,0.0,0), -- [H] Piton-du-Tonnerre – Capitale Tauren,
  (1,10,80,0,1633.16,239.48,-43.00,0.0,0), -- [H] Fossoyeuse – Capitale Mort-vivant,
  (1,10,80,530,9473.40,-7279.20,14.22,0.0,0); -- [H] Lune-d'Argent – Capitale Elfe de sang;