-- ChatServer social-table migration (idempotent)
-- Target tables:
--   friend_apply: friend request records
--   friend:       friend relationship records
--
-- Usage:
--   USE <your_schema>;   -- e.g. USE cjh;
--   source mysql_social_migration.sql;

SET @schema_name := DATABASE();

-- =========================
-- 1) friend_apply
-- =========================
CREATE TABLE IF NOT EXISTS `friend_apply` (
  `id` BIGINT NOT NULL AUTO_INCREMENT,
  `from_id` INT NOT NULL,
  `from_uid` INT NOT NULL,
  `to_uid` INT NOT NULL,
  `status` TINYINT NOT NULL DEFAULT 0,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_friend_apply_from_to` (`from_id`, `to_uid`),
  KEY `idx_friend_apply_to_uid_id` (`to_uid`, `id`),
  KEY `idx_friend_apply_from_uid_to_uid` (`from_uid`, `to_uid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Backfill old rows if one side is empty
UPDATE `friend_apply`
SET `from_uid` = `from_id`
WHERE `from_uid` IS NULL OR `from_uid` = 0;

UPDATE `friend_apply`
SET `from_id` = `from_uid`
WHERE `from_id` IS NULL OR `from_id` = 0;

-- Keep from_id <-> from_uid consistent, to be compatible with mixed SQL in code.
DROP TRIGGER IF EXISTS `trg_friend_apply_bi_sync_from`;
DROP TRIGGER IF EXISTS `trg_friend_apply_bu_sync_from`;

DELIMITER //
CREATE TRIGGER `trg_friend_apply_bi_sync_from`
BEFORE INSERT ON `friend_apply`
FOR EACH ROW
BEGIN
  IF (NEW.`from_uid` IS NULL OR NEW.`from_uid` = 0) AND (NEW.`from_id` IS NOT NULL AND NEW.`from_id` <> 0) THEN
    SET NEW.`from_uid` = NEW.`from_id`;
  END IF;
  IF (NEW.`from_id` IS NULL OR NEW.`from_id` = 0) AND (NEW.`from_uid` IS NOT NULL AND NEW.`from_uid` <> 0) THEN
    SET NEW.`from_id` = NEW.`from_uid`;
  END IF;
END //

CREATE TRIGGER `trg_friend_apply_bu_sync_from`
BEFORE UPDATE ON `friend_apply`
FOR EACH ROW
BEGIN
  IF NEW.`from_id` <> OLD.`from_id` THEN
    SET NEW.`from_uid` = NEW.`from_id`;
  ELSEIF NEW.`from_uid` <> OLD.`from_uid` THEN
    SET NEW.`from_id` = NEW.`from_uid`;
  END IF;
END //
DELIMITER ;

-- =========================
-- 2) friend
-- =========================
CREATE TABLE IF NOT EXISTS `friend` (
  `id` BIGINT NOT NULL AUTO_INCREMENT,
  `self_id` INT NOT NULL,
  `friend_id` INT NOT NULL,
  `back` VARCHAR(255) NOT NULL DEFAULT '',
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_friend_pair` (`self_id`, `friend_id`),
  KEY `idx_friend_self_id` (`self_id`),
  KEY `idx_friend_friend_id` (`friend_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

