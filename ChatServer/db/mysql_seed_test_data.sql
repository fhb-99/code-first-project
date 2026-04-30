-- ChatServer seed data (idempotent)
-- Usage:
--   USE <your_schema>;   -- e.g. USE cjh;
--   source mysql_seed_test_data.sql;

-- 1) Ensure base user rows exist and all profile columns are filled
INSERT INTO `user` (`uid`, `name`, `email`, `pwd`, `nick`, `desc`, `sex`, `icon`)
VALUES
  (1, 'aaa', 'ylncf@qq.com', '745230', '阿A', '这是账号 aaa 的测试简介', 0, 'avatar_aaa.png'),
  (2, 'bbb', 'bbb@test.com', '123456', '小B', '这是账号 bbb 的测试简介', 1, 'avatar_bbb.png'),
  (3, 'ccc', 'ccc@test.com', '123456', '小C', '这是账号 ccc 的测试简介', 0, 'avatar_ccc.png')
ON DUPLICATE KEY UPDATE
  `name` = VALUES(`name`),
  `email` = VALUES(`email`),
  `pwd` = VALUES(`pwd`),
  `nick` = VALUES(`nick`),
  `desc` = VALUES(`desc`),
  `sex` = VALUES(`sex`),
  `icon` = VALUES(`icon`);

-- If uid=1 already exists with another name/email, at least fill nullable profile fields
UPDATE `user`
SET
  `nick` = CASE WHEN `nick` IS NULL OR `nick` = '' THEN CONCAT('用户', `uid`) ELSE `nick` END,
  `desc` = CASE WHEN `desc` IS NULL OR `desc` = '' THEN CONCAT('测试用户', `uid`, '的默认简介') ELSE `desc` END,
  `icon` = CASE WHEN `icon` IS NULL OR `icon` = '' THEN CONCAT('avatar_', `uid`, '.png') ELSE `icon` END
WHERE `uid` IN (1, 2, 3);

-- 2) Friend apply seed
INSERT INTO `friend_apply` (`from_id`, `from_uid`, `to_uid`, `status`)
VALUES
  (2, 2, 1, 0), -- bbb -> aaa pending
  (3, 3, 1, 1)  -- ccc -> aaa approved
ON DUPLICATE KEY UPDATE
  `from_uid` = VALUES(`from_uid`),
  `status` = VALUES(`status`);

-- 3) Friend relation seed (approved pair: aaa <-> ccc)
INSERT INTO `friend` (`self_id`, `friend_id`, `back`)
VALUES
  (1, 3, '小C'),
  (3, 1, '阿A')
ON DUPLICATE KEY UPDATE
  `back` = VALUES(`back`);

