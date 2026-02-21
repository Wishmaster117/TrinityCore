-- Ensure table exists
CREATE TABLE IF NOT EXISTS `playerbots_bots` (
  `guid` BIGINT UNSIGNED NOT NULL,
  `bot_type` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `owner_guid` BIGINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`),
  KEY `idx_owner_guid` (`owner_guid`),
  KEY `idx_bot_type` (`bot_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;