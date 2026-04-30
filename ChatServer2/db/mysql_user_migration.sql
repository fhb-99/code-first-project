-- ChatServer MySQL user-table migration (idempotent)
-- 目标：让 `user` 表字段与 `New-Client/ChatServer` 代码一致：
--   uid, name, email, pwd, nick, desc, sex, icon
--
-- 使用方法：
--   USE <你的schema>;   -- 例如：USE cjh;
--   source mysql_user_migration.sql;

-- 1) 创建表（不存在才创建）
CREATE TABLE IF NOT EXISTS `user` (
  `uid` INT NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(255) NOT NULL,
  `email` VARCHAR(255) NOT NULL,
  `pwd` VARCHAR(255) NOT NULL,
  `nick` VARCHAR(255) NOT NULL DEFAULT '',
  `desc` VARCHAR(1024) NOT NULL DEFAULT '',
  `sex` INT NOT NULL DEFAULT 0,
  `icon` VARCHAR(1024) NOT NULL DEFAULT '',
  PRIMARY KEY (`uid`),
  UNIQUE KEY `uk_user_name` (`name`),
  UNIQUE KEY `uk_user_email` (`email`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 2) 兼容老表列名：id -> uid, passwd -> pwd（仅在检测到对应列时执行）
SET @schema_name := DATABASE();

SET @has_uid := (
  SELECT COUNT(*) FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = @schema_name AND TABLE_NAME = 'user' AND COLUMN_NAME = 'uid'
);
SET @has_id := (
  SELECT COUNT(*) FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = @schema_name AND TABLE_NAME = 'user' AND COLUMN_NAME = 'id'
);
SET @has_pwd := (
  SELECT COUNT(*) FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = @schema_name AND TABLE_NAME = 'user' AND COLUMN_NAME = 'pwd'
);
SET @has_passwd := (
  SELECT COUNT(*) FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = @schema_name AND TABLE_NAME = 'user' AND COLUMN_NAME = 'passwd'
);

-- 如果存在 id 且不存在 uid，则改名为 uid（保持 AUTO_INCREMENT/NOT NULL）
SET @sql := IF(@has_id > 0 AND @has_uid = 0, 'ALTER TABLE `user` CHANGE COLUMN `id` `uid` INT NOT NULL AUTO_INCREMENT', 'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- 如果存在 passwd 且不存在 pwd，则改名为 pwd
SET @sql := IF(@has_passwd > 0 AND @has_pwd = 0, 'ALTER TABLE `user` CHANGE COLUMN `passwd` `pwd` VARCHAR(255) NOT NULL', 'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- 3) 补齐资料字段（不存在才 ADD）
SET @has_nick := (
  SELECT COUNT(*) FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = @schema_name AND TABLE_NAME = 'user' AND COLUMN_NAME = 'nick'
);
SET @has_desc := (
  SELECT COUNT(*) FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = @schema_name AND TABLE_NAME = 'user' AND COLUMN_NAME = 'desc'
);
SET @has_sex := (
  SELECT COUNT(*) FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = @schema_name AND TABLE_NAME = 'user' AND COLUMN_NAME = 'sex'
);
SET @has_icon := (
  SELECT COUNT(*) FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = @schema_name AND TABLE_NAME = 'user' AND COLUMN_NAME = 'icon'
);

SET @sql := IF(@has_nick = 0, 'ALTER TABLE `user` ADD COLUMN `nick` VARCHAR(255) NOT NULL DEFAULT ''''', 'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql := IF(@has_desc = 0, 'ALTER TABLE `user` ADD COLUMN `desc` VARCHAR(1024) NOT NULL DEFAULT ''''', 'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql := IF(@has_sex = 0, 'ALTER TABLE `user` ADD COLUMN `sex` INT NOT NULL DEFAULT 0', 'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql := IF(@has_icon = 0, 'ALTER TABLE `user` ADD COLUMN `icon` VARCHAR(1024) NOT NULL DEFAULT ''''', 'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- =========================
-- reg_user 存储过程（与代码一致：CALL reg_user(?,?,?,@result)）
-- =========================
DROP PROCEDURE IF EXISTS `reg_user`;
DELIMITER //
CREATE PROCEDURE `reg_user`(
  IN new_name VARCHAR(255),
  IN new_email VARCHAR(255),
  IN new_pwd VARCHAR(255),
  OUT result INT
)
BEGIN
  DECLARE EXIT HANDLER FOR SQLEXCEPTION
  BEGIN
    ROLLBACK;
    SET result = -1;
  END;

  START TRANSACTION;

  IF EXISTS (SELECT 1 FROM `user` WHERE `name` = new_name) THEN
    SET result = 0;
    ROLLBACK;
  ELSEIF EXISTS (SELECT 1 FROM `user` WHERE `email` = new_email) THEN
    SET result = 0;
    ROLLBACK;
  ELSE
    INSERT INTO `user` (`name`, `email`, `pwd`)
    VALUES (new_name, new_email, new_pwd);

    SET result = LAST_INSERT_ID();
    COMMIT;
  END IF;
END //
DELIMITER ;
