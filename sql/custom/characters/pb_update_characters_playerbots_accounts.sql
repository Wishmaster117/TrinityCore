-- Playerbots account pool table (characters DB)
-- account_type: 0=unassigned, 1=rndbot, 2=addclass

CREATE TABLE IF NOT EXISTS `playerbots_accounts` (
  `account_id` INT UNSIGNED NOT NULL,
  `account_index` INT UNSIGNED NOT NULL DEFAULT 0,
  `account_type` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`account_id`),
  KEY `idx_account_index` (`account_index`),
  KEY `idx_account_type` (`account_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;